// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IComponent.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"
#include "Math/float3.h"
#include "Math/Quat.h"
#include "Geometry/OBB.h"
#include "Geometry/AABB.h"
#include "Math/Transform.h"
#include "AssetReference.h"
#include "AssetRefListener.h"

namespace Tundra
{

typedef WeakPtr<Placeable> PlaceableWeakPtr;

/// Mesh component
/** <table class="header">
    <tr>
    <td>
    <h2>Mesh</h2>
    Mesh component
    Needs to be attached to a placeable (aka scene node) to be visible. 

    Registered by UrhoRenderer plugin.

    <b>Attributes</b>:
    <ul>
    <li>Transform: nodeTransformation
    <div>@copydoc nodeTransformation</div>
    <li>AssetReference: meshRef
    <div>@copydoc meshRef</div>
    <li>AssetReference: skeletonRef
    <div>@copydoc skeletonRef</div>
    <li>AssetReferenceList: materialRefs
    <div>@copydoc materialRefs</div>
    <li>float: drawDistance
    <div>@copydoc drawDistance</div>
    <li>bool: castShadows
    <div>@copydoc castShadows</div>
    </ul>

    Does not emit any actions.

    <b>Depends on the component Placeable</b>.
    </table> */
class URHO_MODULE_API Mesh : public IComponent
{
    COMPONENT_NAME(Mesh, 17)

public:
    /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit Mesh(Urho3D::Context* context, Scene* scene);
    virtual ~Mesh();
    /// @endcond

    /// Mesh transformation in relation to its scene node
    Attribute<Transform> nodeTransformation;

    /// Asset reference to the mesh asset that should be displayed
    Attribute<AssetReference> meshRef;

    /// Skeleton asset reference.
    Attribute<AssetReference> skeletonRef;

    /// Mesh material asset reference list.
    Attribute<AssetReferenceList> materialRefs;

    /// Mesh draw distance, 0.0 = draw always (default)
    Attribute<float> drawDistance;

    /// Will the mesh cast shadows.
    Attribute<bool> castShadows;

    /// Should the mesh entity be created with instancing. Irrelevant in tundra-urho3d (depends automatically on geometry), but provided for network protocol compatibility
    Attribute<bool> useInstancing;

    /// Returns a bone scene node by name. If mesh is not skeletal, always returns null.
    Urho3D::Node* BoneNode(const String& boneName) const;

    /// Returns adjustment scene node (used for scaling/offset/orientation modifications)
    Urho3D::Node* AdjustmentSceneNode() const { return adjustmentNode_; }

    /// IComponent override, implemented to support old TXML with the "Mesh materials" attribute instead of "materialRefs"/"Material refs".
    /// @todo 2014-10-17 This can be removed at some point when enough time has passed.
    void DeserializeFrom(Urho3D::XMLElement& element, AttributeChange::Type change) override;

    /// Emitted before the mesh is about to be destroyed
    Signal0<void> MeshAboutToBeDestroyed;

    /// Signal is emitted when mesh has successfully loaded and applied to entity.
    Signal0<void> MeshChanged;

    /// Signal is emitted when material has successfully applied to sub mesh.
    Signal2<uint, const String&> MaterialChanged;

    /// Signal is emitted when skeleton has successfully applied to entity.
    Signal0<void> SkeletonChanged;

    /// Return the Urho mesh entity component.
    Urho3D::AnimatedModel* UrhoMesh() const;

private:
    /// Called when the parent entity has been set.
    void UpdateSignals();

    /// Called when component has been added or removed from the parent entity. Checks the existence of the Placeable component, and attaches this mesh entity to it.
    void OnComponentStructureChanged(IComponent*, AttributeChange::Type);

    /// Attaches mesh to placeable
    void AttachMesh();

    /// Detaches mesh from placeable
    void DetachMesh();

    /// React to attribute changes
    void AttributesChanged() override;

    /// Mesh asset has been loaded
    void OnMeshAssetLoaded(AssetPtr asset);

    /// Material asset refs changed.
    void OnMaterialAssetRefsChanged(const AssetReferenceList &materialRefs);

    /// Material asset load failed.
    void OnMaterialAssetFailed(int index, IAssetTransfer *transfer, String error);

    /// Material asset has been loaded.
    void OnMaterialAssetLoaded(int index, AssetPtr asset);

    /// Adjustment scene node (scaling/offset/orientation modifications)
    SharedPtr<Urho3D::Node> adjustmentNode_;
    /// Urho mesh component. Always an AnimatedModel; this does not hurt in case the model is non-skeletal instead
    SharedPtr<Urho3D::AnimatedModel> mesh_;

    /// Placeable component attached to.
    PlaceableWeakPtr placeable_;

    /// Graphics world ptr
    GraphicsWorldWeakPtr world_;

    /// Manages mesh asset requests.
    AssetRefListenerPtr meshRefListener_;

    /// Manager material asset requests.
    AssetRefListListenerPtr materialRefListListener_;
};

COMPONENT_TYPEDEFS(Mesh)

}

