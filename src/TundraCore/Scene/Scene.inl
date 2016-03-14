// For conditions of distribution and use, see copyright notice in license.txt

namespace Tundra
{

template <class T>
SharedPtr<T> Scene::Subsystem() const
{
    for (SubsystemMap::ConstIterator it = subsystems.Begin(); it != subsystems.End(); ++it)
    {
        if (it->second_->GetType() == T::GetTypeStatic())
            return Urho3D::StaticCast<T>(it->second_);
    }
    return SharedPtr<T>();
}

template <typename T>
Vector<SharedPtr<T> > Scene::Components(const String &name) const
{
    Vector<SharedPtr<T> > ret;
    if (name.Empty())
    {
        for(ConstIterator it = Begin(); it != End(); ++it)
        {
            Vector<SharedPtr<T> > components =  it->second_->ComponentsOfType<T>();
            if (!components.Empty())
                ret.Insert(ret.End(), components.Begin(), components.End());
        }
    }
    else
    {
        for(ConstIterator it = Begin(); it != End(); ++it)
        {
            SharedPtr<T> component = it->second_->Component<T>(name);
            if (component)
                ret.Push(component);
        }
    }
    return ret;
}

template <typename T>
EntityVector Scene::EntitiesWithComponent(const String &name) const
{
    return EntitiesWithComponent(T::ComponentTypeId, name);
}

}
