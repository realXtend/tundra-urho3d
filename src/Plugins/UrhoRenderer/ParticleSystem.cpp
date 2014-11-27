// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "ParticleSystem.h"
#include "GraphicsWorld.h"
#include "AttributeMetadata.h"
#include "Placeable.h"
#include "UrhoRenderer.h"
#include "Scene/Scene.h"
#include "LoggingFunctions.h"
#include "IParticleAsset.h"

#include <Engine/Scene/Node.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Graphics/ParticleEmitter.h>
#include <Engine/Graphics/ParticleEffect.h>
#include <Engine/Graphics/GraphicsDefs.h>
#include <Engine/Resource/ResourceCache.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Graphics/Technique.h>

namespace Tundra
{

ParticleSystem::ParticleSystem(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(particleRef, "Particle Ref", AssetReference("", "OgreParticle")),
    INIT_ATTRIBUTE_VALUE(castShadows, "Cast shadows", false),
    INIT_ATTRIBUTE_VALUE(enabled, "Enabled", true),
    INIT_ATTRIBUTE_VALUE(renderingDistance, "Rendering distance", 0.0f)
{
    if (scene)
        world_ = scene->Subsystem<GraphicsWorld>();

    static AttributeMetadata drawDistanceData("", "0", "10000");
    renderingDistance.SetMetadata(&drawDistanceData);

    ParentEntitySet.Connect(this, &ParticleSystem::UpdateSignals);
}

ParticleSystem::~ParticleSystem()
{
    if (world_.Expired())
    {
        if (particleEmitter_)
            LogError("ParticleSystem: World has expired, skipping uninitialization!");
        return;
    }

    DetachParticleSystem();

    if (particleEmitter_)
    {
        particleEmitter_.Reset();
        adjustmentNode_->Remove();
        adjustmentNode_.Reset();
    }
}

void ParticleSystem::UpdateSignals()
{
    if (!ViewEnabled())
        return;

    Entity* parent = ParentEntity();
    if (!parent)
        return;

    particleRefListener_ = new AssetRefListener();

    parent->ComponentAdded.Connect(this, &ParticleSystem::OnComponentStructureChanged);
    parent->ComponentRemoved.Connect(this, &ParticleSystem::OnComponentStructureChanged);

    if (parent->ParentScene())
        world_ = parent->ParentScene()->Subsystem<GraphicsWorld>();

    if (world_ && !particleEmitter_)
    {
        Urho3D::Scene* urhoScene = world_->UrhoScene();
        adjustmentNode_ = urhoScene->CreateChild("AdjustmentNode");
        
        // Make the entity & component links for identifying raycasts
        ///\todo Is this needed for particle effects?
        adjustmentNode_->SetVar(GraphicsWorld::entityLink, Variant(WeakPtr<RefCounted>(parent)));
        adjustmentNode_->SetVar(GraphicsWorld::componentLink, Variant(WeakPtr<RefCounted>(this)));

        particleEmitter_ = adjustmentNode_->CreateComponent<Urho3D::ParticleEmitter>();
        particleEmitter_->SetEnabled(false);

        // Connect ref listeners
        particleRefListener_->Loaded.Connect(this, &ParticleSystem::OnParticleAssetLoaded);
    }

    // Make sure we attach to the Placeable if exists.
    AttachParticleSystem();
}

void ParticleSystem::OnComponentStructureChanged(IComponent*, AttributeChange::Type)
{
    // No-op if attached to the same placeable already
    if (placeable_ == parentEntity->Component<Placeable>())
        return;

    AttachParticleSystem(); // Try to attach if placeable is present, otherwise detach
}

void ParticleSystem::AttributesChanged()
{
    if (!ViewEnabled() || !particleEmitter_)
        return;

    if (particleRef.ValueChanged() && particleRefListener_)
        particleRefListener_->HandleAssetRefChange(&particleRef);
    if (castShadows.ValueChanged())
        particleEmitter_->SetCastShadows(castShadows.Get());
    if (enabled.ValueChanged())
        particleEmitter_->SetEnabled(enabled.Get());
    if (renderingDistance.ValueChanged())
        particleEmitter_->SetDrawDistance(renderingDistance.Get());
}

void ParticleSystem::OnParticleAssetLoaded(AssetPtr asset)
{
    IParticleAsset *particleAsset = dynamic_cast<IParticleAsset*>(asset.Get());

    Urho3D::ParticleEffect* effect = particleAsset->UrhoParticleEffect();

    ///\todo Particles are now facing away from camera (or culled wrong side), so need to force fix culling.
    effect->GetMaterial()->SetCullMode(Urho3D::CULL_NONE);

    StringHash alphaPass = StringHash("alpha");
    ///\todo Need to force use of vertex color technique for colored particles. Remove once OgreMaterialProcessor can handle this.
    if (effect->GetColorFrames().Size() > 0 && effect->GetMaterial()->GetTechnique(0) &&
        effect->GetMaterial()->GetTechnique(0)->GetPass(alphaPass))
    {
        if (effect->GetMaterial()->GetTechnique(0)->GetPass(alphaPass)->GetBlendMode() == Urho3D::BLEND_ADD)
            effect->GetMaterial()->SetTechnique(0, GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::Technique>("Techniques/DiffVColAdd.xml"));
        else
            effect->GetMaterial()->SetTechnique(0, GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::Technique>("Techniques/DiffVColUnlitAlpha.xml"));
    }

    particleEmitter_->SetEffect(effect);

    AttachParticleSystem();
}

void ParticleSystem::AttachParticleSystem()
{
    if (!particleEmitter_ || world_.Expired())
        return;

    // Detach first, in case the original placeable no longer exists
    DetachParticleSystem();

    Entity *entity = ParentEntity();
    if (!entity)
        return;
    placeable_ = entity->Component<Placeable>();
    if (!placeable_)
        return;

    Urho3D::Node* placeableNode = placeable_->UrhoSceneNode();
    if (!placeableNode)
    {
        LogError("Can not attach ParticleSystem: placeable does not have an Urho3D scene node");
        return;
    }
  
    adjustmentNode_->SetParent(placeableNode);
    adjustmentNode_->SetPosition(Urho3D::Vector3(0, 0, 0));

    particleEmitter_->SetEnabled(enabled.Get());
}

void ParticleSystem::DetachParticleSystem()
{
     if (!particleEmitter_ || world_.Expired())
        return;

    if (placeable_)
    {
        Urho3D::Scene* urhoScene = world_->UrhoScene();
        // When removed from the placeable, attach to scene root to avoid being removed from scene
        adjustmentNode_->SetParent(urhoScene);
        placeable_.Reset();
        particleEmitter_->SetEnabled(false); // We should not render while detached
    }
}

}
