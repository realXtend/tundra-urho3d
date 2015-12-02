// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreTypes.h"
#include "UrhoRendererApi.h"

#include <Urho3D/Container/HashMap.h>
#include <Urho3D/Math/StringHash.h>
#include <Urho3D/Math/Color.h>

/// @cond PRIVATE

namespace Tundra
{

namespace Ogre
{

namespace ParticleSystem
{
    namespace Block
    {
        const StringHash ParticleSystem             = "particle_system";
        const StringHash Emitter                    = "emitter";
        const StringHash Affector                   = "affector";
        const StringHash DefaultParameters          = "default_params";
    }

    namespace Effect
    {
        const StringHash Quota                      = "quota";
        const StringHash Material                   = "material";
        const StringHash ParticleWidth              = "particle_width";
        const StringHash ParticleHeight             = "particle_height";
        const StringHash CullEach                   = "cull_each";
        const StringHash Renderer                   = "renderer";
        const StringHash Sorted                     = "sorted";
        const StringHash LocalSpace                 = "local_space";
        const StringHash BillboardType              = "billboard_type";
        const StringHash BillboardOrigin            = "billboard_origin";
        const StringHash BillboardRotationType      = "billboard_rotation_type";
        const StringHash CommonDirection            = "common_direction";
        const StringHash CommonUpVector             = "common_up_vector";
        const StringHash PointRendering             = "point_rendering";
        const StringHash AccurateFacing             = "accurate_facing";
        const StringHash IterationInterval          = "iteration_interval";
        const StringHash NonvisibleUpdateTimeout    = "nonvisible_update_timeout";
    }

    namespace Emitter
    {
        const StringHash Point                      = "point";
        const StringHash Box                        = "box";
        const StringHash Cylinder                   = "cylinder";
        const StringHash Ellipsoid                  = "ellipsoid";
        const StringHash HollowEllipsoid            = "hollowellipsoid";
        const StringHash Ring                       = "ring";

        const StringHash Angle                      = "angle";
        const StringHash Colour                     = "colour";
        const StringHash ColourRangeStart           = "colour_range_start";
        const StringHash ColourRangeEnd             = "colour_range_end";
        const StringHash Direction                  = "direction";
        const StringHash EmissionRate               = "emission_rate";
        const StringHash Position                   = "position";
        const StringHash Velocity                   = "velocity";
        const StringHash VelocityMin                = "velocity_min";
        const StringHash VelocityMax                = "velocity_max";
        const StringHash TimeToLive                 = "time_to_live";
        const StringHash TimeToLiveMin              = "time_to_live_min";
        const StringHash TimeToLiveMax              = "time_to_live_max";
        const StringHash Duration                   = "duration";
        const StringHash DurationMin                = "duration_min";
        const StringHash DurationMax                = "duration_max";
        const StringHash RepeatDelay                = "repeat_delay";
        const StringHash RepeatDelayMin             = "repeat_delay_min";
        const StringHash RepeatDelayMax             = "repeat_delay_max";
    }

    namespace Affector
    {
        const StringHash LinearForce                = "LinearForce";
        const StringHash ForceVector                = "force_vector";
        const StringHash ForceApplication           = "force_application";

        const StringHash Scaler                     = "Scaler";
        const StringHash Rate                       = "rate";

        const StringHash Rotator                    = "Rotator";
        const StringHash RotationSpeedRangeStart    = "rotation_speed_range_start";
        const StringHash RotationSpeedRangeEnd      = "rotation_speed_range_end";
        const StringHash RotationRangeStart         = "rotation_range_start";
        const StringHash RotationRangeEnd           = "rotation_range_end";

        const StringHash ColourInterpolator         = "ColourInterpolator";
        const String Time                           = "time";
        const String Color                          = "colour";

        const StringHash ColourFader                = "ColourFader";
        const StringHash Red                        = "red";
        const StringHash Green                      = "green";
        const StringHash Blue                       = "blue";
        const StringHash Alpha                      = "alpha";
    }
}

/// Particle section
enum ParticleSystemPart
{
    PSP_Unsupported,
    PSP_ParticleSystem,
    PSP_Emitter,
    PSP_Affector,
    PSP_DefaultParameters
};

static inline String ParticleSystemPartToString(ParticleSystemPart part)
{
    switch(part)
    {
        case PSP_ParticleSystem:        return "particle_system";
        case PSP_Emitter:               return "emitter";
        case PSP_Affector:              return "affector";
    }
    return "Invalid MaterialPart enum " + String(static_cast<int>(part));
}

/// Material properties
typedef HashMap<StringHash, Vector<String> > ParticleSystemProperties;
typedef HashMap<StringHash, String> ParticleSystemPropertyNames;

/// Material block
struct URHORENDERER_API ParticleSystemBlock
{
    /** Block name, ref or identifier eg,
        particle_system <name>
        emitter <type>   */
    String id;

    /// Blocks part identifier
    ParticleSystemPart part;

    /// Properties
    ParticleSystemProperties properties;
    ParticleSystemPropertyNames propertyNames;

    /// Block indexes
    int emitter;
    int affector;

    /// Child blocks
    Vector<ParticleSystemBlock*> blocks;

    /// parent block
    ParticleSystemBlock *parent;

    ParticleSystemBlock(ParticleSystemBlock *parent_ = 0, ParticleSystemPart part_ = PSP_Unsupported, int e = -1, int a = -1);
    ~ParticleSystemBlock();

    /// Available in MP_Material (MaterialParser::root)
    uint NumEmitters() const;
    ParticleSystemBlock *Emitter(uint index = 0) const;

    uint NumAffectors() const;
    ParticleSystemBlock *Affector(uint index = 0) const;

    /// Number of children of type @c part.
    uint NumChildren(ParticleSystemPart part) const;

    /// Number of @c name properties
    uint Num(const StringHash &name) const;

    /// If @c name property exists
    bool Has(const StringHash &name) const;

    /// Return @c name property as a string for @c index. If not defined @c defaultValue is returned.
    String StringValue(const StringHash &name, const String &defaultValue, uint index = 0) const;

    /// Return @c name property as a string for @c index. If not defined a empty vector is returned.
    /** All values are trimmed separately and whitespace is removed between values. */
    StringVector StringVectorValue(const StringHash &name, uint index = 0) const;

    /// Return @c name property as a VectorN for @c index. If not defined or not enough components @c defaultValue is returned.
    Urho3D::Vector2 Vector2Value(const StringHash &name, const Urho3D::Vector2 &defaultValue, uint index = 0) const;
    Urho3D::Vector3 Vector3Value(const StringHash &name, const Urho3D::Vector3 &defaultValue, uint index = 0) const;
    Urho3D::Vector4 Vector4Value(const StringHash &name, const Urho3D::Vector4 &defaultValue, uint index = 0) const;

    /// Return @c name property as a Color for @c index. If not defined or valid color @c defaultValue is returned.
    Urho3D::Color ColorValue(const StringHash &name, const Urho3D::Color &defaultValue, uint index = 0) const;

    /// Return @c name property as a boolean for @c index. If not defined or valid boolean @c defaultValue is returned.
    bool BooleanValue(const StringHash &name, bool defaultValue, uint index = 0) const;

    /// Return @c name property as a int for @c index. If not defined or valid int @c defaultValue is returned.
    int IntValue(const StringHash &name, int defaultValue, uint index = 0) const;

    /// Return @c name property as a float for @c index. If not defined or valid float @c defaultValue is returned.
    float FloatValue(const StringHash &name, float defaultValue, uint index = 0) const;

    /** @todo Specialized value getters for param_named for "param_named <param_name> float4 1 2 3 4" etc.
        where <param_name> can be provided and 'float4' etc. indentifiers are chopped from the beginning. */

    /// Returns if this block is supported.
    /** If false no properties have been parsed for the block scope. */
    bool IsSupported();

    /// Dump material to stdout.
    void Dump(bool recursive = true, uint indentation = 0);
};

/// ParticleSystem parser
class URHORENDERER_API ParticleSystemParser
{
public:
    ParticleSystemParser();
    ~ParticleSystemParser();

    /// Root block
    /** Can be accessed after Parse to query material data. */
    Vector<ParticleSystemBlock*> templates;

    /// Parses script from input data.
    /** @param Returns success. If false you can query Error() for a printable message. */
    bool Parse(const char *data_, uint lenght_);

    /// Returns error that occurred while parsing,
    String Error() const;

private:
    bool ProcessLine();
    
    void Advance();
    void SkipBlock();

    ParticleSystemBlock *root;

    /// Data
    String data;
    String line;

    uint pos;
    uint lineNum;
    
    /// State
    struct State
    {
        ParticleSystemBlock *block;

        int emitter;
        int affector;

        String error;

        State() : block(0), emitter(-1), affector(-1) {}
    };
    State state;
};

}

}

/// @endcond
