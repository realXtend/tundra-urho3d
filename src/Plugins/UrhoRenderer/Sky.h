// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IComponent.h"
#include "UrhoModuleApi.h"
#include "IAttribute.h"
#include "Math/Quat.h"
#include "AssetReference.h"
#include "UrhoModuleFwd.h"
#include "AssetRefListener.h"

#include <Image.h>
#include <TextureCube.h>

namespace Tundra
{

/// Makes the entity a sky.
/** <table class="header">
    <tr>
    <td>
    <h2>Sky</h2>

    Makes the entity a sky.

    Registered by UrhoModule.

    <b>Attributes</b>:
    <ul>
    <li>AssetReference: materialRef
    <div> @copydoc materialRef </div>
    <li>Quat: orientation
    <div> @copydoc orientation </div>
    <li>float: distance
    <div> @copydoc distance </div>
    <li>bool : drawFirst
    <div> @copydoc drawFirst </div>
    </ul>
    </table> */
class URHO_MODULE_API Sky : public IComponent
{
    COMPONENT_NAME(Sky, 10)

public:
    /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit Sky(Urho3D::Context* context, Scene* scene);
    /// @endcond
    virtual ~Sky();

    /// Sky material reference.
    /** Material defines how the sky looks. Dy default and typically a material with cubic texture is used,
        but also f.ex. a simpler material can be used to paint the sky with solid color. */
    Attribute<AssetReference> materialRef;

    /// Sky texture references.
    Attribute<AssetReferenceList> textureRefs;

    /// Distance in world coordinates from the camera to each plane of the box.
    Attribute<float> distance;

    /// Optional parameter to specify the orientation of the box.
    Attribute<Quat> orientation;

    /// Defines is sky drawn first.
    Attribute<bool> drawFirst;

    /// Is the sky enabled.
    Attribute<bool> enabled;

    /// Updates skybox material or textures
    void Update();

private:
    /// Creates Urho scene node
    void CreateSkyboxNode();

    /// Called when the parent entity has been set.
    void UpdateSignals();

    /// Called when component has been added or removed from the parent entity. Checks the existence of the Placeable component, and attaches this light to it.
    void OnComponentStructureChanged(IComponent*, AttributeChange::Type);

    void AttributesChanged() override;

    void OnMaterialAssetLoaded(AssetPtr asset);
    void OnMaterialAssetFailed(IAssetTransfer *transfer, String error);

    void OnTextureAssetRefsChanged(const AssetReferenceList &tRefs);

    void OnTextureAssetFailed(uint index, IAssetTransfer* transfer, String error);

    void OnTextureAssetLoaded(uint index, AssetPtr asset);

    AssetRefListenerPtr materialAsset_;
    AssetRefListListenerPtr textureRefListListener_;

    /// The Urho3D node the skybox is attached to
    SharedPtr<Urho3D::Node> urhoNode_;

    /// Material. If material contains cube texture, use internal Urho skybox material, otherwise use this material as is.
    MaterialAssetPtr material_;
    
    GraphicsWorldWeakPtr world_;
};

}

