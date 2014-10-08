// For conditions of distribution and use, see copyright notice in license.txt

namespace Tundra
{

template <class T>
SharedPtr<T> Scene::Subsystem() const
{
    SubsystemMap::ConstIterator it = subsystems.Find(T::GetTypeStatic());
    return Urho3D::DynamicCast<T>(it->second_);
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
                ret.insert(ret.End(), components.Begin(), components.End());
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
