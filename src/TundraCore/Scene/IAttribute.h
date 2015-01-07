/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   IAttribute.h
    @brief  Abstract base class and template class implementations for entity-component attributes. */

#pragma once

#include "TundraCoreApi.h"
#include "CoreDefines.h"
#include "AttributeChangeType.h"
#include "SceneFwd.h"

#include <Urho3D/Container/Ptr.h>

namespace Tundra
{

/// Abstract base class for entity-component attributes.
/** Concrete attribute classes will be subclassed out of this. */
class TUNDRACORE_API IAttribute
{
public:
    /// Constructor
    /** @param owner Component which this attribute will be attached to.
        @param id ID of the attribute. Will also be assigned as the attribute's human-readable name. */
    IAttribute(IComponent* owner, const char* id);

    /// Constructor
    /** @param owner Component which this attribute will be attached to.
        @param id ID of the attribute.
        @param name Human-readable name of the attribute. */
    IAttribute(IComponent* owner, const char* id, const char* name);

    virtual ~IAttribute() {}

    /// Returns attribute's owner component.
    IComponent* Owner() const { return owner; }

    /// Returns the ID of the attribute for serialization. Should be same as the variable/property name.
    const String &Id() const { return id; }

    /// Returns human-readable name of the attribute. This is shown in the EC editor. For dynamic attributes, is the same as ID.
    const String &Name() const { return name; }

    /// Change the attribute's name. Needed for PlaceholderComponent when constructing attributes dynamically at deserialization
    void SetName(const String& newName);

    /// Writes attribute to string for XML serialization
    virtual String ToString() const = 0;

    /// Reads attribute from string for XML deserialization
    virtual void FromString(const String &str, AttributeChange::Type change) = 0;

    /// Returns the type name of the data stored in this attribute.
    /** @note As attribute type names are handled case-insensitively internally by the SceneAPI,
        a case-insensitive comparison is recommended when comparing attribute type names.
        In general, comparing by type ID, instead of type name, is recommended for efficiency.
        @sa TypeId */
    virtual const String &TypeName() const = 0;
    
    /// Returns the type ID of this attribute.
    virtual u32 TypeId() const = 0;

    /// Writes attribute to binary for binary serialization
    virtual void ToBinary(kNet::DataSerializer& dest) const = 0;

    /// Reads attribute from binary for binary deserialization
    virtual void FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change) = 0;

    /// Sets attribute's metadata.
    /** If owner component is set calls IComponent::EmitAttributeMetadataChanged to make IComponent emit metadata changed signal.
        If you change metadata directly with Metadata().property = value; you need to call 
        EmitAttributeMetadataChanged() to notify the component of the change.
        @param meta Metadata. */
    void SetMetadata(AttributeMetadata *meta);

    /// Returns attribute's metadata, or null if no metadata exists.
    AttributeMetadata *Metadata() const;
    
    /// Informs the parent component that this attribute's metadata has changed.
    /** @see SetMetadata and IComponent::EmitAttributeMetadataChanged. */
    void EmitAttributeMetadataChanged();

    /// Returns whether attribute has been dynamically allocated. By default false
    bool IsDynamic() const { return dynamic; }
    
    /// Returns attribute's index in the parent component
    u8 Index() const { return index; }
    
    /// Notifies owner component that the attribute has changed.
    /** This function is called automatically when the Attribute value is Set(). You may call this manually
        to force a change signal to be emitted for this attribute. Calling this is equivalent to calling the
        IComponent::AttributeChanged(this->Name()) for the owner of this attribute. */
    void Changed(AttributeChange::Type change);

    /// Creates a clone of this attribute by dynamic allocation.
    /** The caller is responsible for eventually freeing the created attribute. The clone will have the same type and value, but no owner.
        This is meant to be used by network sync managers and such, that need to do interpolation/extrapolation/dead reckoning. */
    virtual IAttribute* Clone() const = 0;

    /// Copies the value from another attribute of the same type.
    virtual void CopyValue(IAttribute* source, AttributeChange::Type change) = 0;

    /// Interpolates the value of this attribute based on two values, and a lerp factor between 0 and 1
    /** The attributes given must be of the same type for the result to be defined.
        Is a no-op if the attribute (for example string) does not support interpolation.
        The value will be set using the given change type. */
    virtual void Interpolate(IAttribute* start, IAttribute* end, float t, AttributeChange::Type change) = 0;

    /// Returns whether the value of this attribute is dirty and pending an update by the component that owns this attribute.
    /** This flag is used to optimize the attribute update events from an asymptotically Theta(n^2) operation to a Theta(n) when loading a scene or
            creating a new component with all new attributes. This also enables the components themselves to avoid having to cache "shadow" values of its attributes 
            in the component, in order to optimize its loading.
        @return If true, an external source (scene load event, UI attribute edit event, a script, received sync message from sync manager, etc.)
            has modified the value of this attribute, but the *implementation* of the component this attribute is part of has not
            yet reacted to this change.
        @note This flag is NOT to be used except by the code that is implementing a new component. Do not read this flag from client code
            that is trying to detect if an attribute has changed, use the attribute changed signal in the component instead. */
    bool ValueChanged() const { return valueChanged; }

    /// Acknowledges that the component owning this attribute has updated the component state to reflect the current value of this attribute.
    void ClearChangedFlag() { valueChanged = false; }

    /// Attribute types supported by the system.
    enum TypeId : u32
    {
        NoneId = 0,
        StringId = 1,
        IntId = 2,
        RealId = 3,
        ColorId = 4,
        Float2Id = 5,
        Float3Id = 6,
        Float4Id = 7,
        BoolId = 8,
        UIntId = 9,
        QuatId = 10,
        AssetReferenceId = 11,
        AssetReferenceListId = 12,
        EntityReferenceId = 13,
        VariantId = 14,
        VariantListId = 15,
        TransformId = 16,
        PointId = 17,
        NumTypes = 18
    };

    static const String NoneTypeName;
    static const String StringTypeName;
    static const String IntTypeName;
    static const String RealTypeName;
    static const String ColorTypeName;
    static const String Float2TypeName;
    static const String Float3TypeName;
    static const String Float4TypeName;
    static const String BoolTypeName;
    static const String UIntTypeName;
    static const String QuatTypeName;
    static const String AssetReferenceTypeName;
    static const String AssetReferenceListTypeName;
    static const String EntityReferenceTypeName;
    static const String VariantTypeName;
    static const String VariantListTypeName;
    static const String TransformTypeName;
    static const String PointTypeName;

protected:
    friend class SceneAPI;
    friend class IComponent;
    
    IComponent* owner; ///< Owning component.
    String id; ///< ID of attribute.
    String name; ///< Human-readable name of attribute for editing.
    AttributeMetadata *metadata; ///< Possible attribute metadata.
    bool dynamic; ///< Dynamic attributes must be deleted at component destruction
    u8 index; ///< Attribute index in the parent component's attribute list

    /// If true, the value of this attribute has changed, but the implementing code has not yet reacted to it.
    /// @see ValueChanged().
    bool valueChanged;
};

typedef Vector<IAttribute*> AttributeVector;

/// Attribute template class
template<typename T>
class Attribute : public IAttribute
{
public:
    /** Constructor.
        value is initialiazed to DefaultValue.
        @param owner Owner component.
        @param id Attribute ID */
    Attribute(IComponent* owner, const char* id) :
        IAttribute(owner, id),
        value(DefaultValue())
    {
    }

    /** Constructor taking also initial value.
        @param owner Owner component.
        @param id Attribute ID
        @param val Value. */
    Attribute(IComponent* owner, const char* id, const T &val) :
        IAttribute(owner, id),
        value(val)
    {
    }

    /** Constructor taking attribute ID/name separately
        value is initialiazed to DefaultValue.
        @param owner Owner component.
        @param id Attribute id.
        @param name Human-readable name. */
    Attribute(IComponent* owner, const char* id, const char* name) :
        IAttribute(owner, id, name),
        value(DefaultValue())
    {
    }

    /** Constructor taking initial value and attribute ID/name separately.
        @param owner Owner component.
        @param id Attribute ID.
        @param name Human-readable name.
        @param val Value. */
    Attribute(IComponent* owner, const char* id, const char* name, const T &val) :
        IAttribute(owner, id, name),
        value(val)
    {
    }

    /// Returns attribute's value.
    const T &Get() const { return value; }

    /** Sets attribute's value.
        @param value New value.
        @param change Change type. */
    void Set(const T &value, AttributeChange::Type change = AttributeChange::Default)
    {
        this->value = value;
        valueChanged = true; // Signal to IComponent owning this attribute that the value of this attribute has changed.
        Changed(change);
    }

    IAttribute* Clone() const override
    {
        Attribute<T>* new_attr = new Attribute<T>(0, name.CString());
        new_attr->metadata = metadata;
        // The new attribute has no owner, so the Changed function will have no effect, and therefore the changetype does not actually matter
        new_attr->Set(Get(), AttributeChange::Disconnected);
        return static_cast<IAttribute*>(new_attr);
    }

    void CopyValue(IAttribute* source, AttributeChange::Type change) override
    {
        Attribute<T>* source_attr = dynamic_cast<Attribute<T>*>(source);
        if (source_attr)
            Set(source_attr->Get(), change);
    }

    String ToString() const override;
    void FromString(const String& str, AttributeChange::Type change) override;
    void ToBinary(kNet::DataSerializer& dest) const override;
    void FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change) override;
    void Interpolate(IAttribute* start, IAttribute* end, float t, AttributeChange::Type change) override;
    const String &TypeName() const override;
    u32 TypeId() const override;

    /// Returns pre-defined default value for the attribute.
    /** Usually zero for primitive data types and for classes/structs that are collections of primitive data types (e.g. float3::zero), or the default consturctor. */
    T DefaultValue() const;

private:
    T value; ///< The value of this Attribute.
};

/// Represents weak pointer to an attribute.
struct AttributeWeakPtr
{
    AttributeWeakPtr() : attribute(nullptr) {}
    /// Constructor.
    /** @param c Owner component.
        @param a The actual attribute. */
    AttributeWeakPtr(IComponent* c, IAttribute *a) : owner(c), attribute(a) {}

    /// Returns pointer to the attribute or null if the owner component doesn't exist anymore.
    IAttribute *Get() const { return !owner.Expired() ? attribute : 0; }

    bool operator ==(const AttributeWeakPtr &rhs) const
    {
        ComponentPtr ownerPtr = owner.Lock();
        return rhs.owner.Lock() == ownerPtr && (rhs.attribute == attribute || !ownerPtr);
    }

    bool operator !=(const AttributeWeakPtr &rhs) const { return !(*this == rhs); }

    /// Returns if the owner of the attribute is expired, i.e. it's not safe to access the attribute.
    bool Expired() const { return owner.Expired(); }

    ComponentWeakPtr owner; ///< Owner component.
    IAttribute *attribute; ///< The actual attribute.
};

}
