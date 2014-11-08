// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "GraphicsWorld.h"
#include "UrhoRenderer.h"

#include "Entity.h"
#include "Scene/Scene.h"
#include "ConfigAPI.h"
#include "FrameAPI.h"
#include "LoggingFunctions.h"
#include "Camera.h"
#include "Placeable.h"
#include "Framework.h"
#include "Math/Transform.h"
#include "Math/Color.h"

#include <Math/float2.h>
#include <Math/float3x4.h>
#include <Geometry/AABB.h>
#include <Geometry/OBB.h>
#include <Geometry/Plane.h>
#include <Geometry/LineSegment.h>
#include <Math/float3.h>
#include <Math/float4.h>
#include <Geometry/Circle.h>
#include <Geometry/Sphere.h>

#include <Profiler.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Graphics/DebugRenderer.h>
#include <Engine/Graphics/Drawable.h>
#include <Engine/Graphics/Octree.h>
#include <Engine/Graphics/OctreeQuery.h>
#include <Engine/Graphics/Renderer.h>
#include <Engine/Graphics/View.h>
#include <Engine/Graphics/Viewport.h>
#include <Engine/Graphics/Zone.h>

namespace Tundra
{

StringHash GraphicsWorld::entityLink("ENTITY");
StringHash GraphicsWorld::componentLink("COMPONENT");

GraphicsWorld::GraphicsWorld(UrhoRenderer* owner, Scene* scene) :
    Object(owner->GetContext()),
    framework_(scene->GetFramework()),
    renderer_(owner),
    scene_(scene)
{
    urhoScene_ = new Urho3D::Scene(context_);
    urhoScene_->CreateComponent<Urho3D::Octree>();
    urhoScene_->CreateComponent<Urho3D::DebugRenderer>();

    // Set a default ambient color that matches the default ambient color of EnvironmentLight, in case there is no environmentlight component.
    Urho3D::Zone* zone = urhoScene_->CreateComponent<Urho3D::Zone>();
    // Just set up a large bounding zone for the whole scene
    /// \todo The octree size is not adjusted yet, so objects outside the default octree range always end up in the root octqant
    zone->SetBoundingBox(Urho3D::BoundingBox(-100000.0f, 100000.0f));
    zone->SetAmbientColor(DefaultSceneAmbientLightColor());
    
    SetDefaultSceneFog();

    // Connect to frame update to handle time-based update
    framework_->Frame()->Updated.Connect(this, &GraphicsWorld::OnUpdated);
}

GraphicsWorld::~GraphicsWorld()
{
    urhoScene_.Reset();
}

void GraphicsWorld::OnUpdated(float /*timeStep*/)
{
    PROFILE(GraphicsWorld_OnUpdated);

    visibleEntities_.Clear();

    Urho3D::Renderer* renderer = GetSubsystem<Urho3D::Renderer>();
    Camera* cameraComp = renderer_->MainCameraComponent();

    if (IsActive() && renderer)
    {
        Urho3D::Camera* cam = cameraComp ? cameraComp->UrhoCamera() : nullptr;
        Urho3D::Viewport* vp = renderer->GetViewport(0);
        Urho3D::View* view = vp ? vp->GetView() : nullptr;
        if (view)
        {
            const Urho3D::PODVector<Urho3D::Drawable*>& geometries = view->GetGeometries();
            for (uint i = 0; i < geometries.Size(); ++i)
            {
                // Verify that the geometry is in main camera view, as also eg. shadow geometries get listed
                Urho3D::Drawable* dr = geometries[i];
                if (!dr || !dr->IsInView(cam))
                    continue;
                EntityWeakPtr ent(static_cast<Entity*>(dr->GetNode()->GetVar(entityLink).GetPtr()));
                if (!ent)
                    continue;
                
                visibleEntities_.Insert(ent);
            }
        }
    }

    // Perform visibility change tracking
    for (HashMap<EntityWeakPtr, bool>::Iterator i = visibilityTrackedEntities_.Begin(); i != visibilityTrackedEntities_.End();)
    {
        // Check whether entity has expired
        if (!i->first_)
            i = visibilityTrackedEntities_.Erase(i);
        else
        {
            bool current = visibleEntities_.Contains(i->first_);
            bool prev = i->second_;
            if (current != prev)
            {
                i->second_ = current;
                if (current)
                {
                    i->first_->EmitEnterView(cameraComp);
                    EntityEnterView.Emit(i->first_.Get());
                }
                else
                {
                    i->first_->EmitLeaveView(cameraComp);
                    EntityLeaveView.Emit(i->first_.Get());
                }
            }

            ++i;
        }
    }
}

Color GraphicsWorld::DefaultSceneAmbientLightColor()
{
    return Color(0.364f, 0.364f, 0.364f, 1.f);
}

void GraphicsWorld::SetDefaultSceneFog()
{
    Urho3D::Zone* zone = UrhoZone();
    if (!zone)
        return;
    zone->SetFogStart(2000.0f);
    zone->SetFogEnd(4000.0f);
    zone->SetFogColor(Urho3D::Color::BLACK);
}

RayQueryResult GraphicsWorld::Raycast(int x, int y)
{
    return Raycast(x, y, 0xffffffff, FLOAT_INF);
}

RayQueryResult GraphicsWorld::Raycast(int x, int y, float maxDistance)
{
    return Raycast(x, y, 0xffffffff, maxDistance);
}

RayQueryResult GraphicsWorld::Raycast(int x, int y, unsigned layerMask)
{
    return Raycast(x, y, layerMask, FLOAT_INF);
}

RayQueryResult GraphicsWorld::Raycast(const Ray& ray, unsigned layerMask)
{
    return Raycast(ray, layerMask, FLOAT_INF);
}

RayQueryResult GraphicsWorld::Raycast(int x, int y, unsigned layerMask, float maxDistance)
{
    Ray ray;
    Camera* camera = renderer_->MainCameraComponent();
    if (camera)
        ray = camera->ScreenPointToRay(x, y);

    RaycastInternal(ray, layerMask, maxDistance, false);

    // Return the closest hit, or a cleared raycastresult if no hits
    return rayHits_.Size() ? rayHits_[0] : RayQueryResult();
}

RayQueryResult GraphicsWorld::Raycast(const Ray& ray, unsigned layerMask, float maxDistance)
{
    RaycastInternal(ray, layerMask, maxDistance, false);
    
    // Return the closest hit, or a cleared raycastresult if no hits
    return rayHits_.Size() ? rayHits_[0] : RayQueryResult();
}

Vector<RayQueryResult> GraphicsWorld::RaycastAll(int x, int y)
{
    return RaycastAll(x, y, 0xffffffff, FLOAT_INF);
}

Vector<RayQueryResult> GraphicsWorld::RaycastAll(int x, int y, float maxDistance)
{
    return RaycastAll(x, y, 0xffffffff, maxDistance);
}

Vector<RayQueryResult> GraphicsWorld::RaycastAll(int x, int y, unsigned layerMask)
{
    return RaycastAll(x, y, layerMask, FLOAT_INF);
}

Vector<RayQueryResult> GraphicsWorld::RaycastAll(const Ray& ray, unsigned layerMask)
{
    return RaycastAll(ray, layerMask, FLOAT_INF);
}

Vector<RayQueryResult> GraphicsWorld::RaycastAll(int x, int y, unsigned layerMask, float maxDistance)
{
    Ray ray;
    Camera* camera = renderer_->MainCameraComponent();
    if (camera)
        ray = camera->ScreenPointToRay(x, y);

    RaycastInternal(ray, layerMask, maxDistance, true);
    
    return rayHits_;
}

Vector<RayQueryResult> GraphicsWorld::RaycastAll(const Ray& ray, unsigned layerMask, float maxDistance)
{
    RaycastInternal(ray, layerMask, maxDistance, true);
    
    return rayHits_;
}

void GraphicsWorld::RaycastInternal(const Ray& ray, unsigned layerMask, float maxDistance, bool getAllResults)
{
    PROFILE(GraphicsWorld_Raycast);
    
    rayHits_.Clear();

    Urho3D::PODVector<Urho3D::RayQueryResult> result;
    Urho3D::Octree* octree = urhoScene_->GetComponent<Urho3D::Octree>();
    Urho3D::RayOctreeQuery query(result, ray, Urho3D::RAY_TRIANGLE, maxDistance, Urho3D::DRAWABLE_GEOMETRY);
    octree->Raycast(query);

    for (Urho3D::PODVector<Urho3D::RayQueryResult>::ConstIterator i = result.Begin(); i != result.End(); ++i)
    {
        if (!i->node_)
            continue;
        Entity* entity = static_cast<Entity*>(i->node_->GetVar(entityLink).GetPtr());
        if (!entity)
            continue; // Not a drawable associated with Tundra entity
        Placeable* placeable = entity->Component<Placeable>();
        if (placeable && (placeable->selectionLayer.Get() & layerMask) == 0)
            continue;
        IComponent* component = static_cast<IComponent*>(i->node_->GetVar(componentLink).GetPtr());
        
        RayQueryResult res;
        res.component = component;
        res.entity = entity;
        res.pos = i->position_;
        res.normal = i->normal_;
        /// \todo Fill the rest, like submesh information

        rayHits_.Push(res);
        if (!getAllResults)
            break;
    }
}

EntityVector GraphicsWorld::FrustumQuery(const Urho3D::IntRect &viewrect) const
{
    PROFILE(GraphicsWorld_FrustumQuery);
    
    EntityVector ret;
    Camera* cameraComp = renderer_->MainCameraComponent();
    if (!cameraComp || cameraComp->ParentScene() != scene_)
        return ret;
     Urho3D::Camera* cam = cameraComp->UrhoCamera();
     if (!cam)
        return ret;

    int width = renderer_->WindowWidth();
    int height = renderer_->WindowHeight();
    float w = (float)width;
    float h = (float)height;
    float left = (float)(viewrect.left_) / w, right = (float)(viewrect.right_) / w;
    float top = (float)(viewrect.top_) / h, bottom = (float)(viewrect.bottom_) / h;
    if (left > right) std::swap(left, right);
    if (top > bottom) std::swap(top, bottom);
    // don't do selection if box is too small
    if ((right - left) * (bottom - top) < 0.0001)
        return ret;

    Urho3D::Frustum fr;
    fr.vertices_[0] = cam->ScreenToWorldPoint(Urho3D::Vector3(right, top, cam->GetNearClip()));
    fr.vertices_[1] = cam->ScreenToWorldPoint(Urho3D::Vector3(right, bottom, cam->GetNearClip()));
    fr.vertices_[2] = cam->ScreenToWorldPoint(Urho3D::Vector3(left, bottom, cam->GetNearClip()));
    fr.vertices_[3] = cam->ScreenToWorldPoint(Urho3D::Vector3(left, top, cam->GetNearClip()));
    fr.vertices_[4] = cam->ScreenToWorldPoint(Urho3D::Vector3(right, top, cam->GetFarClip()));
    fr.vertices_[5] = cam->ScreenToWorldPoint(Urho3D::Vector3(right, bottom, cam->GetFarClip()));
    fr.vertices_[6] = cam->ScreenToWorldPoint(Urho3D::Vector3(left, bottom, cam->GetFarClip()));
    fr.vertices_[7] = cam->ScreenToWorldPoint(Urho3D::Vector3(left, top, cam->GetFarClip()));
    fr.UpdatePlanes();

    Urho3D::PODVector<Urho3D::Drawable*> result;
    Urho3D::FrustumOctreeQuery query(result, fr, Urho3D::DRAWABLE_GEOMETRY);
    urhoScene_->GetComponent<Urho3D::Octree>()->GetDrawables(query);

    for (Urho3D::PODVector<Urho3D::Drawable*>::ConstIterator i = result.Begin(); i != result.End(); ++i)
    {
        Entity* entity = static_cast<Entity*>((*i)->GetNode()->GetVar(entityLink).GetPtr());
        if (entity)
            ret.Push(EntityPtr(entity));
    }

    return ret;
}

bool GraphicsWorld::IsEntityVisible(Entity* entity) const
{
    return entity ? visibleEntities_.Contains(EntityWeakPtr(entity)) : false;
}

EntityVector GraphicsWorld::VisibleEntities() const
{
    EntityVector ret;

    for (HashSet<EntityWeakPtr>::ConstIterator i = visibleEntities_.Begin(); i != visibleEntities_.End(); ++i)
    {
        if (*i)
            ret.Push(i->Lock());
    }

    return ret;
}

bool GraphicsWorld::IsActive() const
{
    Entity* mainCamera = renderer_->MainCamera();
    return mainCamera && mainCamera->ParentScene() == scene_;
}

Urho3D::Zone* GraphicsWorld::UrhoZone() const
{
    return urhoScene_->GetComponent<Urho3D::Zone>();
}

void GraphicsWorld::DebugDrawAABB(const AABB &aabb, const Color &clr, bool depthTest)
{
    for(int i = 0; i < 12; ++i)
        DebugDrawLineSegment(aabb.Edge(i), clr, depthTest);
}

void GraphicsWorld::DebugDrawOBB(const OBB &obb, const Color &clr, bool depthTest)
{
    for(int i = 0; i < 12; ++i)
        DebugDrawLineSegment(obb.Edge(i), clr, depthTest);
}

void GraphicsWorld::DebugDrawLineSegment(const LineSegment &l, const Color &clr, bool depthTest)
{
    Urho3D::DebugRenderer* debug = urhoScene_->GetComponent<Urho3D::DebugRenderer>();
    if (debug)
        debug->AddLine(l.a, l.b, clr, depthTest);
}

void GraphicsWorld::DebugDrawLine(const float3& start, const float3& end, const Color &clr, bool depthTest)
{
    Urho3D::DebugRenderer* debug = urhoScene_->GetComponent<Urho3D::DebugRenderer>();
    if (debug)
        debug->AddLine(start, end, clr, depthTest);
}

void GraphicsWorld::DebugDrawPlane(const Plane &plane, const Color &clr, const float3 &refPoint, float uSpacing, float vSpacing, 
                               int uSegments, int vSegments, bool depthTest)
{
    float U0 = -uSegments * uSpacing / 2.f;
    float V0 = -vSegments * vSpacing / 2.f;

    float U1 = uSegments * uSpacing / 2.f;
    float V1 = vSegments * vSpacing / 2.f;

    for(int y = 0; y < vSegments; ++y)
        for(int x = 0; x < uSegments; ++x)
        {
            float u = U0 + x * uSpacing;
            float v = V0 + y * vSpacing;
            DebugDrawLine(plane.Point(U0, v, refPoint), plane.Point(U1, v, refPoint), clr, depthTest);
            DebugDrawLine(plane.Point(u, V0, refPoint), plane.Point(u, V1, refPoint), clr, depthTest);
        }
}

void GraphicsWorld::DebugDrawTransform(const Transform &t, float axisLength, float boxSize, const Color &clr, bool depthTest)
{
    DebugDrawFloat3x4(t.ToFloat3x4(), axisLength, boxSize, clr, depthTest);
}

void GraphicsWorld::DebugDrawFloat3x4(const float3x4 &t, float axisLength, float boxSize, const Color &clr, bool depthTest)
{
    AABB aabb(float3::FromScalar(-boxSize/2.f), float3::FromScalar(boxSize/2.f));
    OBB obb = aabb.Transform(t);
    DebugDrawOBB(obb, clr);
    DebugDrawLineSegment(LineSegment(t.TranslatePart(), t.TranslatePart() + axisLength * t.Col(0)), 1, 0, 0, depthTest);
    DebugDrawLineSegment(LineSegment(t.TranslatePart(), t.TranslatePart() + axisLength * t.Col(1)), 0, 1, 0, depthTest);
    DebugDrawLineSegment(LineSegment(t.TranslatePart(), t.TranslatePart() + axisLength * t.Col(2)), 0, 0, 1, depthTest);
}

void GraphicsWorld::DebugDrawCircle(const Circle &c, int numSubdivisions, const Color &clr, bool depthTest)
{
    float3 p = c.GetPoint(0);
    for(int i = 1; i <= numSubdivisions; ++i)
    {
        float3 p2 = c.GetPoint(i * 2.f * 3.14f / numSubdivisions);
        DebugDrawLineSegment(LineSegment(p, p2), clr, depthTest);
        p = p2;
    }
}

void GraphicsWorld::DebugDrawAxes(const float3x4 &t, bool depthTest)
{
    float3 translate, scale;
    Quat rotate;
    t.Decompose(translate, rotate, scale);
    
    DebugDrawLine(translate, translate + rotate * float3(scale.x, 0.f, 0.f), 1, 0, 0, depthTest);
    DebugDrawLine(translate, translate + rotate * float3(0., scale.y, 0.f), 0, 1, 0, depthTest);
    DebugDrawLine(translate, translate + rotate * float3(0.f, 0.f, scale.z), 0, 0, 1, depthTest);
}

void GraphicsWorld::DebugDrawSphere(const float3& center, float radius, int vertices, const Color &clr, bool depthTest)
{
    if (vertices <= 0)
        return;
    
    Vector<float3> positions(vertices);

    Sphere sphere(center, radius);
    int actualVertices = sphere.Triangulate(&positions[0], 0, 0, vertices, true);
    for (int i = 0; i < actualVertices; i += 3)
    {
        DebugDrawLine(positions[i], positions[i + 1], clr, depthTest);
        DebugDrawLine(positions[i + 1], positions[i + 2], clr, depthTest);
        DebugDrawLine(positions[i + 2], positions[i], clr, depthTest);
    }
}

void GraphicsWorld::DebugDrawLight(const float3x4 &t, int lightType, float range, float spotAngle, const Color &clr, bool depthTest)
{
    float3 translate, scale;
    Quat rotate;
    t.Decompose(translate, rotate, scale);
    float3 lightDirection = rotate * float3(0.0f, 0.0f, 1.0f);
    switch (lightType)
    {
        // Point
    case 0:
        DebugDrawCircle(Circle(translate, float3(1.f, 0.f, 0.f), range), 8, clr, depthTest);
        DebugDrawCircle(Circle(translate, float3(0.f, 1.f, 0.f), range), 8, clr, depthTest);
        DebugDrawCircle(Circle(translate, float3(0.f, 0.f, 1.f), range), 8, clr, depthTest);
        break;
        
        // Spot
    case 1:
        {
            float3 endPoint = translate + range * lightDirection;
            float coneRadius = range * sinf(DegToRad(spotAngle));
            Circle spotCircle(endPoint, -lightDirection, coneRadius);
            
            DebugDrawCircle(Circle(endPoint, -lightDirection, coneRadius), 8, clr, depthTest);
            for (int i = 1; i <= 8; ++i)
                DebugDrawLine(translate, spotCircle.GetPoint(i * 2.f * 3.14f / 8), clr, depthTest);
        }
        break;
        
        // Directional
    case 2:
        {
            const float cDirLightRange = 10.f;
            float3 endPoint = translate + cDirLightRange * lightDirection;
            float3 offset = rotate * float3(1.f, 0.f, 0.f);
            DebugDrawLine(translate, endPoint, clr, depthTest);
            DebugDrawLine(translate + offset, endPoint + offset, clr, depthTest);
            DebugDrawLine(translate - offset, endPoint - offset, clr, depthTest);
        }
        break;
    }
}

void GraphicsWorld::DebugDrawCamera(const float3x4 &t, float size, const Color &clr, bool depthTest)
{
    AABB aabb(float3(-size/2.f, -size/2.f, -size), float3(size/2.f, size/2.f, size));
    OBB obb = aabb.Transform(t);
    DebugDrawOBB(obb, clr, depthTest);
    
    float3 translate(0, 0, -size * 1.25f);
    AABB aabb2(translate + float3::FromScalar(-size/4.f), translate + float3::FromScalar(size/4.f));
    OBB obb2 = aabb2.Transform(t);
    DebugDrawOBB(obb2, clr, depthTest);
}

void GraphicsWorld::DebugDrawSoundSource(const float3 &soundPos, float soundInnerRadius, float soundOuterRadius, const Color & /*clr*/, bool depthTest)
{
    // Draw three concentric diamonds as a visual cue
    for (int i = 2; i < 5; ++i)
        DebugDrawSphere(soundPos, i/3.f, 24, i==2?1.0f:0.0f, i==3?1.0f:0.0f, i==4?1.0f:0.0f, depthTest);

    DebugDrawSphere(soundPos, soundInnerRadius, 384, 1, 0, 0, depthTest);
    DebugDrawSphere(soundPos, soundOuterRadius, 384, 0, 1, 0, depthTest);
}

void GraphicsWorld::StartViewTracking(Entity* entity)
{
    if (entity && entity->ParentScene() == scene_.Get())
        visibilityTrackedEntities_[EntityWeakPtr(entity)] = IsEntityVisible(entity);
}

/// Stop tracking an entity's visibility
void GraphicsWorld::StopViewTracking(Entity* entity)
{
    visibilityTrackedEntities_.Erase(EntityWeakPtr(entity));
}

}
