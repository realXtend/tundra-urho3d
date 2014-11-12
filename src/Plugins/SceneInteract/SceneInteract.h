// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "SceneFwd.h"
#include "IAttribute.h"
#include "AttributeChangeType.h"
#include "IRenderer.h"
#include "Signals.h"

namespace Tundra
{

/// Transforms generic mouse and keyboard input events on scene entities to input-related entity actions and signals.
/** Performs a raycast to the mouse position each frame and executes entity actions depending current input.

    <b>Local</b> entity actions executed to the hit entity:
    <ul>
    <li>"MouseHoverIn" - Executed when mouse hover enters an entity.
    <div>No parameters.</div>
    <li>"MouseHover" - Executed when mouse hovers on an entity.
    <div>No parameters</div>
    <li>"MouseHoverOut" - Executed when mouse hover leaves an entity.
    <div></div>
    <li>"MousePress" - Executed when mouse button is clicked on an entity.
    <div>String parameters: (int)"mousebutton", (float,float,float)"x,y,z"</div>
    <li>"MouseRelease" - Executed when mouse button is released on an entity.
    <div>String parameters: (int)"mousebutton", (float,float,float)"x,y,z"</div>
    </ul> */
class SceneInteract : public IModule
{
    OBJECT(SceneInteract);

public:
    SceneInteract(Framework* owner);
    ~SceneInteract();

    /// Frame-based module update
    void Update(float frametime) override;
   /// Returns the latest raycast result to last known mouse cursor position in the currently active scene.
    /** @return Raycast result. */
    RayQueryResult* CurrentMouseRaycastResult() const;

    /// Returns the closest intersect point when raycasting from @c from to @c to.
    /** @param from Source position where raycast is executed.
        @param to Target position for the raycast.
        @param layerMask Layer mask @see OgreWorld::Raycast.
        @param maxDistance Max. distance to raycast @see OgreWorld::Raycast.
        @return Intersect world position. float3::nan is returned if nothing was hit, use float3::IsFinite() to check the result. */
    float3 RaycastClosestIntersect(const float3 &from, const float3 &to, unsigned layerMask = 0xffffffff, float maxDistance = 1000.0f) const;
    /// @overload
    float3 RaycastClosestIntersect(const float3 &from, const Vector<float3> &to, unsigned layerMask = 0xffffffff, float maxDistance = 1000.0f) const;

    /// Returns the furthest intersect point when raycasting from @c from to @c to.
    /** @param from Source position where raycast is executed.
        @param to Target position for the raycast.
        @param layerMask Layer mask @see OgreWorld::Raycast. 
        @return Intersect world position. float3::nan is returned if nothing was hit, use float3::IsFinite() to check the result. */
    float3 RaycastFurthestIntersect(const float3 &from, const float3 &to, unsigned layerMask = 0xffffffff) const;
    /// @overload
    float3 RaycastFurthestIntersect(const float3 &from, const Vector<float3> &to, unsigned layerMask = 0xffffffff) const;

    /// Emitted when mouse cursor moves on top of an entity.
    /** @param entity Hit entity.
        @param Possible mouse button held down during the move.
        @param result Raycast result data object. */
    Signal3<Entity*, int, RayQueryResult*> EntityMouseMove;

    /// Emitted when mouse was scrolled and raycast hit an entity.
    /** @param entity Hit entity.
        @param delta The difference in the mouse wheel position.
        @param result Raycast result data object. */
    Signal3<Entity*, int, RayQueryResult*> EntityMouseScroll;

    /// Emitted when scene was clicked and raycast hit an entity.
    /** @param entity Hit entity.
        @param Clicked mouse button
        @param result Raycast result data object. */
    Signal3<Entity*, int, RayQueryResult*> EntityClicked;

    /// Emitted when scene was clicked and raycast hit an entity.
    /** @param entity Hit entity.
        @param Released mouse button.
        @param result Raycast result data object. */
    Signal3<Entity*, int, RayQueryResult*> EntityClickReleased;

private:
    /// Performs raycast to last known mouse cursor position in the currently active scene.
    /** This function will only perform the raycast once per Tundra mainloop frame. */
    RayQueryResult* ExecuteRaycast();
    
    /// Handle mouse input events. \todo Refactor to use InputAPI once it exists
    void HandleMouseEvent(StringHash eventType, VariantMap& eventData);

    int lastX; ///< Last known mouse cursor's x position.
    int lastY; ///< Last known mouse cursor's y position.
    
    bool itemUnderMouse; ///< Was there widget under mouse in last known position.
    bool frameRaycasted; ///< Has raycast been already done for this frame.

    EntityWeakPtr lastHitEntity; ///< Last entity raycast has hit.
    mutable RayQueryResult lastRaycast; ///< Last raycast result.
    
};

}
