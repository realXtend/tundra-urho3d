// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"
#include "IComponent.h"

#include <Str.h>
#include <Context.h>
#include <RefCounted.h>

namespace Tundra
{

/// A common interface for factories which instantiate components of different types.
class TUNDRACORE_API IComponentFactory : public Urho3D::RefCounted
{
public:
    virtual ~IComponentFactory() {}

    virtual const String &TypeName() const = 0;
    virtual u32 TypeId() const = 0;
    virtual ComponentPtr Create(Scene* scene, const String &newComponentName) const = 0;
//    virtual ComponentPtr Clone(IComponent *existingComponent, const String &newComponentName) const = 0;
};

/// A factory for instantiating components of a templated type T.
template<typename T>
class GenericComponentFactory : public IComponentFactory
{
public:
    const String &TypeName() const { return T::TypeNameStatic(); }
    u32 TypeId() const { return T::TypeIdStatic(); }

    ComponentPtr Create(Urho3D::Context* context, Scene* scene, const String &newComponentName) const
    {
        ComponentPtr component(new T(context, scene));
        component->SetName(newComponentName);
        return component;
    }
/*     ///\todo Implement this.

    ComponentPtr Clone(IComponent *existingComponent, const String &newComponentName) const
    {
        if (!existingComponent)
        {
            LogError("Cannot clone component from a null pointer!");
            return ComponentPtr();
        }

        T *existing = dynamic_cast<T*>(existingComponent);
        if (!existing)
        {
            LogError("Cannot clone component of type \"" + TypeName() + " from a component of type \"" + existingComponent->TypeName() + "\"!");
            return ComponentPtr();
        }
//        ComponentPtr component = MAKE_SHARED(T, *existingComponent);
        ComponentPtr component = SharedPtr<T>(new T, *existingComponent);
        component->SetName(newComponentName);
        return component;
    }
    */
};

}
