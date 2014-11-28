// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "WaterPlane.h"
#include "Fog.h"

#include "Placeable.h"
#include "AttributeMetadata.h"
#include "Renderer.h"
#include "Scene/Scene.h"
#include "GraphicsWorld.h"
#include "LoggingFunctions.h"
#include "Framework.h"
#include "FrameAPI.h"
#include "Profiler.h"
#include "IMaterialAsset.h"
#include "AssetRefListener.h"
#include "UrhoRenderer.h"
#include "Camera.h"

#include <Engine/Scene/Node.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Graphics/StaticModel.h>
#include <Engine/Resource/ResourceCache.h>
#include <Engine/Graphics/Model.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Graphics/Zone.h>
#include <Engine/Graphics/Camera.h>

namespace Tundra
{

WaterPlane::WaterPlane(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(xSize, "X-size", 5000),
    INIT_ATTRIBUTE_VALUE(ySize, "Y-size", 5000),
    INIT_ATTRIBUTE_VALUE(depth, "Depth", 10000),
    INIT_ATTRIBUTE_VALUE(position, "Position", float3::zero),
    INIT_ATTRIBUTE_VALUE(rotation, "Rotation", Quat::identity),
    INIT_ATTRIBUTE_VALUE(scaleUfactor, "U factor", 0.0002f),
    INIT_ATTRIBUTE_VALUE(scaleVfactor, "V factor", 0.0002f),
    INIT_ATTRIBUTE_VALUE(xSegments, "Segments in x", 10),
    INIT_ATTRIBUTE_VALUE(ySegments, "Segments in y", 10),
    INIT_ATTRIBUTE_VALUE(materialRef, "Material ref", AssetReference("", "OgreMaterial")),
    INIT_ATTRIBUTE_VALUE(fogColor, "Fog color", Color(0.2f,0.4f,0.35f,1.0f)),
    INIT_ATTRIBUTE_VALUE(fogStartDistance, "Fog start dist.", 100.f),
    INIT_ATTRIBUTE_VALUE(fogEndDistance, "Fog end dist.", 2000.f),
    INIT_ATTRIBUTE_VALUE(fogMode, "Fog mode", 3),
    INIT_ATTRIBUTE_VALUE(fogExpDensity, "Fog exponential density", 0.001f),
    cameraInsideWaterCube_(false)
{
    static AttributeMetadata fogModeMetadata;
    static bool metadataInitialized = false;
    if (!metadataInitialized)
    {
        fogModeMetadata.enums[Fog::None] = "NoFog";
        fogModeMetadata.enums[Fog::Exponentially] = "Exponentially";
        fogModeMetadata.enums[Fog::ExponentiallySquare] = "ExponentiallySquare";
        fogModeMetadata.enums[Fog::Linear] = "Linearly";
        metadataInitialized = true;
    }
    fogMode.SetMetadata(&fogModeMetadata);

    materialAsset_ = new AssetRefListener();

    ParentEntitySet.Connect(this, &WaterPlane::UpdateSignals);
}

WaterPlane::~WaterPlane()
{
    if (world_.Expired())
    {
        if (waterPlane_)
            LogError("WaterPlane: World has expired, skipping uninitialization!");
        return;
    }

    if (waterPlane_)
    {
        framework->Frame()->Updated.Disconnect(this, &WaterPlane::Update);
        
        RestoreFog();
        Detach();
    
        waterPlane_.Reset();
        
        adjustmentNode_->Remove();
        adjustmentNode_.Reset();
    }
}

void WaterPlane::UpdateSignals()
{
    if (!ViewEnabled())
        return;

    Entity* parent = ParentEntity();
    if (!parent)
        return;

    parent->ComponentAdded.Connect(this, &WaterPlane::OnComponentStructureChanged);
    parent->ComponentRemoved.Connect(this, &WaterPlane::OnComponentStructureChanged);

    if (parent->ParentScene())
        world_ = parent->ParentScene()->Subsystem<GraphicsWorld>();

    if (world_ && !waterPlane_)
    {
        Urho3D::Scene* urhoScene = world_->UrhoScene();
        adjustmentNode_ = urhoScene->CreateChild("AdjustmentNode");
        
        // Make the entity & component links for identifying raycasts
        adjustmentNode_->SetVar(GraphicsWorld::entityLink, Variant(WeakPtr<RefCounted>(parent)));
        adjustmentNode_->SetVar(GraphicsWorld::componentLink, Variant(WeakPtr<RefCounted>(this)));

        waterPlane_ = adjustmentNode_->CreateComponent<Urho3D::StaticModel>();
        waterPlane_->SetModel(GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::Model>("Models/Plane.mdl"));
        if (materialRef.Get().ref.Empty())
            waterPlane_->SetMaterial(GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::Material>("Materials/Water.xml"));
       
        SetupWaterPlane();

        materialAsset_->Loaded.Connect(this, &WaterPlane::OnMaterialAssetLoaded);

        framework->Frame()->Updated.Connect(this, &WaterPlane::Update);
    }

    // Make sure we attach to the Placeable if exists.
    Attach();
}

void WaterPlane::OnComponentStructureChanged(IComponent*, AttributeChange::Type)
{
    // No-op if attached to the same placeable already
    if (placeable_ == parentEntity->Component<Placeable>())
        return;

    Attach();
}

void WaterPlane::Attach()
{
    if (!waterPlane_ || world_.Expired())
        return;

    // Detach first, in case the original placeable no longer exists
    Detach();

    Entity *entity = ParentEntity();
    if (!entity)
        return;
    placeable_ = entity->Component<Placeable>();
    if (!placeable_)
        return;

    Urho3D::Node* placeableNode = placeable_->UrhoSceneNode();
    if (!placeableNode)
    {
        LogError("Can not attach mesh: placeable does not have an Urho3D scene node");
        return;
    }
    adjustmentNode_->SetParent(placeableNode);
}

void WaterPlane::Detach()
{
    if (!waterPlane_ || world_.Expired())
        return;

    if (placeable_)
    {
        Urho3D::Scene* urhoScene = world_->UrhoScene();
        // When removed from the placeable, attach to scene root to avoid being removed from scene
        adjustmentNode_->SetParent(urhoScene);
        placeable_.Reset();
    }
}

void WaterPlane::Update(float /*frametime*/)
{
    if (!world_.Get())
        return;
    PROFILE(EC_WaterPlane_Update);
    bool cameraWasInsideWaterCube = cameraInsideWaterCube_;
    cameraInsideWaterCube_ = IsCameraInsideWaterCube();
    if (!cameraWasInsideWaterCube && cameraInsideWaterCube_)
        SetUnderwaterFog();
    else if (cameraWasInsideWaterCube && !cameraInsideWaterCube_)
        RestoreFog();
}

float3 WaterPlane::GetPointOnPlane(const float3 &point) const 
{
    if (!adjustmentNode_)
        return float3::nan;

    Urho3D::Matrix3x4 worldTM = adjustmentNode_->GetTransform(); // local->world
    Urho3D::Matrix3x4 inv = worldTM.Inverse(); // world->local
    Urho3D::Vector3 local = inv * Urho3D::Vector3(point.x, point.y, point.z);

    local.y_ = 0;
    Urho3D::Vector3 world = worldTM * local;
    return world;
}

float WaterPlane::GetDistanceToWaterPlane(const float3& point) const
{
    if (!waterPlane_)
        return 0.f;

    return point.y - GetPointOnPlane(point).y;
}

bool WaterPlane::IsTopOrBelowWaterPlane(const float3& point) const
{
    if (!adjustmentNode_)
        return false;

    float3 local = adjustmentNode_->GetWorldRotation().Inverse() * (point - adjustmentNode_->GetWorldPosition()) / adjustmentNode_->GetWorldScale();

    int x = xSize.Get(), y = ySize.Get();

    float xMax = x * 0.5f;
    float yMax = y * 0.5f;
    float xMin = -x * 0.5f;
    float yMin = -y * 0.5f;

    if (local.x > xMin && local.x < xMax && local.y > yMin && local.y < yMax)
        return true;

    return false;
}

bool WaterPlane::IsPointInsideWaterCube(const float3& point) const
{
    if (!waterPlane_  || !ViewEnabled())
        return false;

    if (IsTopOrBelowWaterPlane(point))
    {
        float d = GetDistanceToWaterPlane(point);
        if (d < 0 && depth.Get() >= fabs(d))
            return true;
    }

    return false;
}

bool WaterPlane::IsCameraInsideWaterCube()
{
    if (!waterPlane_ || !ViewEnabled() || !world_)
        return false;

    Camera* camera = world_->Renderer()->MainCameraComponent();
    if (!camera)
        return false;

    float3 posCamera = camera->UrhoCamera()->GetNode()->GetWorldPosition();
    if (IsTopOrBelowWaterPlane(posCamera))
    {
        float d = GetDistanceToWaterPlane(posCamera);
        if (d < 0 && depth.Get() >= fabs(d))
            return true;
    }

    return false;
}

void WaterPlane::SetupWaterPlane()
{
    if (!waterPlane_)
        return;

    waterPlane_->SetCastShadows(false);
    adjustmentNode_->SetScale(Urho3D::Vector3((float)xSize.Get(), 1, (float)ySize.Get()));

    waterPlane_->GetMaterial()->SetUVTransform(Urho3D::Vector2::ZERO, 0, Urho3D::Vector2(scaleUfactor.Get(), scaleVfactor.Get()));
}

void WaterPlane::OnMaterialAssetLoaded(AssetPtr asset)
{
    IMaterialAsset *material = dynamic_cast<IMaterialAsset*>(asset.Get());
    if (material && waterPlane_)
    {
        waterPlane_->SetMaterial(material->UrhoMaterial());
    }
}

void WaterPlane::AttributesChanged()
{
    if (xSize.ValueChanged() || ySize.ValueChanged() || scaleUfactor.ValueChanged() || scaleVfactor.ValueChanged())
        SetupWaterPlane();

    if (xSegments.ValueChanged() || ySegments.ValueChanged())
        SetupWaterPlane();

    if (position.ValueChanged())
        SetPosition();

    if (rotation.ValueChanged())
        SetOrientation();

    if (fogColor.ValueChanged() || fogStartDistance.ValueChanged() || fogEndDistance.ValueChanged() ||
        fogMode.ValueChanged() || fogExpDensity.ValueChanged())
    {
        if (IsCameraInsideWaterCube()) // Apply fog immediately only if the camera is within the water cube.
            SetUnderwaterFog();
    }

    if (materialRef.ValueChanged() && materialAsset_)
        materialAsset_->HandleAssetRefChange(&materialRef);
}

void WaterPlane::SetPosition()
{
    if (!adjustmentNode_ || !ViewEnabled())
        return;

    const float3 &pos = position.Get();
    if (pos.IsFinite())
        adjustmentNode_->SetPosition(pos);
}

void WaterPlane::SetOrientation()
{
    if (!adjustmentNode_ || !ViewEnabled())
        return;

    adjustmentNode_->SetRotation(rotation.Get());
}

void WaterPlane::SetUnderwaterFog()
{
    if (!ViewEnabled())
        return;

    if (world_)
    {
        /// @todo in Tundra1-series, if we were within WaterPlane, the waterPlaneColor*fogColor was used as the scene fog color.
        /// Do we want to do the same here?
        Urho3D::Zone* zone = world_->UrhoZone();
        if (zone)
        {
            zone->SetFogColor(fogColor.Get());
            zone->SetFogStart(fogStartDistance.Get());
            zone->SetFogEnd(fogEndDistance.Get());
        }
    }
}

void WaterPlane::RestoreFog()
{
    if (!ViewEnabled() || !world_)
        return;

    // Restore current scene fog settings, if existing.
    /// @todo Optimize
    Vector<SharedPtr<Fog> > fogs = ParentScene() ? ParentScene()->Components<Fog>() : Vector<SharedPtr<Fog> >();
    Urho3D::Zone* zone = world_->UrhoZone();
    if (!fogs.Empty() && zone)
    {
        zone->SetFogColor(fogs[0]->color.Get());
        zone->SetFogStart(fogs[0]->startDistance.Get());
        zone->SetFogEnd(fogs[0]->endDistance.Get());
    }
    // No scene fog, set the default fog.
    else
        world_->SetDefaultSceneFog();
}

}
