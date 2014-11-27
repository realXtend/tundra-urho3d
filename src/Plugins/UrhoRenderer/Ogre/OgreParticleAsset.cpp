// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Renderer.h"
#include "AssetAPI.h"
#include "AssetCache.h"
#include "Profiler.h"
#include "LoggingFunctions.h"
#include "OgreParticleAsset.h"
#include "OgreParticleSystemDefines.h"
#include "IMaterialAsset.h"
#include "OgreMeshAsset.h"
#include "OgreMeshDefines.h"

#include <Engine/Core/StringUtils.h>
#include <Engine/Graphics/ParticleEffect.h>
#include <Engine/Resource/ResourceCache.h>
#include <Engine/Graphics/Material.h>

namespace Tundra
{

Urho3D::EmitterType UrhoEmitterTypeFromOgre(const String &ogreType)
{
    Urho3D::EmitterType type = Urho3D::EMITTER_SPHERE;
    if (StringHash(ogreType) == Ogre::ParticleSystem::Emitter::Box)
        type = Urho3D::EMITTER_BOX;

    return type;
}

SharedPtr<Urho3D::ParticleEffect> ParticleEffectFromTemplate(SharedPtr<Urho3D::ParticleEffect> effect, const Ogre::ParticleSystemParser &parser)
{
    Tundra::Ogre::ParticleSystemBlock* effectBlock = parser.root;

    effect->SetNumParticles(effectBlock->IntValue(Ogre::ParticleSystem::Effect::Quota, 10000));
    Urho3D::Vector2 size(effectBlock->FloatValue(Ogre::ParticleSystem::Effect::ParticleWidth, 100), effectBlock->FloatValue(Ogre::ParticleSystem::Effect::ParticleHeight, 100));
    effect->SetMinParticleSize(size);
    effect->SetMaxParticleSize(size);
    effect->SetSorted(effectBlock->BooleanValue(Ogre::ParticleSystem::Effect::Sorted, false));
    effect->SetRelative(effectBlock->BooleanValue(Ogre::ParticleSystem::Effect::LocalSpace, false));
    effect->SetUpdateInvisible(effectBlock->IntValue(Ogre::ParticleSystem::Effect::NonvisibleUpdateTimeout, 0) == 0);
    if (effectBlock->NumEmitters() > 0)
    {
        Tundra::Ogre::ParticleSystemBlock* emitterBlock = effectBlock->Emitter(0);
        effect->SetEmitterType(UrhoEmitterTypeFromOgre(emitterBlock->id));
        //effect->SetMinRotation(emitterBlock->FloatValue(Ogre::ParticleSystem::Emitter::Angle
        Vector<Urho3D::ColorFrame> colorFrames;
        if (emitterBlock->Has(Ogre::ParticleSystem::Emitter::Colour))
            colorFrames.Push(Urho3D::ColorFrame(emitterBlock->ColorValue(Ogre::ParticleSystem::Emitter::Colour, Urho3D::Color::WHITE)));
        else
        {
            colorFrames.Push(Urho3D::ColorFrame(emitterBlock->ColorValue(Ogre::ParticleSystem::Emitter::ColourRangeStart, Urho3D::Color::WHITE), 0));
            colorFrames.Push(Urho3D::ColorFrame(emitterBlock->ColorValue(Ogre::ParticleSystem::Emitter::ColourRangeEnd, Urho3D::Color::WHITE), 3));
        }
        
        effect->SetColorFrames(colorFrames);
        
        float emissionRate = emitterBlock->FloatValue(Ogre::ParticleSystem::Emitter::EmissionRate, 10);
        effect->SetMaxEmissionRate(emissionRate);
        effect->SetMinEmissionRate(emissionRate);

        float velocity = emitterBlock->FloatValue(Ogre::ParticleSystem::Emitter::Velocity, 1);
        effect->SetMinVelocity(emitterBlock->FloatValue(Ogre::ParticleSystem::Emitter::VelocityMin, velocity));
        effect->SetMaxVelocity(emitterBlock->FloatValue(Ogre::ParticleSystem::Emitter::VelocityMax, velocity));

        float timeToLive = emitterBlock->FloatValue(Ogre::ParticleSystem::Emitter::TimeToLive, 5);
        effect->SetMinTimeToLive(emitterBlock->FloatValue(Ogre::ParticleSystem::Emitter::TimeToLiveMin, timeToLive));
        effect->SetMaxTimeToLive(emitterBlock->FloatValue(Ogre::ParticleSystem::Emitter::TimeToLiveMax, timeToLive));

        float duration = 0;
        if (emitterBlock->Has(Ogre::ParticleSystem::Emitter::Duration))
            duration = emitterBlock->FloatValue(Ogre::ParticleSystem::Emitter::Duration, duration);
        else
            duration = emitterBlock->FloatValue(Ogre::ParticleSystem::Emitter::DurationMin, duration);
        effect->SetActiveTime(duration);

        float repeatDelay = 0;
        if (emitterBlock->Has(Ogre::ParticleSystem::Emitter::RepeatDelay))
            repeatDelay = emitterBlock->FloatValue(Ogre::ParticleSystem::Emitter::RepeatDelay, duration);
        else
            repeatDelay = emitterBlock->FloatValue(Ogre::ParticleSystem::Emitter::RepeatDelayMin, duration);
        effect->SetInactiveTime(repeatDelay);
    }

    ///\todo Ogre Affectors


    return effect;
}

OgreParticleAsset::OgreParticleAsset(AssetAPI *owner, const String &type_, const String &name_) :
    IParticleAsset(owner, type_, name_)
{
}

bool OgreParticleAsset::DeserializeFromData(const u8 *data_, uint numBytes, bool /*allowAsynchronous*/)
{
    PROFILE(OgreParticleAsset_LoadFromFileInMemory);

    /// Force an unload of previous data first.
    Unload();

    LogDebug("Reading Ogre particle script '" + Name() + "'");

    Ogre::ParticleSystemParser parser;
    if (parser.Parse((const char*)data_, numBytes))
    {
        ///\todo Handle multiple particle effects in single file.
        particleEffect_ = ParticleEffectFromTemplate(SharedPtr<Urho3D::ParticleEffect>(new Urho3D::ParticleEffect(GetContext())), parser);
        if (parser.root->Has(Ogre::ParticleSystem::Effect::Material))
        {
            String materialRef = parser.root->StringValue(Ogre::ParticleSystem::Effect::Material, "");
            materials_.Push(Urho3D::MakePair(0, AssetReference(assetAPI->ResolveAssetRef(Name(), materialRef), "OgreMaterial")));
        }
        else
            particleEffect_->SetMaterial(GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::Material>("Materials/DefaultGrey.xml"));

        assetAPI->AssetLoadCompleted(Name());
        return true;
    }

    return false;
}

void OgreParticleAsset::DependencyLoaded(AssetPtr dependee)
{
    IMaterialAsset *material = dynamic_cast<IMaterialAsset*>(dependee.Get());
    if (material && particleEffect_ != nullptr)
    {
        LogDebug("Material loaded: " + material->Name());
        bool found = false;
        for (uint i = 0; i < materials_.Size(); ++i)
        {
            /// \todo Is this ref compare reliable?
            if (!materials_[i].second_.ref.Compare(material->Name(), false))
            {
                particleEffect_->SetMaterial(material->UrhoMaterial());
                found = true;
            }
        }

        if (!found)
            LogWarning("OgreParticleAsset::DependencyLoaded: material '" + material->Name() + "' was not found as ref in any of the particle systems in '" + Name() + "'");
    }

    LoadCompleted();
}

Vector<AssetReference> OgreParticleAsset::FindReferences() const
{
    Vector<AssetReference> ret;
    for (uint i = 0; i < materials_.Size(); ++i)
        ret.Push(materials_[i].second_);
    return ret;
}

}

