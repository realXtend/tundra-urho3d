// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IComponent.h"
#include "IAttribute.h"
#include "SceneFwd.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"
#include "CoreStringUtils.h"

namespace Tundra
{

typedef WeakPtr<Placeable> PlaceableWeakPtr;
typedef WeakPtr<Mesh> MeshWeakPtr;

/// Mesh entity animation controller
/** <table class="header">
    <tr>
    <td>
    <h2>AnimationController</h2>
    Mesh entity animation controller

    Needs to be told of a @ref Mesh "Mesh" component to be usable.

    <b>Exposes the following scriptable functions:</b>
    <ul>
    <li>"EnableAnimation": @copydoc EnableAnimation
    <li>"EnableExclusiveAnimation": @copydoc EnableExclusiveAnimation
    <li>"HasAnimationFinished": @copydoc HasAnimationFinished
    <li>"IsAnimationActive": @copydoc IsAnimationActive
    <li>"DisableAnimation": @copydoc DisableAnimation
    <li>"DisableAllAnimations": @copydoc DisableAllAnimations
    <li>"SetAnimationToEnd": @copydoc SetAnimationToEnd
    <li>"SetAnimationSpeed": @copydoc SetAnimationSpeed
    <li>"SetAnimationWeight": @copydoc SetAnimationWeight
    <li>"SetAnimationPriority": @copydoc SetAnimationPriority
    <li>"SetAnimationTimePosition": @copydoc SetAnimationTimePosition
    <li>"SetAnimationAutoStop": @copydoc SetAnimationAutoStop
    <li>"SetAnimationNumLoops": @copydoc SetAnimationNumLoops
    <li>"GetAvailableAnimations": @copydoc GetAvailableAnimations
    <li>"GetActiveAnimations": @copydoc GetActiveAnimations
    </ul>

    <b>Reacts on the following actions:</b>
    <ul>
    <li>"PlayAnim": Plays an animation. Usage: PlayAnim <name> [fadein] [exclusive]
    <li>"PlayLoopedAnim": Plays an animation in looped mode. Usage: PlayLoopedAnim <name> [fadein] [exclusive]
    <li>"PlayReverseAnim": Plays an animation reversed. Usage: PlayReverseAnim <name> [fadein] [exclusive]
    <li>"PlayAnimAutoStop": Plays an animation and autostops (fades out) when finished. Usage: PlayAnimAutoStop <name> [fadein] [exclusive]
    <li>"StopAnim": Stops an animation. Usage: StopAnim <name> [fadeout]
    <li>"StopAllAnims": Stops all animations. Usage: StopAllAnims [fadeout]
    <li>"SetAnimSpeed": Sets speed of a running animation. Use negative speed to play in reverse. Usage: SetAnimSpeed <name> <speed>
    <li>"SetAnimWeight": Sets blending weight of a running animation. May not be effective when animation is already fading in or out. Usage: SetAnimWeight <name> <weight>
    </ul>
    </td>
    </tr>

    Does not emit any actions.

    <b>Depends on the component Mesh</b>.
    </table> */
class URHO_MODULE_API AnimationController : public IComponent
{
    COMPONENT_NAME(AnimationController, 14)

public:
    /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit AnimationController(Urho3D::Context* context, Scene* scene);
    /// @endcond
    virtual ~AnimationController();

     /// Animation state attribute. Is a "freedata" field to store the current animation state.
    /** It is up to a logic script to change & interpret this, AnimationController does not change or read it by itself. */
    Attribute<String> animationState;

    Attribute<bool> drawDebug;

    /// Enumeration of animation phase
    enum AnimationPhase
    {
        PHASE_FADEIN = 0,
        PHASE_PLAY,
        PHASE_FADEOUT,
        PHASE_STOP,
        PHASE_FREE //in external control. for dynamiccomponent testing now
    };

    /// Structure for an ongoing animation
    struct Animation
    {
        /// Autostop at end (default false)
        bool auto_stop_;

        /// Time in milliseconds it takes to fade in/out an animation completely
        float fade_period_;
        
        /// Weight of an animation in animation blending, maximum 1.0
        float weight_;

        /// Weight adjust
        float weight_factor_;

        /// How an animation is sped up or slowed down, default 1.0 (original speed)
        float speed_factor_;

        /// loop animation through num_repeats times, or loop if zero
        uint num_repeats_;

        /// priority. high priority will reduce the weight of low priority animations, if exists on the same bone tracks
        bool high_priority_;
        
        /// current phase
        AnimationPhase phase_;

        Animation() :
            auto_stop_(false),
            fade_period_(0.0),
            weight_(0.0),
            weight_factor_(1.0),
            speed_factor_(1.0),
            num_repeats_(0),
            high_priority_(false),
            phase_(PHASE_STOP)
        {
        }
    };
    typedef HashMap<String, Animation> AnimationMap;

    /// Returns all running animations
    const AnimationMap& GetRunningAnimations() const { return animations_; }

    /// Auto-associate mesh component if not yet set
    void AutoSetMesh();
    
    /// Updates animation(s) by elapsed time
    void Update(float frametime);

    /// Draws the mesh skeleton
    void DrawSkeleton(float frametime);

    /// Enables animation with optional fade-in time
    /* @param name Animation name
       @param looped Is animation looped
       @param fadein Animation fadein time, 0 = instant
       @param highPriority Whether animation uses high-priority blending (overrides and reduces weight of low-priority animations)
       @return true if animation exists and could be enabled */
    bool EnableAnimation(const String& name, bool looped = true, float fadein = 0.0f, bool highPriority = false);

    /// Enables an exclusive animation. This means that other animations will start fading out with the fade-out time specified
    /* @param name Animation name
       @param looped Is animation looped
       @param fadein Animation fadein time, 0 = instant
       @param fadeout Other animations' fadeout time, 0 = instant
       @param highPriority Whether animation uses high-priority blending (overrides and reduces weight of low-priority animations)
       @return true if animation exists and could be enabled */
    bool EnableExclusiveAnimation(const String& name, bool looped, float fadein = 0.0f, float fadeout = 0.0f, bool highPriority = false);

    /// Checks whether non-looping animation has finished. If looping, returns always false
    /* @return true if animation finished */
    bool HasAnimationFinished(const String& name);

    /// Checks whether animation is active
    /** @param name Animation name
        @param check_fade_out if true, also fade-out (until totally faded) phase is interpreted as "active"
        @return true if animation active */
    bool IsAnimationActive(const String& name, bool checkFadeout = true);

    /// Disables animation with optional fade-out time
    /** @param name Animation name
        @param fadeout Animation fadeout time, 0 = instant
        @return true if animation exists and could be disabled */
    bool DisableAnimation(const String& name, float fadeout = 0.0f);

    /// Disables all animations with the same fadeout time
    /** @param fadeout Animation fadeout time */
    void DisableAllAnimations(float fadeout = 0.0f);

    /// Forwards animation to end, useful if animation is played in reverse
    /** @param name Animation name */
    void SetAnimationToEnd(const String& name);

    /// Sets relative speed of an active animation. Note that this speed will be forgotten if the animation is disabled
    /** @param name Animation name
        @param speedFactor Relative speed. 1 is default. Use negative speed to play in reverse
        @return true if successful (animation was currently playing) */
    bool SetAnimationSpeed(const String& name, float speedFactor);

    /// Changes weight of an active animation.
    /** @param name Animation name
        @param weight Animation weight (from 0 to 1)
        @return true if successful (animation was currently playing) */
    bool SetAnimationWeight(const String& name, float weight);
    
    /// Changes animation priority. Can lead to fun visual effects, but provided for completeness
    /** @param name Animation name
        @param highPriority High priority bit
        @return true if successful (animation was currently playing) */
    bool SetAnimationPriority(const String& name, bool highPriority);

    /// Sets time position of an active animation.
    /** @param name Animation name
        @param newPosition New time position
        @return true if successful (animation was currently playing) */
    bool SetAnimationTimePosition(const String& name, float newPosition);

    /// Sets length-relative time position of an active animation (ie. 0 = start, 1 = end)
    /** @param name Animation name
        @param newPosition New time position
        @return true if successful (animation was currently playing) */
    bool SetAnimationRelativeTimePosition(const String& name, float newPosition);
    
    /// Sets autostop on animation
    /** @param name Animation name
        @param enable Autostop flag
        @return true if successful */
    bool SetAnimationAutoStop(const String& name, bool enable);

    /// Sets number of times the animation is repeated
    /** @param name Animation name
        @param repeats Number of repeats (0 = repeat indefinitely)
        @return true if successful */
    bool SetAnimationNumLoops(const String& name, unsigned repeats);

    /// Get available animations
    StringList GetAvailableAnimations();
    
    /// Get active animations as a simple stringlist
    StringList GetActiveAnimations() const;
    
    /// Returns length of animation
    /** @param name Animation name
        @return length of animation in seconds, or 0 if no such animation */
    float GetAnimationLength(const String& name);
    
    /// Returns time position of animation
    /** @param name Animation name
        @return time position of animation in seconds, or 0 if not active */
    float GetAnimationTimePosition(const String& name);
    
    /// Returns relative time position of animation
    /** @param name Animation name
        @return time position of animation between 0 - 1, or 0 if not active */
    float GetAnimationRelativeTimePosition(const String& name);
    
    /// Implements the PlayAnim action
    void PlayAnim(const String &name, const String &fadein, const String &exclusive);
    /// Implements the PlayLoopedAnim action
    void PlayLoopedAnim(const String &name, const String &fadein, const String &exclusive);
    /// Implements the PlayReverseAnim action
    void PlayReverseAnim(const String &name, const String &fadein, const String &exclusive);
    /// Implements the PlayAnimAutoStop action
    void PlayAnimAutoStop(const String &name, const String &fadein, const String &exclusive);
    /// Implements the StopAnim action
    void StopAnim(const String &name, const String &fadeout);
    /// Implements the StopAllAnims action
    void StopAllAnims(const String &fadeout);
    /// Implements the SetAnimSpeed action
    void SetAnimSpeed(const String &name, const String &animspeed);
    /// Implements the SetAnimWeight action
    void SetAnimWeight(const String &name, const String &animweight);

    /// Emitted when a non-looping animation has finished
    Signal1<String> AnimationFinished;
    /// Emitted when a looping animation has completed a cycle
    Signal1<String> AnimationCycled;

private:
    /// Called when the parent entity has been set.
    void UpdateSignals();

    /// Called when component has been added or removed from the parent entity. Checks the existence of the Placeable component, and attaches this light to it.
    void OnComponentStructureChanged(IComponent*, AttributeChange::Type);

    void AttributesChanged() override;

    /// Gets animationstate 
    /** @param name Animation name
        @return animationstate, or null if not found */
    Urho3D::AnimationState* GetAnimationState(const String& name);

    /// Mesh component
    MeshWeakPtr mesh_;

    /// placeable component 
    PlaceableWeakPtr placeable_;

    /// World ptr
    GraphicsWorldWeakPtr world_;

    /// Map of animations
    AnimationMap animations_;

    HashMap<String, Urho3D::AnimationState*> animationStates_;
};

COMPONENT_TYPEDEFS(AnimationController)

}