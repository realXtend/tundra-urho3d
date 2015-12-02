// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

namespace Urho3D
{
    class AnimatedModel;
    class Animation;
    class AnimationState;
    class BoundingBox;
    class Camera;
    class Image;
    class Light;
    class Material;
    class Model;
    class Node;
    class Scene;
    class StaticModel;
    class Texture2D;
    class Zone;
    class ParticleEffect;
    class ParticleEmitter;
}

namespace Tundra
{
    class UrhoRenderer;
    class GraphicsWorld;
    class Placeable;
    class Mesh;
    class Camera;
    class TextureAsset;
    class IOgreMaterialProcessor;
    class IMaterialAsset;
    class IMeshAsset;

    typedef SharedPtr<GraphicsWorld> GraphicsWorldPtr;
    typedef WeakPtr<GraphicsWorld> GraphicsWorldWeakPtr;
    typedef SharedPtr<TextureAsset> TextureAssetPtr;
    typedef SharedPtr<IMaterialAsset> MaterialAssetPtr;

    namespace Ogre
    {
        class MaterialParser;
    }
}
