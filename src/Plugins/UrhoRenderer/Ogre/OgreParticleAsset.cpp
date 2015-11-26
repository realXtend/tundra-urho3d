// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "AssetAPI.h"
#include "AssetCache.h"
#include <Urho3D/Core/Profiler.h>
#include "LoggingFunctions.h"
#include "OgreParticleAsset.h"
#include "OgreParticleSystemDefines.h"
#include "IMaterialAsset.h"
#include "OgreMeshAsset.h"
#include "OgreMeshDefines.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Graphics/ParticleEffect.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/Material.h>

namespace Tundra
{

Urho3D::EmitterType UrhoEmitterTypeFromOgre(const String &ogreType)
{
    Urho3D::EmitterType type = Urho3D::EMITTER_SPHERE;
    if (StringHash(ogreType) == Ogre::ParticleSystem::Emitter::Box)
        type = Urho3D::EMITTER_BOX;

    return type;
}

SharedPtr<Urho3D::ParticleEffect> ParticleEffectFromTemplate(SharedPtr<Urho3D::ParticleEffect> effect, const Tundra::Ogre::ParticleSystemBlock *effectBlock)
{
    // Particle system attributes
    effect->SetNumParticles(effectBlock->IntValue(Ogre::ParticleSystem::Effect::Quota, 10000));
    Urho3D::Vector2 size(effectBlock->FloatValue(Ogre::ParticleSystem::Effect::ParticleWidth, 100), effectBlock->FloatValue(Ogre::ParticleSystem::Effect::ParticleHeight, 100));
    effect->SetMinParticleSize(size);
    effect->SetMaxParticleSize(size);
    effect->SetSorted(effectBlock->BooleanValue(Ogre::ParticleSystem::Effect::Sorted, false));
    effect->SetRelative(effectBlock->BooleanValue(Ogre::ParticleSystem::Effect::LocalSpace, false));
    effect->SetUpdateInvisible(effectBlock->IntValue(Ogre::ParticleSystem::Effect::NonvisibleUpdateTimeout, 0) == 0);

    // Emitter attributes
    if (effectBlock->NumEmitters() > 0)
    {
        Tundra::Ogre::ParticleSystemBlock* emitterBlock = effectBlock->Emitter(0);
        effect->SetEmitterType(UrhoEmitterTypeFromOgre(emitterBlock->id));
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

    // Affectors
    for(uint i = 0 ; i < effectBlock->NumAffectors(); ++i)
    {
        Tundra::Ogre::ParticleSystemBlock *affectorBlock = effectBlock->Affector(i);

        // Linear force
        if (StringHash(affectorBlock->id) == Ogre::ParticleSystem::Affector::LinearForce)
        {
            if (affectorBlock->Has(Ogre::ParticleSystem::Affector::ForceVector))
                effect->SetConstantForce(affectorBlock->Vector3Value(Ogre::ParticleSystem::Affector::ForceVector, Urho3D::Vector3::ZERO));
        }

        // Scaler
        if (StringHash(affectorBlock->id) == Ogre::ParticleSystem::Affector::Scaler)
        {
            if (affectorBlock->Has(Ogre::ParticleSystem::Affector::Rate))
                effect->SetSizeAdd(affectorBlock->FloatValue(Ogre::ParticleSystem::Affector::Rate, 0));
        }

        // Rotator
        if (StringHash(affectorBlock->id) == Ogre::ParticleSystem::Affector::Rotator)
        {
            effect->SetMinRotationSpeed(affectorBlock->FloatValue(Ogre::ParticleSystem::Affector::RotationSpeedRangeStart, 0));
            effect->SetMaxRotationSpeed(affectorBlock->FloatValue(Ogre::ParticleSystem::Affector::RotationSpeedRangeEnd, 0));
            effect->SetMinRotation(affectorBlock->FloatValue(Ogre::ParticleSystem::Affector::RotationRangeStart, 0));
            effect->SetMaxRotation(affectorBlock->FloatValue(Ogre::ParticleSystem::Affector::RotationRangeEnd, 0));
        }

        // ColourInterpolator, overrides ColourRangeStart, ColourRangeEnd
        if (StringHash(affectorBlock->id) == Ogre::ParticleSystem::Affector::ColourInterpolator)
        {
            Vector<Urho3D::ColorFrame> colorFrames;
            for (int j = 0 ; j < 6; ++j)
            {
                StringHash color = StringHash(Ogre::ParticleSystem::Affector::Color + String(j));
                StringHash time = StringHash(Ogre::ParticleSystem::Affector::Time + String(j));
                if (!affectorBlock->Has(color) || !affectorBlock->Has(time))
                    break;
                colorFrames.Push(Urho3D::ColorFrame(affectorBlock->ColorValue(color, Urho3D::Color::WHITE), affectorBlock->FloatValue(time, 0)));
            }
            if (colorFrames.Size() > 0)
                effect->SetColorFrames(colorFrames);
        }

        // ColourFader, overrides ColourRangeEnd
        if (StringHash(affectorBlock->id) == Ogre::ParticleSystem::Affector::ColourFader)
        {
            Urho3D::Color startColor = Urho3D::Color::WHITE;
            if (effect->GetColorFrames().Size() > 0)
                startColor = effect->GetColorFrames()[0].color_;

            Vector<Urho3D::ColorFrame> colorFrames;
            colorFrames.Push(Urho3D::ColorFrame(startColor, 0));
            float4 affectorColor = float4(affectorBlock->FloatValue(Ogre::ParticleSystem::Affector::Red, 0),
                affectorBlock->FloatValue(Ogre::ParticleSystem::Affector::Green, 0),
                affectorBlock->FloatValue(Ogre::ParticleSystem::Affector::Blue, 0),
                affectorBlock->FloatValue(Ogre::ParticleSystem::Affector::Alpha, 0));

            float4 endColor = startColor.ToVector4() + affectorColor * effect->GetMinTimeToLive();
            Urho3D::Color c = Urho3D::Color(endColor.x, endColor.y, endColor.z, endColor.w);
            colorFrames.Push(Urho3D::ColorFrame(c, effect->GetMinTimeToLive()));
            effect->SetColorFrames(colorFrames);
        }
    }

    return effect;
}

OgreParticleAsset::OgreParticleAsset(AssetAPI *owner, const String &type_, const String &name_) :
    IParticleAsset(owner, type_, name_)
{
}

bool OgreParticleAsset::DeserializeFromData(const u8 *data_, uint numBytes, bool /*allowAsynchronous*/)
{
    URHO3D_PROFILE(OgreParticleAsset_LoadFromFileInMemory);

    /// Force an unload of previous data first.
    Unload();

    LogDebug("Reading Ogre particle script '" + Name() + "'");

    Ogre::ParticleSystemParser parser;
    if (parser.Parse((const char*)data_, numBytes))
    {
        for(uint i = 0 ; i < parser.templates.Size(); ++i)
        {
            SharedPtr<Urho3D::ParticleEffect> effect = ParticleEffectFromTemplate(SharedPtr<Urho3D::ParticleEffect>(new Urho3D::ParticleEffect(GetContext())), parser.templates[i]);
            particleEffects_.Push(effect);
            if (parser.templates[i]->Has(Ogre::ParticleSystem::Effect::Material))
            {
                String materialRef = parser.templates[i]->StringValue(Ogre::ParticleSystem::Effect::Material, "");
                materials_.Push(Urho3D::MakePair(0, AssetReference(assetAPI->ResolveAssetRef(Name(), materialRef), "OgreMaterial")));
            }
            else
                effect->SetMaterial(GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::Material>("Materials/DefaultGrey.xml"));

            assetAPI->AssetLoadCompleted(Name());
        }
        return true;
    }

    return false;
}

void OgreParticleAsset::DependencyLoaded(AssetPtr dependee)
{
    IMaterialAsset *material = dynamic_cast<IMaterialAsset*>(dependee.Get());
    if (material)
    {
        LogDebug("Material loaded: " + material->Name());
        bool found = false;
        for (uint i = 0; i < materials_.Size(); ++i)
        {
            /// \todo Is this ref compare reliable?
            if (!materials_[i].second_.ref.Compare(material->Name(), false) && i < particleEffects_.Size())
            {
                particleEffects_[i]->SetMaterial(material->UrhoMaterial());
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

