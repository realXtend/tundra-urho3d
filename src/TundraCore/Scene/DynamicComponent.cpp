// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "DynamicComponent.h"
#include "SceneAPI.h"
#include "Entity.h"
#include "Scene.h"
#include "LoggingFunctions.h"

#include <kNet/DataSerializer.h>
#include <kNet/DataDeserializer.h>

#include <Urho3D/Resource/XMLFile.h>
#include <Sort.h>

namespace Tundra
{

/** @cond PRIVATE */
struct DeserializeData
{
    DeserializeData(const String &id = "", const String &type = "", const String &value = ""):
        id_(id),
        type_(type),
        value_(value)
    {
    }

    String id_;
    String type_;
    String value_;
};

/// Function that is used by std::sort algorithm to sort attributes by their ID.
bool CmpAttributeById(const IAttribute *a, const IAttribute *b)
{
    return a->Id().Compare(b->Id(), false) < 0;
}

/// Function that is used by std::sort algorithm to sort DeserializeData by their ID.
bool CmpAttributeDataById(const DeserializeData &a, const DeserializeData &b)
{
    return a.id_.Compare(b.id_, false) < 0;
}

/** @endcond */

DynamicComponent::DynamicComponent(Urho3D::Context* context, Scene* scene):
    IComponent(context, scene)
{
}

DynamicComponent::~DynamicComponent()
{
}

void DynamicComponent::DeserializeFrom(Urho3D::XMLElement& element, AttributeChange::Type change)
{
    if (!BeginDeserialization(element))
        return;

    Vector<DeserializeData> deserializedAttributes;
    Urho3D::XMLElement child = element.GetChild("attribute");
    while(!child.IsNull())
    {
        String id = child.GetAttribute("id");
        // Fallback if ID is not defined
        if (!id.Length())
            id = child.GetAttribute("name");
        String type = child.GetAttribute("type");
        String value = child.GetAttribute("value");
        DeserializeData attributeData(id, type, value);
        deserializedAttributes.Push(attributeData);

        child = child.GetNext("attribute");
    }

    DeserializeCommon(deserializedAttributes, change);
}

void DynamicComponent::DeserializeCommon(Vector<DeserializeData>& deserializedAttributes, AttributeChange::Type change)
{
    // Sort both lists in alphabetical order.
    AttributeVector oldAttributes = NonEmptyAttributes();
    Sort(oldAttributes.Begin(), oldAttributes.End(), &CmpAttributeById);
    Sort(deserializedAttributes.Begin(), deserializedAttributes.End(), &CmpAttributeDataById);

    Vector<DeserializeData> addAttributes;
    Vector<DeserializeData> remAttributes;
    AttributeVector::Iterator iter1 = oldAttributes.Begin();
    Vector<DeserializeData>::Iterator iter2 = deserializedAttributes.Begin();

    // Check what attributes we need to add or remove from the dynamic component (done by comparing two list differences).
    while(iter1 != oldAttributes.End() || iter2 != deserializedAttributes.End())
    {
        // No point to continue the iteration if other list is empty. We can just push all new attributes into the dynamic component.
        if(iter1 == oldAttributes.End())
        {
            for(;iter2 != deserializedAttributes.End(); ++iter2)
                addAttributes.Push(*iter2);
            break;
        }
        // Only old attributes are left and they can be removed from the dynamic component.
        else if(iter2 == deserializedAttributes.End())
        {
            for(;iter1 != oldAttributes.End(); ++iter1)
                remAttributes.Push(DeserializeData((*iter1)->Id()));
            break;
        }

        // Attribute has already created and we only need to update it's value.
        if((*iter1)->Id() == (*iter2).id_)
        {
            //SetAttribute(String::fromStdString(iter2->name_), String::fromStdString(iter2->value_), change);
            for(AttributeVector::ConstIterator attr_iter = attributes.Begin(); attr_iter != attributes.End(); ++attr_iter)
                if((*attr_iter)->Id() == iter2->id_)
                    (*attr_iter)->FromString(iter2->value_, change);

            ++iter2;
            ++iter1;
        }
        // Found a new attribute that need to be created and added to the component.
        else if((*iter1)->Id() > (*iter2).id_)
        {
            addAttributes.Push(*iter2);
            ++iter2;
        }
        // Couldn't find the attribute in a new list so it need to be removed from the component.
        else
        {
            remAttributes.Push(DeserializeData((*iter1)->Id()));
            ++iter1;
        }
    }

    while(!addAttributes.Empty())
    {
        DeserializeData attributeData = addAttributes.Back();
        IAttribute *attribute = CreateAttribute(attributeData.type_, attributeData.id_);
        if (attribute)
            attribute->FromString(attributeData.value_, change);
        addAttributes.Pop();
    }
    while(!remAttributes.Empty())
    {
        DeserializeData attributeData = remAttributes.Back();
        RemoveAttribute(attributeData.id_);
        remAttributes.Pop();
    }
}

IAttribute *DynamicComponent::CreateAttribute(const String &typeName, const String &id, AttributeChange::Type change)
{
    if (ContainsAttribute(id))
        return IComponent::AttributeById(id);

    IAttribute *attribute = SceneAPI::CreateAttribute(typeName, id);
    if (!attribute)
    {
        LogError("Failed to create new attribute of type \"" + typeName + "\" with ID \"" + id + "\" to dynamic component \"" + Name() + "\".");
        return nullptr;
    }

    IComponent::AddAttribute(attribute);

    Scene* scene = ParentScene();
    if (scene)
        scene->EmitAttributeAdded(this, attribute, change);

    AttributeAdded.Emit(attribute);
    EmitAttributeChanged(attribute, change);
    return attribute;
}

void DynamicComponent::RemoveAttribute(const String &id, AttributeChange::Type change)
{
    for(AttributeVector::ConstIterator iter = attributes.Begin(); iter != attributes.End(); ++iter)
        if(*iter && (*iter)->Id().Compare(id, false) == 0)
        {
            IComponent::RemoveAttribute((*iter)->Index(), change);
            break;
        }
}

void DynamicComponent::RemoveAllAttributes(AttributeChange::Type change)
{
    for(AttributeVector::ConstIterator iter = attributes.Begin(); iter != attributes.End(); ++iter)
        if (*iter)
            IComponent::RemoveAttribute((*iter)->Index(), change);

    attributes.Clear();
}

int DynamicComponent::GetInternalAttributeIndex(int index) const
{
    if (index >= (int)attributes.Size())
        return -1; // Can be sure that is not found.
    int cmp = 0;
    for (unsigned i = 0; i < attributes.Size(); ++i)
    {
        if (!attributes[i])
            continue;
        if (cmp == index)
            return i;
        ++cmp;
    }
    return -1;
}

bool DynamicComponent::ContainSameAttributes(const DynamicComponent &comp) const
{
    AttributeVector myAttributeVector = NonEmptyAttributes();
    AttributeVector attributeVector = comp.NonEmptyAttributes();
    if(attributeVector.Size() != myAttributeVector.Size())
        return false;
    if(attributeVector.Empty() && myAttributeVector.Empty())
        return true;

    Sort(myAttributeVector.Begin(), myAttributeVector.End(), &CmpAttributeById);
    Sort(attributeVector.Begin(), attributeVector.End(), &CmpAttributeById);

    AttributeVector::ConstIterator iter1 = myAttributeVector.Begin();
    AttributeVector::ConstIterator iter2 = attributeVector.Begin();
    while(iter1 != myAttributeVector.End() && iter2 != attributeVector.End())
    {
        // Compare attribute names and type and if they mach continue iteration if not components aren't exactly the same.
        if (((*iter1)->Id().Compare((*iter2)->Id(), false) == 0) &&
            (*iter1)->TypeName().Compare((*iter2)->TypeName(), false) == 0)
        {
            if(iter1 != myAttributeVector.End())
                ++iter1;
            if(iter2 != attributeVector.End())
                ++iter2;
        }
        else
        {
            return false;
        }
    }
    return true;
}

bool DynamicComponent::ContainsAttribute(const String &id) const
{
    return AttributeById(id) != 0;
}

void DynamicComponent::SerializeToBinary(kNet::DataSerializer& dest) const
{
    dest.Add<u8>((u8)attributes.Size());
    // For now, transmit all values as strings
    AttributeVector::ConstIterator iter = attributes.Begin();
    while(iter != attributes.End())
    {
        if (*iter)
        {
            dest.AddString((*iter)->Id().CString());
            dest.AddString((*iter)->TypeName().CString());
            dest.AddString((*iter)->ToString().CString()); /**< @todo Use UTF-8, see Attribute<String>::ToBinary */
        }
        ++iter;
    }
}

void DynamicComponent::DeserializeFromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    u8 num_attributes = source.Read<u8>();
    Vector<DeserializeData> deserializedAttributes;
    for(uint i = 0; i < num_attributes; ++i)
    {
        std::string id = source.ReadString();
        std::string typeName = source.ReadString();
        std::string value = source.ReadString();
        
        DeserializeData attrData(id.c_str(), typeName.c_str(), value.c_str());
        deserializedAttributes.Push(attrData);
    }

    DeserializeCommon(deserializedAttributes, change);
}

}
