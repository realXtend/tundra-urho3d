// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"
#include "IComponent.h"
#include "Attribute.h"
#include "Math/Color.h"
#include "Math/float3.h"
#include "Math/Quat.h"
#include "AssetReference.h"
#include "AssetFwd.h"

namespace Tundra
{

/// Makes the entity a water plane.
/** <table class="header">
    <tr>
    <td>
    <h2>WaterPlane</h2>

    Water plane component creates a cubic water plane. Inside the water cube scene fog is overridden by underwater fog properties.
    Despite the cubic nature, water plane is visible for the outside viewer only as a plane.

    Registered by UrhoRenderer plugin.

    <b>Attributes</b>:
    <ul>
    <li>int: xSize
    <div> @copydoc xSize </div>
    <li>int: ySize
    <div> @copydoc ySize </div>
    <li>int: depth
    <div> @copydoc depth </div>
    <li>float3: position
    <div> @copydoc position </div>
    <li>Quat: rotation
    <div> @copydoc rotation </div>
    <li>float: scaleUfactor
    <div> @copydoc scaleUfactor </div>
    <li>float: scaleVfactor
    <div> @copydoc </div>
    <li>int: xSegments
    <div> @copydoc xSegments </div>
    <li>int: ySegments
    <div> @copydoc ySegments </div>
    <li>AssetReference: materialRef
    <div> @copydoc materialRef </div>
    <li>Color: fogColor
    <div> @copydoc fogColor </div>
    <li>float : fogStartDistance
    <div> @copydoc </div>
    <li>float : fogEndDistance
    <div> @copydoc fogEndDistance </div>
    <li>enum: fogMode
    <div> @copydoc fogMode </div>
    </ul>

    <b>Exposes the following scriptable functions:</b>
    <ul>
    <li>"IsCameraInsideWaterCube": @copydoc IsCameraInsideWaterCube
    <li>"IsPointInsideWaterCube": @copydoc IsPointInsideWaterCube
    <li>"GetPointOnPlane": @copydoc GetPointOnPlane
    <li>"GetDistanceToWaterPlane": @copydoc GetDistanceToWaterPlane
    <li>"IsTopOrBelowWaterPlane": @copydoc IsTopOrBelowWaterPlane
    </ul>

    Does not emit any actions.

    <b>Can use @ref Placeable "Placeable" component</b>. If entity has the position defined by the Placeable component then it also specifies the position
    in the world space where this water plane is by default placed at. Note component does not need Placeable component.
    </table> */
class URHO_MODULE_API WaterPlane : public IComponent
{
    COMPONENT_NAME(WaterPlane, 12);

public:
    /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit WaterPlane(Urho3D::Context* context, Scene* scene);
    /// @endcond
    virtual ~WaterPlane();

    /// Water plane size in x-axis.
    Attribute<int> xSize;

    /// Water plane size in y-axis.
    Attribute<int> ySize;

    /// Defines how much below the surface water fog color is used. Meaning this attribute defines how "deep" is our ocean/pond. 
    Attribute<int> depth;

    /// Position of the water plane.
    /** Used if no Placeable present in the same entity. */
    Attribute<float3> position;

    /// Orientation of the water plane.
    Attribute<Quat> rotation;

    /// U Scale, factor which defines how many times the texture should be repeated in the u direction.
    /** @note The current default value  is so small 0.002, so it does not show up correctly in EC editor. */
    Attribute<float> scaleUfactor;

    /// V Scale, factor which defines how many times the texture should be repeated in the v direction.
    /** @note The current default value is so small 0.002, so it does not show up correctly in EC editor. */
    Attribute<float> scaleVfactor;

    /// The number of segments to the plane in the x direction.
    Attribute<int> xSegments;

    /// The number of segments to the plane in the y direction.
    Attribute<int> ySegments;

    /// The material used for the water plane.
    Attribute<AssetReference> materialRef;

    /// Color of underwater fog.
    Attribute<Color> fogColor;

    /// Underwater fog start distance (meters), @ref Fog::Linear "Linear" mode only.
    Attribute<float> fogStartDistance;

    /// Underwater fog end distance (meters), @ref Fog::Linear "Linear" mode only.
    Attribute<float> fogEndDistance;

    /// Underwater fog mode, defines how the fog density increases.
    /** @see Fog::FogMode 
        @todo Only linear fog mode supported. */
    Attribute<int> fogMode;

    /// The density of the fog in @ref Fog::Exponentially "Exponentially" or @ref Fog::ExponentiallySquare "ExponentiallySquare" mode, as a value between 0 and 1. The default is 0.001.
    Attribute<float> fogExpDensity;

public:
    /// Returns true if camera is inside of water cube.
    bool IsCameraInsideWaterCube();

    /// Returns true if point is inside of water cube.
    /** @param point in world coordinate system. */
    bool IsPointInsideWaterCube(const float3& point) const;

    /// Returns the point on the water plane in world space that lies on top of the given world space coordinate.
    /** @param point The point in world space to get the corresponding map point (in world space) for. */
    float3 GetPointOnPlane(const float3 &point) const;

    /// Returns distance from plane (note, here is assumption that point is top/or below plane), distance in here is distance from water plane surface.
    /** @param point is point in world coordinate system. */
    float GetDistanceToWaterPlane(const float3& point) const;

    /// Returns true if given point is top or below water plane.
    /** @param point is in world coordinate system. */
    bool IsTopOrBelowWaterPlane(const float3& point) const;

private:
    /// Sets up the waterplane
    void SetupWaterPlane();

    void OnMaterialAssetLoaded(AssetPtr asset);

    /// Called if parent entity has set.
    void Create();

    void Update(float frametime);

    /// Called when the parent entity has been set.
    void UpdateSignals();

    /// Called when component has been added or removed from the parent entity. Checks the existence of the Placeable component, and attaches this light to it.
    void OnComponentStructureChanged(IComponent*, AttributeChange::Type);

    void AttributesChanged() override;

    /// Sets placeable component
    /** set a null placeable to detach the light, otherwise will attach
        @param placeable placeable component */
    void SetPlaceable(const ComponentPtr &placeable);

    /// Attach to placeable
    void Attach();

    /// Detach from placeable
    void Detach();

    /// Finds out that is EC_Placeable component connected to same entity where water plane component is placed. 
    /** @returns component pointer to Placeable component. */
    ComponentPtr FindPlaceable() const;

    /// Changes water plane position.
    /** This function should be called only if the parent entity of this component has no Placeable component.
        @note Uses attribute @p position to for water plane defining water plane position  */
    void SetPosition();

    /// Changes water plane rotation
    /** This function should be called only if the parent entity of this component has no Placeable component.
        @note Uses attribute @p rotation to for water plane defining water plane rotation */
    void SetOrientation();

    void SetUnderwaterFog();
    void RestoreFog();
    void UpdateMaterial();

    /// World ptr
    GraphicsWorldWeakPtr world_;

    /// Urho3D node
    SharedPtr<Urho3D::Node> adjustmentNode_;

    /// Static plane for the water
    SharedPtr<Urho3D::StaticModel> waterPlane_;

    /// placeable component 
    WeakPtr<Placeable> placeable_;

    /// Used for caching whether or not the camera is inside this water plane.
    /// If it was last frame, but isn't anymore, the original scene fog is restored (if existent in the first place).
    bool cameraInsideWaterCube_;

    /// Asset ref listener for material
    AssetRefListenerPtr materialAsset_;
};

COMPONENT_TYPEDEFS(WaterPlane);

}
