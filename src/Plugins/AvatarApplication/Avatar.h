// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IComponent.h"
#include "AssetReference.h"
#include "AvatarApi.h"
#include "AssetFwd.h"
#include "AvatarDescAsset.h"

namespace Tundra
{

struct BoneModifier;
class AvatarDescAsset;
typedef SharedPtr<AvatarDescAsset> AvatarDescAssetPtr;

/// Avatar component.
/** <table class="header">
    <tr>
    <td>
    <h2>Avatar</h2>

    @note This component no longer generates the required EC_Mesh, EC_Placeable and EC_AnimationController
    components to an entity to display an avatar.

    @todo Write better description!

    Registered by AvatarApplication.

    <b>Attributes</b>:
    <ul>
    <li>AssetReference: appearanceRef
    <div> @copydoc appearanceRef</div>
    </ul>

    <b>Exposes the following scriptable functions:</b>
    <ul>
    </ul>

    <b>Reacts on the following actions:</b>
    <ul>
    </ul>

    Does not emit any actions.

    <b>Depends on the components @ref EC_Mesh "Mesh" and @ref EC_Placeable "Placeable".</b>
    </table> */
class AVATAR_API Avatar : public IComponent
{
    COMPONENT_NAME(Avatar, 1)

public:
    /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit Avatar(Urho3D::Context* context, Scene* scene);
    /// @endcond
    virtual ~Avatar();

    /// Asset id for the avatar appearance file that will be used to generate the visible avatar.
    Attribute<AssetReference> appearanceRef;

    /// Refresh appearance completely
    void SetupAppearance();
    /// Refresh dynamic parts of the appearance (morphs, bone modifiers)
    void SetupDynamicAppearance();
    /// Return the avatar description asset, if set
    AvatarDescAssetPtr AvatarDesc() const;
    /// Returns a generic property from the avatar description, or an empty string if not found
    String AvatarProperty(const String& name) const;

private:
    /// Avatar asset loaded.
    void OnAvatarAppearanceLoaded(AssetPtr asset);
    /// Avatar asset failed to load.
    void OnAvatarAppearanceFailed(IAssetTransfer* transfer, String reason);

    /// Called when some of the attributes has been changed.
    void AttributesChanged();
    /// Adjust avatar's height offset
    void AdjustHeightOffset();
    /// Rebuild mesh and set materials
    void SetupMeshAndMaterials();
    /// Rebuild attachment meshes
    void SetupAttachments();
    /// Lookup absolute asset reference
    String LookupAsset(const String& ref);

    /// Ref listener for the avatar asset
    AssetRefListenerPtr avatarAssetListener_;
    /// Last set avatar asset
    WeakPtr<AvatarDescAsset> avatarAsset_;
    /// Current attachment entities
    Vector<EntityWeakPtr> attachmentEntities_;
};
COMPONENT_TYPEDEFS(Avatar);

}
