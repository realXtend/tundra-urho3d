// For conditions of distribution and use, see copyright notice in license.txt

namespace Tundra
{

template <typename T>
SharedPtr<T> Entity::CreateComponent(const String &name, AttributeChange::Type change, bool replicated)
{
    return Urho3D::DynamicCast<T>(CreateComponent(T::ComponentTypeId, name, change, replicated)); /**< @todo static_pointer_cast should be ok here. */
}

template<typename T>
SharedPtr<T> Entity::GetOrCreateComponent(const String &name, AttributeChange::Type change, bool replicated)
{
    SharedPtr<T> existing = name.Empty() ? Component<T>() : Component<T>(name);
    if (existing)
        return existing;

    return CreateComponent<T>(name, change, replicated);
}

template <class T>
SharedPtr<T> Entity::Component() const
{
    return Urho3D::StaticCast<T>(Component(T::TypeIdStatic()));
}

template <class T>
Vector<SharedPtr<T> > Entity::ComponentsOfType() const
{
    Vector<SharedPtr<T> > ret;
    for(ComponentMap::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
    {
        SharedPtr<T> t = Urho3D::StaticCast<T>(i->second_);
        if (t)
            ret.Push(t);
    }
    return ret;
}

template <class T>
SharedPtr<T> Entity::Component(const String& name) const
{
    return Urho3D::DynamicCast<T>(Component(T::TypeIdStatic(), name));
}

}

