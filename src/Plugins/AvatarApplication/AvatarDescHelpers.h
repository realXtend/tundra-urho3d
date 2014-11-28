// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreTypes.h"

#include "Math/float3.h"
#include "Math/Quat.h"
#include "AvatarApi.h"

namespace Tundra
{

/// Defines a transform for an avatar, attachment or bone
struct AVATAR_API AvatarTransform
{
    float3 position_;
    Quat orientation_;
    float3 scale_;
    
    AvatarTransform() : position_(float3::zero),
        orientation_(Quat::identity),
        scale_(1.0f, 1.0f, 1.0f)
    {
    }
};

/// Defines an appearance modifier, possibly under control of a master modifier through position mapping
struct AVATAR_API AppearanceModifier
{
    enum ModifierType
    {
        Undefined = 0,
        Morph,
        Bone
    };

    /// Modifier name
    String name_;
    /// Modifier type
    ModifierType type_;
    /// Manual state. If true, master modifiers have no effect
    bool manual_;
    /// Modifier influence value (0.0 - 1.0)
    float value_;

    /// Sum of data accumulated so far
    float sum_;
    /// Number of samples accumulated
    int samples_;

    void ResetAccumulation();
    void AccumulateValue(float value, bool use_average);
    
    AppearanceModifier(ModifierType type = Undefined) : 
        manual_(false),
        type_(type),
        value_(0.0)
    {
        ResetAccumulation();
    }
};

/// Defines a bone modifier
struct AVATAR_API BoneModifier
{
    /// Possible modes for a bone modification
    enum BoneModifierMode
    {
        /// Relative to the bone's initial transform
        Relative = 0,
        /// Absolute, overrides the bone's initial transform
        Absolute,
        /// Cumulative, adds to a previous modifier
        Cumulative
    };    
    
    /// Name of bone in avatar skeleton
    String bone_name_;
    /// Start transform
    AvatarTransform start_;
    /// End transform
    AvatarTransform end_;
    /// Mode of applying position modification
    BoneModifierMode position_mode_;
    /// Mode of applying rotation modification
    BoneModifierMode orientation_mode_;

    BoneModifier() : 
        position_mode_(Relative),
        orientation_mode_(Relative)
    {
    }
};

/// Defines a set of bone modifiers (also called a dynamic animation)
struct AVATAR_API BoneModifierSet : public AppearanceModifier
{
    /// Individual bone modifiers
    Vector<BoneModifier> modifiers_;
    
    BoneModifierSet() :
        AppearanceModifier(Bone)
    {
    }
};

/// Defines a morph modifier
struct AVATAR_API MorphModifier : public AppearanceModifier
{
    /// Name of morph animation
    String morph_name_;
    
    MorphModifier() :
        AppearanceModifier(Morph)
    {
    }
};

/// Describes a modifier driven by a master modifier
struct AVATAR_API SlaveModifier
{
    /// Master value accumulation mode
    enum AccumulationMode
    {
        Average = 0,
        Cumulative
    };
    /// Defines a point in master-slaver modifier value mapping
    struct ValueMapping
    {
        ValueMapping() :
            master_(0.0f),
            slave_(0.0f)
        {
        }

        float master_;
        float slave_;
        
        bool operator < (const ValueMapping &rhs) const
        {
            return (master_ < rhs.master_);
        }
    };

    float GetMappedValue(float master_value);
    
    /// Value accumulation mode
    AccumulationMode mode_;
    /// Value mapping table. If empty, identity mapping
    Vector<ValueMapping> mapping_;
    /// Name 
    String name_;
    /// Type
    AppearanceModifier::ModifierType type_;
    
    SlaveModifier() : mode_(Average), type_(AppearanceModifier::Undefined)
    {
    }
};

/// Defines a master modifier that controls several appearance (slave) modifiers
struct AVATAR_API MasterModifier
{
    /// Current position value (0.0 - 1.0)
    float value_;
    /// Name
    String name_;
    /// Category description
    String category_;
    /// Modifiers controlled 
    Vector<SlaveModifier> modifiers_;
};

typedef Vector<MasterModifier> MasterModifierVector;

/// Defines an animation for an avatar
struct AVATAR_API AnimationDefinition
{
    /// Most likely a UUID
    String id_;
    /// Identifying human-readable name, not mandatory and not used directly in code
    String name_;
    /// Actual animation name in the mesh/skeleton
    String animation_name_;
    /// Should play looped?
    bool looped_;
    /// Exclusive; override (stop) other animations
    bool exclusive_;
    /// Speed scaled with avatar movement speed?
    bool use_velocity_;
    /// Always restart animation when it starts playing?
    bool always_restart_;
    /// Blend-in period in seconds
    float fadein_;
    /// Blend-out period in seconds
    float fadeout_;
    /// Speed modification (1.0 original)
    float speedfactor_;
    /// Weight modification (1.0 full)
    float weightfactor_;
    
    AnimationDefinition() :
        looped_(true),
        exclusive_(false),
        use_velocity_(false),
        always_restart_(false),
        fadein_(0.0),
        fadeout_(0.0),
        speedfactor_(1.0),
        weightfactor_(1.0)
    {
    }
};

/// Defines an attachment for an avatar
struct AVATAR_API AvatarAttachment
{
    /// Name of attachment
    String name_;
    /// Mesh asset reference
    String mesh_;
    /// Materials used by the attachment mesh
    Vector<String> materials_;
    /// Whether skeleton should be linked (for animations)
    bool link_skeleton_;
    /// Transform 
    AvatarTransform transform_;
    /// Category of attachment
    String category_;
    /// Base bone of attachment. Empty if attached directly to avatar scene node
    String bone_name_;
    /// Polygons indices to hide from avatar when using this attachment
    Vector<uint> vertices_to_hide_;
};

}

