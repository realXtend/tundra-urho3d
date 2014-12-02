// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "Math/MathNamespace.h"
#include "IParticleAsset.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"

namespace Tundra
{

/// Represents a particle asset loaded from Ogre binary format.
/** \todo does not support following:
      Particle system attributes:
        cull_each
        billboard_type
        billboard_origin
        billboard_rotation_type
        common_direction
        common_up_vector
        renderer
        point_rendering
        accurate_facing
        iteration_interval
        nonvisible_update_timeout (timeout part is unsupported: if timeout == 0, always updates, otherwise does not update when not visible)

      Emitter attributes:
        position
        angle
        direction

        Multiple emitters inside single particle_system -block
        Most emitter types (supports only sphere and box emitter types)
        Linear Force Affector: force_application average (supports only 'add')
        DeflectorPlane affector
        DirectionRandomiser affector
        ColourFader2  affector  */
class URHO_MODULE_API OgreParticleAsset : public IParticleAsset
{
    OBJECT(OgreParticleAsset);

public:
    OgreParticleAsset(AssetAPI *owner, const String &type_, const String &name_);

    /// Load mesh from memory. IAsset override.
    bool DeserializeFromData(const u8 *data_, uint numBytes, bool allowAsynchronous) override;

    /// IAsset override.
    Vector<AssetReference> FindReferences() const override;
    /// IAsset override.
    void DependencyLoaded(AssetPtr dependee) override;
};

}
