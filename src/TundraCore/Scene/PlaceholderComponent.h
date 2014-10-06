// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "IComponent.h"
#include "IAttribute.h"

namespace Tundra
{

/// Placeholder/fallback for components that don't currently have a registered C++ implementation.
class TUNDRACORE_API PlaceholderComponent : public IComponent
{
    OBJECT(CustomComponent);

public:
     /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit PlaceholderComponent(Urho3D::Context* context, Scene* scene);
    /// @endcond
    ~PlaceholderComponent();

    /// Returns the static typename of this component, which will always be CustomComponent.
    static const String &TypeNameStatic()
    {
        static const String name = "CustomComponent";
        return name;
    }
    /// Returns the static type ID of this component. Always 0.
    static u32 TypeIdStatic()
    {
        return 0;
    }
    /// Returns the stored typename, ie. the typename this component is acting as a placeholder to.
    virtual const String &TypeName() const
    {
        return typeName;
    }
    /// Returns the stored typeid, ie. the typeid this component is acting as a placeholder to.
    virtual u32 TypeId() const
    {
        return typeId;
    }

    /// IComponent override
    virtual void DeserializeFromBinary(kNet::DataDeserializer& source, AttributeChange::Type change);

    /// IComponent override
    /** PlaceholderComponent attributes need to be treated as static for the network protocol, though they are dynamically allocated */
    virtual int NumStaticAttributes() const { return static_cast<int>(attributes.Size()); }

    void SetTypeId(u32 newTypeId);
    void SetTypeName(const String& newTypeName);
    IAttribute *CreateAttribute(const String &typeName, const String &id, const String &name, AttributeChange::Type change = AttributeChange::Default);

private:
    String typeName;
    u32 typeId;
};

}
