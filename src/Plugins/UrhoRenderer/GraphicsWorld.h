// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreDefines.h"
#include "CoreTypes.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"
#include "SceneFwd.h"
#include "Scene/Scene.h"
#include "IRenderer.h"
#include "Math/Color.h"
#include "Math/Point.h"
#include "Geometry/Ray.h"
#include "Signals.h"

#include <Rect.h>
#include <HashSet.h>
#include <Object.h>

namespace Tundra
{

/// Contains the graphical representation of a scene, ie. the Urho Scene
class URHO_MODULE_API GraphicsWorld : public Object
{
    OBJECT(GraphicsWorld);

public:
    /// Called by the UrhoRenderer upon the creation of a new scene
    GraphicsWorld(UrhoRenderer* owner, Scene* scene);
    /// Fully destroys the Urho scene
    virtual ~GraphicsWorld();

    /// The default color used as ambient light.
    static Color DefaultSceneAmbientLightColor();

    /// Sets scene fog to default ineffective settings
    void SetDefaultSceneFog();

    /// Does a raycast into the world from screen coordinates, using specific selection layer(s)
    /** @note The coordinates are screen positions, not viewport positions [0,1].
        @param x Horizontal screen position for the origin of the ray
        @param y Vertical screen position for the origin of the ray
        @param layerMask Which selection layer(s) to use (bitmask)
        @return Raycast result structure */
    RayQueryResult Raycast(int x, int y, unsigned layerMask);
    RayQueryResult Raycast(const Point &point, unsigned layerMask) { return Raycast(point.x, point.y, layerMask);} /**< @overload @param point Screen point. */
    /// @overload
    /** Does a raycast into the world from screen coordinates, using all selection layers */
    RayQueryResult Raycast(int x, int y);
    RayQueryResult Raycast(const Point &point) { return Raycast(point.x, point.y);} /**< @overload @param point Screen point. */
    /// @overload
    /** Does raycast into the world using a ray in world space coordinates. */
    RayQueryResult Raycast(const Ray& ray, unsigned layerMask);
    /// @overload
    /** Does a raycast into the world with specified layers and maximum distance */
    RayQueryResult Raycast(int x, int y, unsigned layerMask, float maxDistance);
    /// @overload
    /** Does a raycast into the world with specified screen point, layers and maximum distance. */
    RayQueryResult Raycast(const Point &point, unsigned layerMask, float maxDistance) { return Raycast(point.x, point.y, layerMask, maxDistance);}
    /// @overload
    /** Does a raycast into the world from screen coordinates, using all selection layers and a maximum distance */
    RayQueryResult Raycast(int x, int y, float maxDistance);
    /// @overload
    /** Does a raycast into the world with specified screen point, using all selection layers and a maximum distance */
    RayQueryResult Raycast(const Point &point, float maxDistance) { return Raycast(point.x, point.y, maxDistance);}
    /// @overload
    /** Does raycast into the world using a ray in world space coordinates and a maximum distance */
    RayQueryResult Raycast(const Ray& ray, unsigned layerMask, float maxDistance);
    
    /// Does a raycast into the world from screen coordinates using specific selection layer(s), and returns all results
    /** @note The coordinates are screen positions, not viewport positions [0,1].
        @param x Horizontal screen position for the origin of the ray
        @param y Vertical screen position for the origin of the ray
        @param layerMask Which selection layer(s) to use (bitmask)
        @return List of raycast result structure, empty if no hits. */
    RayQueryResultVector RaycastAll(int x, int y, unsigned layerMask);
    RayQueryResultVector RaycastAll(const Point &point, unsigned layerMask) { return RaycastAll(point.x, point.y, layerMask);} /**< @overload @param point Screen point. */
    /// @overload
    /** Does a raycast into the world from screen coordinates, using all selection layers, and returns all results */
    RayQueryResultVector RaycastAll(int x, int y);
    RayQueryResultVector RaycastAll(const Point &point) { return RaycastAll(point.x, point.y);} /**< @overload @param point Screen point. */
    /// @overload
    /** Does raycast into the world using a ray in world space coordinates, and returns all results */
    RayQueryResultVector RaycastAll(const Ray& ray, unsigned layerMask);
    /// @overload
    /** Does a raycast into the world with specified layers and maximum distance, and returns all results */
    RayQueryResultVector RaycastAll(int x, int y, unsigned layerMask, float maxDistance);
    /// @overload
    /** Does a raycast into the world with specified screen point, layers and maximum distance and returns all results. */
    RayQueryResultVector RaycastAll(const Point &point, unsigned layerMask, float maxDistance) { return RaycastAll(point.x, point.y, layerMask, maxDistance);}
    /// @overload
    /** Does a raycast into the world from screen coordinates, using all selection layers and a maximum distance, and returns all results */
    RayQueryResultVector RaycastAll(int x, int y, float maxDistance);
    /// @overload
    /** Does a raycast into the world with specified screen point, using all selection layers and a maximum distance, and returns all results */
    RayQueryResultVector RaycastAll(const Point &point, float maxDistance) { return RaycastAll(point.x, point.y, maxDistance);}
    /// @overload
    /** Does raycast into the world using a ray in world space coordinates and a maximum distance, and returns all results */
    RayQueryResultVector RaycastAll(const Ray& ray, unsigned layerMask, float maxDistance);
    
    /// Does a frustum query to the world from viewport coordinates.
    /** @param viewRect The query rectangle in 2d window coords.
        @return List of entities within the frustrum. */
    EntityVector FrustumQuery(const Urho3D::IntRect &viewRect) const;

    /// Returns whether a single entity is visible in the currently active camera
    bool IsEntityVisible(Entity* entity) const;
    
    /// Returns visible entities in the currently active camera
    EntityVector VisibleEntities() const;
    
    /// Returns whether the currently active camera is in this scene
    bool IsActive() const;
    
    /// Returns the Renderer instance
    UrhoRenderer* Renderer() const { return renderer_; }

    /// Returns the Urho3D engine scene
    Urho3D::Scene* UrhoScene() const { return urhoScene_; }

    /// Returns the Zone used for ambient light and fog settings.
    Urho3D::Zone* UrhoZone() const;

    /// Returns the parent Tundra scene
    ScenePtr ParentScene() const { return scene_.Lock(); }

    /// Renders an axis-aligned bounding box.
    void DebugDrawAABB(const AABB &aabb, const Color &clr, bool depthTest = true);
    void DebugDrawAABB(const AABB &aabb, float r, float g, float b, bool depthTest = true) { DebugDrawAABB(aabb, Color(r, g, b), depthTest); } /**< @overload */
    /// Renders an arbitrarily oriented bounding box.
    void DebugDrawOBB(const OBB &obb, const Color &clr, bool depthTest = true);
    void DebugDrawOBB(const OBB &obb, float r, float g, float b, bool depthTest = true) { DebugDrawOBB(obb, Color(r, g, b), depthTest); } /**< @overload */
    /// Renders a line.
    void DebugDrawLine(const float3 &start, const float3 &end, const Color &clr, bool depthTest = true);
    void DebugDrawLine(const float3 &start, const float3 &end, float r, float g, float b, bool depthTest = true){ DebugDrawLine(start, end, Color(r, g, b), depthTest); } /**< @overload */
    /// Renders a plane.
    void DebugDrawPlane(const Plane &plane, const Color &clr, const float3 &refPoint = float3::zero,
        float uSpacing = 1.f, float vSpacing = 1.f,  int uSegments = 10, int vSegments = 10, bool depthTest = true);
    void DebugDrawPlane(const Plane &plane, float r, float g, float b, const float3 &refPoint = float3::zero,
        float uSpacing = 1.f, float vSpacing = 1.f, int uSegments = 10, int vSegments = 10, bool depthTest = true) { DebugDrawPlane(plane, Color(r, g, b), refPoint, uSpacing, vSpacing, uSegments, vSegments, depthTest); } /**< @overload */
    /// Renders a line segment.
    void DebugDrawLineSegment(const LineSegment &l, const Color &clr, bool depthTest = true);
    void DebugDrawLineSegment(const LineSegment &l, float r, float g, float b, bool depthTest = true) { DebugDrawLineSegment(l, Color(r, g, b), depthTest); } /**< @overload */
    /// Renders a transformation of an object.
    void DebugDrawTransform(const Transform &t, float axisLength, float boxSize, const Color &clr, bool depthTest = true);
    void DebugDrawTransform(const Transform &t, float axisLength, float boxSize, float r, float g, float b, bool depthTest = true) { DebugDrawTransform(t, axisLength, boxSize, Color(r, g, b), depthTest); } /**< @overload */
    /// Renders a transformation of an object.
    void DebugDrawFloat3x4(const float3x4 &t, float axisLength, float boxSize, const Color &clr, bool depthTest = true);
    void DebugDrawFloat3x4(const float3x4 &t, float axisLength, float boxSize, float r, float g, float b, bool depthTest = true) { DebugDrawFloat3x4(t, axisLength, boxSize, Color(r, g, b), depthTest); } /**< @overload */
    /// Renders a transform's local X, Y & Z axes in world space, with scaling
    void DebugDrawAxes(const float3x4 &t, bool depthTest = true);
    /// Renders a debug representation of a light.
    /** @param transform Transform of the light. The scale is ignored.
        @param lightType 0=point, 1=spot, 2=directional
        @param range Range of the light (point and spot lights only)
        @param spotAngle Spotlight cone outer angle in degrees (spot lights only) */
    void DebugDrawLight(const float3x4 &t, int lightType, float range, float spotAngle, const Color &clr, bool depthTest = true);
    void DebugDrawLight(const float3x4 &t, int lightType, float range, float spotAngle, float r, float g, float b, bool depthTest = true) { DebugDrawLight(t, lightType, range, spotAngle, Color(r, g, b), depthTest); } /**< @overload */
    /// Renders a hollow circle.
    /// @param numSubdivisions The number of edges to subdivide the circle into. This value must be at least 3.
    void DebugDrawCircle(const Circle &c, int numSubdivisions, const Color &clr, bool depthTest = true);
    void DebugDrawCircle(const Circle &c, int numSubdivisions, float r, float g, float b, bool depthTest = true) { DebugDrawCircle(c, numSubdivisions, Color(r, g, b), depthTest); } /**< @overload */
    /// Renders a simple box-like debug camera.
    void DebugDrawCamera(const float3x4 &t, float size, const Color &clr, bool depthTest = true);
    void DebugDrawCamera(const float3x4 &t, float size, float r, float g, float b, bool depthTest = true) { DebugDrawCamera(t, size, Color(r, g, b), depthTest); } /**< @overload */
    /// Renders a visualization for a spatial Sound object.
    void DebugDrawSoundSource(const float3 &soundPos, float soundInnerRadius, float soundOuterRadius, const Color &clr, bool depthTest = true);
    void DebugDrawSoundSource(const float3 &soundPos, float soundInnerRadius, float soundOuterRadius, float r, float g, float b, bool depthTest = true) { DebugDrawSoundSource(soundPos, soundInnerRadius, soundOuterRadius, Color(r, g, b), depthTest); } /**< @overload */
    /// Renders a sphere as geosphere.
    void DebugDrawSphere(const float3& center, float radius, int vertices, const Color &clr, bool depthTest = true);
    void DebugDrawSphere(const float3& center, float radius, int vertices, float r, float g, float b, bool depthTest = true) { DebugDrawSphere(center, radius, vertices, Color(r, g, b), depthTest); } /**< @overload */

    /// Start tracking an entity's visibility within this scene, using any camera(s)
    /** After this, connect either to the EntityEnterView and EntityLeaveView signals,
    or the entity's EnterView & LeaveView signals, to be notified of the visibility change(s). */
    void StartViewTracking(Entity* entity);
    /// Stop tracking an entity's visibility
    void StopViewTracking(Entity* entity);

    /// An entity has entered the view
    Signal1<Entity*> EntityEnterView;

    /// An entity has left the view
    Signal1<Entity*> EntityLeaveView;

    /// Static userdata identifier for linking Urho scene nodes to Tundra entities for raycasting
    static StringHash entityLink;

    /// Static userdata identifier for linking Urho scene nodes to Tundra components for raycasting
    static StringHash componentLink;

private:
    /// Handle frame update. Used for entity visibility tracking
    void OnUpdated(float timeStep);

    /// Do the actual raycast.
    void RaycastInternal(const Ray& ray, unsigned layerMask, float maxDistance, bool getAllResults);

    /// Returns the currently active camera component, if it belongs to this scene. Else return null
    Camera* VerifyCurrentSceneCameraComponent() const;
    
    /// Framework
    Framework* framework_;
    
    /// Owner/Parent renderer
    UrhoRenderer* renderer_;
    
    /// Parent scene
    SceneWeakPtr scene_;
    
    /// Urho3D scene
    SharedPtr<Urho3D::Scene> urhoScene_;
    
    /// Visible entities during this frame. Acquired from the active camera
    HashSet<EntityWeakPtr> visibleEntities_;

    /// Entities that are being tracked for visiblity changes and their last stored visibility status.
    HashMap<EntityWeakPtr, bool> visibilityTrackedEntities_;
    
    /// Current raycast results
    Vector<RayQueryResult> rayHits_;
};

}
