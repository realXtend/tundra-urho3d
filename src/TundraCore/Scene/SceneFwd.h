/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   SceneFwd.h
    @brief  Forward declarations and typedefs for Scene-related classes. */

#pragma once

#include "CoreTypes.h"

namespace Urho3D
{
    class XMLElement;
    class XMLFile;
    class Context;
}

namespace Tundra
{

class SceneAPI;
class Scene;
class Entity;
class IComponent;
class IComponentFactory;
class IAttribute;
class AttributeMetadata;
class ChangeRequest;
class Transform;

struct SceneDesc;
struct EntityDesc;
struct ComponentDesc;
struct AttributeDesc;
struct AssetDesc;
struct AssetDescCache;
struct EntityReference;
struct ParentingTracker;

typedef SharedPtr<Scene> ScenePtr;
typedef WeakPtr<Scene> SceneWeakPtr;
typedef WeakPtr<Entity> EntityWeakPtr;
typedef SharedPtr<Entity> EntityPtr;
typedef Vector<EntityPtr> EntityVector;
typedef SharedPtr<IComponent> ComponentPtr;
typedef WeakPtr<IComponent> ComponentWeakPtr;
typedef SharedPtr<IComponentFactory> ComponentFactoryPtr;
typedef Vector<IAttribute*> AttributeVector;
typedef HashMap<String, ScenePtr> SceneMap;

typedef Vector<EntityDesc> EntityDescList;
typedef Vector<ComponentDesc> ComponentDescList;
typedef Vector<AttributeDesc> AttributeDescList;
typedef Vector<AssetDesc> AssetDescList;

}
