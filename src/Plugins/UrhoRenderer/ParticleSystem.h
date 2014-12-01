// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IComponent.h"
#include "IAttribute.h"
#include "SceneFwd.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"
#include "AssetReference.h"
#include "AssetRefListener.h"

namespace Tundra
{

/// Particle system.
/** <table class="header">
    <tr>
    <td>
    <h2>ParticleSystem</h2>

    <b>Attributes</b>:
    <ul>
    <li>AssetReference: particleRef
    <div> @copydoc particleRef </div>
    <li>bool: castShadows
    <div> @copydoc castShadows </div>
    <li>bool: enabled
    <div> @copydoc </div>
    <li>float: renderingDistance
    <div> @copydoc renderingDistance</div>
    </ul>

    <b>Reacts on the following actions:</b>
    <ul>
    <li>"SoftStopParticleSystem": Disables emitters on a particle system, but lets the existing particles run.
    Usage: SoftStopParticleSystem [systemName]. If systemName is empty, all the particle emitters in the particle asset will be stopped.
    </td>
    </tr>

    Does not emit any actions.

    <b>Depends on the component @ref Placeable "Placeable".</b>
    </table> */
class URHO_MODULE_API ParticleSystem : public IComponent
{
    COMPONENT_NAME(ParticleSystem, 27);

public:
    /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit ParticleSystem(Urho3D::Context* context, Scene* scene);
    /// @endcond
    ~ParticleSystem();

    /// Particle asset reference
    Attribute<AssetReference> particleRef;

    /// Does particles cast shadows.
    Attribute<bool> castShadows;

    /// Are the particle systems enabled.
    /** If true, will automatically create all particle systems from the asset.</div> */
    Attribute<bool> enabled;

    /// Particles rendering distance.
    Attribute<float> renderingDistance;

    /// Attach particle system to placeable
    void AttachParticleSystem();

    /// Detach particle system from placeable
    void DetachParticleSystem();

    /// Stop particle system emitter (soft stop.)
    /** \todo Implement */
    void SoftStopParticleSystem();

    /// Returns adjustment scene node (used for scaling/offset/orientation modifications)
    Urho3D::Node* AdjustmentSceneNode() const { return adjustmentNode_; }
    
private:
     /// Called when the parent entity has been set.
    void UpdateSignals();

    /// Called when component has been added or removed from the parent entity. Checks the existence of the Placeable component, and attaches this component to it.
    void OnComponentStructureChanged(IComponent*, AttributeChange::Type);

    void AttributesChanged() override;

    /// Sets placeable component
    /** set a null placeable to detach the particle system, otherwise will attach
        @param placeable placeable component */
    void SetPlaceable(const ComponentPtr &placeable);

    void OnParticleAssetLoaded(AssetPtr asset);
    void OnParticleAssetFailed(IAssetTransfer* transfer, String reason);
    void EntitySet();

    /// Adjustment scene node (scaling/offset/orientation modifications)
    SharedPtr<Urho3D::Node> adjustmentNode_;

    /// Urho3D particle emitter component
    Vector<Urho3D::ParticleEmitter*> particleEmitters_;

    /// placeable component 
    WeakPtr<Placeable> placeable_;

    /// World ptr
    GraphicsWorldWeakPtr world_;

    /// Asset ref listener for the particle asset
    AssetRefListenerPtr particleRefListener_;
};

COMPONENT_TYPEDEFS(ParticleSystem)
}

