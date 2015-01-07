// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"
#include "IComponent.h"
#include "IAttribute.h"

#include <Urho3D/Core/Variant.h>

namespace Tundra
{

struct DeserializeData;

/// A component that allows adding of dynamically structured attributes at runtime.
/** <table class="header">
    <tr>
    <td>
    <h2>DynamicComponent</h2>
    Component for which user can add and delete attributes at runtime.
    <b> Name of the attributes must be unique. </b>
    It's recommend to use attribute names when you set or get your attribute values because
    indices can change while the dynamic component's attributes are added or removed.

    Use CreateAttribute for creating new attributes.

    When component is deserialized it will compare old and a new attribute values and will get difference
    between those two and use that information to remove attributes that are not in the new list and add those
    that are only in new list and only update those values that are same in both lists.

    Registered by SceneAPI.

    <b>No Static Attributes.</b>

    Does not react on entity actions.

    Does not emit any actions.

    <b>Doesn't depend on any components</b>.

    </table> */
class TUNDRACORE_API DynamicComponent : public IComponent
{
    COMPONENT_NAME(DynamicComponent, 25)

public:
     /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit DynamicComponent(Urho3D::Context* context, Scene* scene);
    /// @endcond
    ~DynamicComponent();

    void DeserializeFrom(Urho3D::XMLElement& element, AttributeChange::Type change) override;
    void SerializeToBinary(kNet::DataSerializer& dest) const override;
    void DeserializeFromBinary(kNet::DataDeserializer& source, AttributeChange::Type change) override;
    bool SupportsDynamicAttributes() const override { return true; }

    /// A factory method that constructs a new attribute of a given the type name.
    /** @param typeName Type name of the attribute, see SceneAPI::AttributeTypes().
        @param id ID of the attribute, case-insensitive.
        @param change Change type.
        This factory is not extensible. If attribute was already created the method will return it's pointer.

        @note If multiple clients, or the client and the server, add attributes at the same time, unresolvable
        scene replication conflits will occur. The exception is filling attributes immediately after creation
        (before the component is replicated for the first time), which is supported. Prefer to either create
        all attributes at creation, or to only add new attributes on the server. 
        
        @note Name of the attribute will be assigned to same as the ID. */
    IAttribute *CreateAttribute(const String &typeName, const String &id, AttributeChange::Type change = AttributeChange::Default);

    /// Checks if a given component @c comp is holding exactly same attributes as this component.
    /** @param comp Component to be compared with.
        @return Return true if component is holding same attributes as this component else return false. */
    bool ContainSameAttributes(const DynamicComponent &comp) const;

    /// Remove attribute from the component.
    /** @param id ID of the attribute. */
    void RemoveAttribute(const String &id, AttributeChange::Type change = AttributeChange::Default);

    /// Check if component is holding an attribute by the @c name.
    /** @param id ID of attribute that we are looking for, case-insensitive. */
    bool ContainsAttribute(const String &id) const;

    /// Removes all attributes from the component
    void RemoveAllAttributes(AttributeChange::Type change = AttributeChange::Default);

private:
    void DeserializeCommon(Vector<DeserializeData>& deserializedAttributes, AttributeChange::Type change);
    /// Convert attribute index without holes (used by client) into actual attribute index. Returns below zero if not found. Requires a linear search.
    int GetInternalAttributeIndex(int index) const;
};

COMPONENT_TYPEDEFS(DynamicComponent)

}
