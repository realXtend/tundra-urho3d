// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Mesh.h"
#include "Scene/Scene.h"
#include "GraphicsWorld.h"
#include "AttributeMetadata.h"

#include <Log.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Node.h>

namespace Tundra
{

Mesh::Mesh(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(nodeTransformation, "Transform", Transform(float3(0,0,0),float3(0,0,0),float3(1,1,1))),
    INIT_ATTRIBUTE_VALUE(meshRef, "Mesh ref", AssetReference("", "OgreMesh")),
    INIT_ATTRIBUTE_VALUE(skeletonRef, "Skeleton ref", AssetReference("", "OgreSkeleton")),
    INIT_ATTRIBUTE_VALUE(materialRefs, "Mesh materials", AssetReferenceList("OgreMaterial")), /**< @todo 24.10.2013 Rename name to "Material refs" or similar. */
    INIT_ATTRIBUTE_VALUE(drawDistance, "Draw distance", 0.0f),
    INIT_ATTRIBUTE_VALUE(castShadows, "Cast shadows", false),
    INIT_ATTRIBUTE_VALUE(useInstancing, "Use instancing", false),
    adjustmentNode_(0)
{
    if (scene)
        world_ = scene->Subsystem<GraphicsWorld>();

    static AttributeMetadata drawDistanceData("", "0", "10000");
    drawDistance.SetMetadata(&drawDistanceData);

    static AttributeMetadata materialMetadata;
    materialMetadata.elementType = "AssetReference";
    materialRefs.SetMetadata(&materialMetadata);

    GraphicsWorldPtr world = world_.Lock();
    if (world)
    {
        Urho3D::Scene* scene = world->UrhoScene();
        adjustmentNode_ = scene->CreateChild("MeshAdjust");
    }
}

Mesh::~Mesh()
{
    if (world_.Expired())
        return;

    if (adjustmentNode_)
    {
        adjustmentNode_->Remove();
        adjustmentNode_ = 0;
    }
}

Urho3D::Node* Mesh::BoneNode(const String& name) const
{
    // When a skeletal mesh is created, the bone hierarchy will be under the adjustment node
    return adjustmentNode_ ? adjustmentNode_->GetChild(name, true) : (Urho3D::Node*)0;
}

}
