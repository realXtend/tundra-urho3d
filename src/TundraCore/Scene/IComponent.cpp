/*
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   IComponent.cpp
    @brief  The common interface for all components, which are the building blocks the scene entities are formed of. */

#include "StableHeaders.h"

#include "IComponent.h"
#include "Entity.h"
#include "Scene.h"
#include "SceneAPI.h"

#include "Framework.h"

#include <XMLFile.h>

#include <kNet/DataSerializer.h>
#include <kNet/DataDeserializer.h>
#include <ForEach.h>
#include <Log.h>

namespace Tundra
{

IComponent::IComponent(Urho3D::Context* context, Scene* scene) :
    Object(context),
    parentEntity(0),
    framework(scene ? scene->GetFramework() : 0),
    updateMode(AttributeChange::Replicate),
    replicated(true),
    temporary(false),
    id(0)
{
}

IComponent::~IComponent()
{
    foreach(IAttribute* a, attributes)
    {
        if (a && a->IsDynamic())
            SAFE_DELETE(a);
    }
}

void IComponent::SetNewId(entity_id_t newId)
{
    id = newId;
}

void IComponent::SetName(const String& name_)
{
    // no point to send a signal if name have stayed same as before.
    if (componentName == name_)
        return;

    String oldName = componentName;
    componentName = name_;
    ComponentNameChanged.Emit(componentName, oldName);
}

bool IComponent::IsUnacked() const
{
    return id >= UniqueIdGenerator::FIRST_UNACKED_ID && id < UniqueIdGenerator::FIRST_LOCAL_ID;
}

void IComponent::SetUpdateMode(AttributeChange::Type defaultMode)
{
    // Note: we can't allow default mode to be Default, because that would be meaningless
    if (defaultMode == AttributeChange::Disconnected || defaultMode == AttributeChange::LocalOnly ||
        defaultMode == AttributeChange::Replicate)
    {
        updateMode = defaultMode;
    }
    else
    {
        LOGWARNING("IComponent::SetUpdateMode: Trying to set default update mode to an invalid value! (" + String((int)defaultMode) + ")");
    }
}

void IComponent::SetParentEntity(Entity* entity)
{
    if (entity)
    {
        parentEntity = entity;
         // Make sure that framework pointer will be valid, in case this component was originally created unparented.
        framework = parentEntity->GetFramework();
        ParentEntitySet.Emit();
    }
    else
    {
        // Let signal cleanup etc. happen before the parent entity is set null
        ParentEntityAboutToBeDetached.Emit();
        parentEntity = entity;
    }
}

Entity* IComponent::ParentEntity() const
{
    return parentEntity;
}

Scene* IComponent::ParentScene() const
{
    if (!parentEntity)
        return 0;
    return parentEntity->ParentScene();
}

void IComponent::SetReplicated(bool enable)
{
    if (id)
    {
        LOGERROR("Replication mode can not be changed after an ID has been assigned!");
        return;
    }
    
    replicated = enable;
}

AttributeVector IComponent::NonEmptyAttributes() const
{
    AttributeVector ret;
    for (unsigned i = 0; i < attributes.Size(); ++i)
        if (attributes[i])
            ret.Push(attributes[i]);
    return ret;
}

StringVector IComponent::AttributeNames() const
{
    StringVector attribute_list;
    for(AttributeVector::ConstIterator iter = attributes.Begin(); iter != attributes.End(); ++iter)
        if (*iter)
            attribute_list.Push((*iter)->Name());
    return attribute_list;
}

StringVector IComponent::AttributeIds() const
{
    StringVector attribute_list;
    for(AttributeVector::ConstIterator iter = attributes.Begin(); iter != attributes.End(); ++iter)
        if (*iter)
            attribute_list.Push((*iter)->Id());
    return attribute_list;
}

bool IComponent::ShouldBeSerialized(bool serializeTemporary, bool serializeLocal) const
{
    if (IsTemporary() && !serializeTemporary)
        return false;
    if (IsLocal() && !serializeLocal)
        return false;
    return true;
}

IAttribute* IComponent::AttributeById(const String &id) const
{
    for(uint i = 0; i < attributes.Size(); ++i)
        if(attributes[i] && attributes[i]->Id().Compare(id, false) == 0)
            return attributes[i];
    return 0;
}

IAttribute* IComponent::AttributeByName(const String &name) const
{
    for(uint i = 0; i < attributes.Size(); ++i)
        if(attributes[i] && attributes[i]->Name().Compare(name, false) == 0)
            return attributes[i];
    return 0;
}

int IComponent::NumAttributes() const
{
    int ret = 0;
    for(uint i = 0; i < attributes.Size(); ++i)
        if(attributes[i])
            ++ret;
    return ret;
}

int IComponent::NumStaticAttributes() const
{
    int ret = 0;
    for(uint i = 0; i < attributes.Size(); ++i)
    {
        // Break when the first hole or dynamically allocated attribute is encountered
        if (attributes[i] && !attributes[i]->IsDynamic())
            ++ret;
        else
            break;
    }
    return ret;
}

IAttribute* IComponent::CreateAttribute(u8 index, u32 typeID, const String& id, AttributeChange::Type change)
{
    if (!SupportsDynamicAttributes())
    {
        LOGERROR("CreateAttribute called on a component that does not support dynamic attributes");
        return 0;
    }
    
    // If this message should be sent with the default attribute change mode specified in the IComponent,
    // take the change mode from this component.
    if (change == AttributeChange::Default)
        change = updateMode;
    assert(change != AttributeChange::Default);

    IAttribute *attribute = SceneAPI::CreateAttribute(typeID, id);
    if(!attribute)
        return 0;
    
    if (!AddAttribute(attribute, index))
    {
        delete attribute;
        return 0;
    }
    
    // Trigger scenemanager signal
    Scene* scene = ParentScene();
    if (scene)
        scene->EmitAttributeAdded(this, attribute, change);
    
    // Trigger internal signal
    AttributeAdded.Emit(attribute);
    EmitAttributeChanged(attribute, change);
    return attribute;
}

void IComponent::RemoveAttribute(u8 index, AttributeChange::Type change)
{
    if (!SupportsDynamicAttributes())
    {
        LOGERROR("RemoveAttribute called on a component that does not support dynamic attributes");
        return;
    }
    
    // If this message should be sent with the default attribute change mode specified in the IComponent,
    // take the change mode from this component.
    if (change == AttributeChange::Default)
        change = updateMode;
    assert(change != AttributeChange::Default);

    if (index < attributes.Size() && attributes[index])
    {
        IAttribute* attr = attributes[index];
        if (!attr->IsDynamic())
        {
            LOGERROR("Can not remove static attribute at index " + String((int)index));
            return;
        }
        
        // Trigger scenemanager signal
        Scene* scene = ParentScene();
        if (scene)
            scene->EmitAttributeRemoved(this, attr, change);
        
        // Trigger internal signal(s)
        AttributeAboutToBeRemoved.Emit(attr);
        SAFE_DELETE(attributes[index]);
    }
    else
        LOGERROR("Can not remove nonexisting attribute at index " + String((int)index));
}

void IComponent::AddAttribute(IAttribute* attr)
{
    if (!attr)
        return;
    // If attribute is static (member variable attributes), we can just Push it.
    if (!attr->IsDynamic())
    {
        attr->index = (u8)attributes.Size();
        attr->owner = this;
        attributes.Push(attr);
    }
    else
    {
        // For dynamic attributes, need to scan for holes first, and then Push if no holes
        for (unsigned i = 0; i < attributes.Size(); ++i)
        {
            if (!attributes[i])
            {
                attr->index = (u8)i;
                attr->owner = this;
                attributes[i] = attr;
                return;
            }
        }
        attr->index = (u8)attributes.Size();
        attr->owner = this;
        attributes.Push(attr);
    }
}

bool IComponent::AddAttribute(IAttribute* attr, u8 index)
{
    if (!attr)
        return false;
    if (index < attributes.Size())
    {
        IAttribute* existing = attributes[index];
        if (existing)
        {
            if (!existing->IsDynamic())
            {
                LOGERROR("Can not overwrite static attribute at index " + String((int)index));
                return false;
            }
            else
            {
                LOGWARNING("Removing existing attribute at index " + String((int)index) + " to make room for new attribute");
                delete existing;
                attributes[index] = 0;
            }
        }
    }
    else
    {
        // Make enough holes until we can reach the index
        while (attributes.Size() <= index)
            attributes.Push(0);
    }
    
    attr->index = index;
    attr->owner = this;
    attributes[index] = attr;

    return true;
}

Urho3D::XMLElement IComponent::BeginSerialization(Urho3D::XMLFile& doc, Urho3D::XMLElement& base_element, bool serializeTemporary) const
{
    Urho3D::XMLElement comp_element;
    if (!base_element)
        comp_element = doc.CreateRoot("component");
    else
        comp_element = base_element.CreateChild("component");

    comp_element.SetAttribute("type", EnsureTypeNameWithoutPrefix(TypeName())); /**< @todo 27.09.2013 typeName would be better here */
    comp_element.SetUInt("typeId", TypeId());
    if (!componentName.Empty())
        comp_element.SetAttribute("name", componentName);
    comp_element.SetBool("sync", replicated);
    if (serializeTemporary)
        comp_element.SetBool("temporary", temporary);

    return comp_element;
}

void IComponent::WriteAttribute(Urho3D::XMLFile& /*doc*/, Urho3D::XMLElement& comp_element, const String& name, const String& id, const String& value, const String &type) const
{
    Urho3D::XMLElement attribute_element = comp_element.CreateChild("attribute");
    attribute_element.SetAttribute("name", name);
    attribute_element.SetAttribute("id", id);
    attribute_element.SetAttribute("value", value);
    attribute_element.SetAttribute("type", type);
}

void IComponent::WriteAttribute(Urho3D::XMLFile& doc, Urho3D::XMLElement& compElement, const IAttribute *attr) const
{
    WriteAttribute(doc, compElement, attr->Name(), attr->Id(), attr->ToString(), attr->TypeName());
}

bool IComponent::BeginDeserialization(Urho3D::XMLElement& compElem)
{
    if (Urho3D::ToUInt(compElem.GetAttribute("typeId")) == TypeId() || // typeId takes precedence over typeName
        EnsureTypeNameWithoutPrefix(compElem.GetAttribute("type")) == TypeName())
    {
        SetName(compElem.GetAttribute("name"));
        return true;
    }

    return false;
}

void IComponent::EmitAttributeChanged(IAttribute* attribute, AttributeChange::Type change)
{
    // If this message should be sent with the default attribute change mode specified in the IComponent,
    // take the change mode from this component.
    if (change == AttributeChange::Default)
        change = updateMode;
    assert(change != AttributeChange::Default);

    if (change == AttributeChange::Disconnected)
        return; // No signals
    
    // Trigger scenemanager signal
    Scene* scene = ParentScene();
    if (scene)
        scene->EmitAttributeChanged(this, attribute, change);
    
    // Trigger internal signal
    AttributeChanged.Emit(attribute, change);

    // Tell the derived class that some attributes have changed.
    AttributesChanged();
    // After having notified the derived class, clear all change bits on all attributes,
    // since the derived class has reacted on them. 
    for(uint i = 0; i < attributes.Size(); ++i)
        if (attributes[i])
            attributes[i]->ClearChangedFlag();
}

void IComponent::EmitAttributeMetadataChanged(IAttribute* attribute)
{
    if (!attribute)
        return;
    if (!attribute->Metadata())
    {
        LOGWARNING("IComponent::EmitAttributeMetadataChanged: Given attributes metadata is null, signal won't be emitted!");
        return;
    }
    AttributeMetadataChanged.Emit(attribute, attribute->Metadata());
}

void IComponent::EmitAttributeChanged(const String& attributeName, AttributeChange::Type change)
{
    IAttribute *attr = AttributeByName(attributeName);
    if (attr)
        EmitAttributeChanged(attr, change);
}

void IComponent::SerializeTo(Urho3D::XMLFile& doc, Urho3D::XMLElement& base_element, bool serializeTemporary) const
{
    Urho3D::XMLElement comp_element = BeginSerialization(doc, base_element, serializeTemporary);

    for(uint i = 0; i < attributes.Size(); ++i)
        if (attributes[i])
            WriteAttribute(doc, comp_element, attributes[i]);
}

void IComponent::DeserializeFrom(Urho3D::XMLElement& element, AttributeChange::Type change)
{
    if (!BeginDeserialization(element))
        return;

    // If this message should be sent with the default attribute change mode specified in the IComponent,
    // take the change mode from this component.
    if (change == AttributeChange::Default)
        change = updateMode;
    assert(change != AttributeChange::Default);

    // When we are deserializing the component from XML, only apply those attribute values which are present in that XML element.
    // For all other elements, use the current value in the attribute (if this is a newly allocated component, the current value
    // is the default value for that attribute specified in ctor. If this is an existing component, the DeserializeFrom can be 
    // thought of applying the given "delta modifications" from the XML element).
    Urho3D::XMLElement attribute_element = element.GetChild("attribute");
    while (attribute_element)
    {
        IAttribute* attr = 0;
        String id = attribute_element.GetAttribute("id");
        // Prefer lookup by ID if it's specified, but fallback to using attribute's human-readable name if ID not defined or erroneous.
        if (!id.Empty())
            attr = AttributeById(id);
        if (!attr)
        {
            id = attribute_element.GetAttribute("name");
            attr = AttributeByName(id);
        }
        
        if (!attr)
            LOGWARNING(TypeName() + "::DeserializeFrom: Could not find attribute \"" + id + "\" specified in the XML element.");
        else
            attr->FromString(attribute_element.GetAttribute("value"), change);
        
        attribute_element = attribute_element.GetNext("attribute");
    }
}

void IComponent::SerializeToBinary(kNet::DataSerializer& dest) const
{
    dest.Add<u8>((u8)attributes.Size());
    for(uint i = 0; i < attributes.Size(); ++i)
        if (attributes[i])
            attributes[i]->ToBinary(dest);
}

void IComponent::DeserializeFromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    u8 num_attributes = source.Read<u8>();
    if (num_attributes != NumAttributes())
    {
        LOGERROR("Wrong number of attributes in DeserializeFromBinary!");
        return;
    }
    for(uint i = 0; i < attributes.Size(); ++i)
        if (attributes[i])
            attributes[i]->FromBinary(source, change);
}

void IComponent::ComponentChanged(AttributeChange::Type change)
{
    // If this message should be sent with the default attribute change mode specified in the IComponent,
    // take the change mode from this component.
    if (change == AttributeChange::Default)
        change = updateMode;

    // We are signalling attribute changes, but the desired change type is saying "don't signal about changes".
    assert(change != AttributeChange::Default && change != AttributeChange::Disconnected);

    for(uint i = 0; i < attributes.Size(); ++i)
        if (attributes[i])
            EmitAttributeChanged(attributes[i], change);
}

void IComponent::SetTemporary(bool enable)
{
    temporary = enable;
}

bool IComponent::IsTemporary() const
{
    if (parentEntity && parentEntity->IsTemporary())
        return true;
    return temporary;
}

bool IComponent::ViewEnabled() const
{
    if (!parentEntity)
        return true;
    Scene* scene = parentEntity->ParentScene();
    if (scene)
        return scene->ViewEnabled();
    else
        return true;
}

}
