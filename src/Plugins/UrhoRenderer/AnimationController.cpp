// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "AnimationController.h"
#include "GraphicsWorld.h"
#include "AttributeMetadata.h"
#include "Placeable.h"
#include "Mesh.h"
#include "UrhoRenderer.h"
#include "Scene/Scene.h"
#include "LoggingFunctions.h"
#include "FrameAPI.h"
#include "Framework.h"

#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationState.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Core/StringUtils.h>

namespace Tundra
{

AnimationController::AnimationController(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(animationState, "Animation state", ""),
    INIT_ATTRIBUTE_VALUE(drawDebug, "Draw debug", false)
{
    ParentEntitySet.Connect(this, &AnimationController::UpdateSignals);
}

AnimationController::~AnimationController()
{
}

void AnimationController::UpdateSignals()
{
    // If scene is not view-enabled, no further action
    if (!ViewEnabled())
        return;
    Entity* parent = ParentEntity();
    if (!parent)
        return;

    framework->Frame()->Updated.Connect(this, &AnimationController::Update);

    parent->ComponentAdded.Connect(this, &AnimationController::OnComponentStructureChanged);
    parent->ComponentRemoved.Connect(this, &AnimationController::OnComponentStructureChanged);

    if (parent->ParentScene())
        world_ = parent->ParentScene()->Subsystem<GraphicsWorld>();
}

void AnimationController::OnComponentStructureChanged(IComponent*, AttributeChange::Type)
{
    Entity *parent = ParentEntity();
    if (!parent)
        return;

    mesh_ = parent->Component<Mesh>();
}

void AnimationController::AttributesChanged()
{

}

Urho3D::Animation* AnimationController::AnimationByName(const String& name)
{
    /// \todo Now only handles animations in the Ogre skeleton asset. Enable use of animation assets
    return mesh_ ? mesh_->AnimationByName(name) : nullptr;
}

void AnimationController::Update(float frametime)
{
    if (!mesh_)
        return;

    Urho3D::AnimatedModel* model = mesh_.Get()->UrhoMesh();
    if (!model)
        return;

    for(auto i = animations_.Begin(); i != animations_.End();)
    {
        // Check expiration (model change?)
        if (!i->second_.animationState_)
            i = animations_.Erase(i);
        else
        {
            Urho3D::AnimationState* animstate = i->second_.animationState_;

            switch(i->second_.phase_)
            {
            case FadeInPhase:
                // If period is infinitely fast, skip to full weight & PLAY status
                if (i->second_.fade_period_ == 0.0f)
                {
                    i->second_.weight_ = 1.0f;
                    i->second_.phase_ = PlayPhase;
                }   
                else
                {
                    i->second_.weight_ += (1.0f / i->second_.fade_period_) * frametime;
                    if (i->second_.weight_ >= 1.0f)
                    {
                        i->second_.weight_ = 1.0f;
                        i->second_.phase_ = PlayPhase;
                    }
                }
                break;
    
            case PlayPhase:
                if (i->second_.auto_stop_ || i->second_.num_repeats_ != 1)
                {
                    if ((i->second_.speed_factor_ >= 0.f && animstate->GetTime() >= animstate->GetLength()) ||
                        (i->second_.speed_factor_ < 0.f && animstate->GetTime() <= 0.f))
                    {
                        if (i->second_.num_repeats_ != 1)
                        {
                            if (i->second_.num_repeats_ > 1)
                                i->second_.num_repeats_--;
    
                            float rewindpos = i->second_.speed_factor_ >= 0.f ? (animstate->GetTime() - animstate->GetLength()) : animstate->GetLength();
                            animstate->SetTime(rewindpos);
                        }
                        else
                        {
                            i->second_.phase_ = FadeOutPhase;
                        }
                    }
                }
                break;
    
            case FadeOutPhase:
                // If period is infinitely fast, skip to disabled status immediately
                if (i->second_.fade_period_ == 0.0f)
                {
                    i->second_.weight_ = 0.0f;
                    i->second_.phase_ = StopPhase;
                }
                else
                {
                    i->second_.weight_ -= (1.0f / i->second_.fade_period_) * frametime;
                    if (i->second_.weight_ <= 0.0f)
                    {
                        i->second_.weight_ = 0.0f;
                        i->second_.phase_ = StopPhase;
                    }
                }
                break;
            }
    
            // Set weight & step the animation forward
            if (i->second_.phase_ != StopPhase)
            {
                float advance = i->second_.speed_factor_ * frametime;
                float new_weight = i->second_.weight_ * i->second_.weight_factor_;
                
                bool cycled = false;
                float oldtimepos = animstate->GetTime();
                float animlength = animstate->GetLength();
                
                if (new_weight != animstate->GetWeight())
                    animstate->SetWeight((float)i->second_.weight_ * i->second_.weight_factor_);
                if (advance != 0.0f)
                    animstate->AddTime((float)(i->second_.speed_factor_ * frametime));
                
                // Check if we should fire an "animation finished" signal
                float newtimepos = animstate->GetTime();
                if (advance > 0.0f)
                {
                    if (!animstate->IsLooped())
                    {
                        if ((oldtimepos < animlength) && (newtimepos >= animlength))
                            cycled = true;
                    }
                    else
                    {
                        if (newtimepos < oldtimepos)
                            cycled = true;
                    }
                }
                else
                {
                    if (!animstate->IsLooped())
                    {
                        if ((oldtimepos > 0.0f) && (newtimepos == 0.0f))
                            cycled = true;
                    }
                    else
                    {
                        if (newtimepos > oldtimepos)
                            cycled = true;
                    }
                }
                
                if (cycled)
                {
                    if (animstate->IsLooped())
                        AnimationCycled.Emit(i->first_);
                    else
                        AnimationFinished.Emit(i->first_);
                }

                ++i;
            }
            else
            {
                // If stopped, disable & remove this animation from list
                model->RemoveAnimationState(i->second_.animationState_);
                i = animations_.Erase(i);
            }
        }
    }
}

void AnimationController::DrawSkeleton()
{
    if (!world_ || !mesh_ || !mesh_->UrhoMesh())
        return;

    world_->UrhoScene()->GetComponent<Urho3D::DebugRenderer>()->AddSkeleton(mesh_->UrhoMesh()->GetSkeleton(), Urho3D::Color::WHITE, false);
}

bool AnimationController::EnableAnimation(const String& name, bool looped, float fadein, bool high_priority)
{
    // See if we already have this animation
    auto i = animations_.Find(name);
    if (i != animations_.End())
    {
        if (!i->second_.animationState_)
            return false;

        i->second_.animationState_->SetLooped(looped);
        i->second_.phase_ = FadeInPhase;
        i->second_.num_repeats_ = (looped ? 0: 1);
        i->second_.fade_period_ = fadein;
        i->second_.high_priority_ = high_priority;
        // If animation is nonlooped and has already reached end, rewind to beginning
        if ((!looped) && (i->second_.speed_factor_ > 0.0f))
        {
            if (i->second_.animationState_->GetTime() >= i->second_.animationState_->GetLength())
                i->second_.animationState_->SetTime(0.0f);
        }
        return true;
    }
    
    // Start new animation from zero weight & speed factor 1, also reset time position
    Urho3D::Animation* anim = AnimationByName(name);
    if (!anim)
        return false;
    Urho3D::AnimationState* animstate = mesh_->UrhoMesh()->AddAnimationState(anim);
    if (!animstate)
        return false;

    animstate->SetTime(0.0f);
    animstate->SetLooped(looped);
    
    Animation newanim;
    newanim.animationState_ = animstate;
    newanim.phase_ = FadeInPhase;
    newanim.num_repeats_ = (looped ? 0: 1); // if looped, repeat 0 times (loop indefinetly) otherwise repeat one time.
    newanim.fade_period_ = fadein;
    newanim.high_priority_ = high_priority;

    animations_[name] = newanim;

    return true;
}

bool AnimationController::EnableExclusiveAnimation(const String& name, bool looped, float fadein, float fadeout, bool high_priority)
{
    // Disable all other active animations
    auto i = animations_.Begin();
    while(i != animations_.End())
    {
        const String& other_name = i->first_;
        if (other_name.Compare(name, false) != 0)
        {
            i->second_.phase_ = FadeOutPhase;
            i->second_.fade_period_ = fadeout;
        }
        ++i;
    }

    // Then enable this
    return EnableAnimation(name, looped, fadein, high_priority);
}

bool AnimationController::HasAnimationFinished(const String& name) const
{
    auto i = animations_.Find(name);
    if (i != animations_.End())
    {
        if (!i->second_.animationState_)
            return true;

        if ((!i->second_.animationState_->IsLooped()) && ((i->second_.speed_factor_ >= 0.f && i->second_.animationState_->GetTime() >= i->second_.animationState_->GetLength()) ||
            (i->second_.speed_factor_ < 0.f && i->second_.animationState_->GetTime() <= 0.f)))
            return true;
        else
            return false;
    }

    // Animation not listed, must be finished
    return true;
}

bool AnimationController::IsAnimationActive(const String& name, bool check_fadeout) const
{
    auto i = animations_.Find(name);
    if (i != animations_.End())
    {
        if (check_fadeout)
            return true;
        else 
        {
            if (i->second_.phase_ != FadeOutPhase)
                return true;
            else
                return false;
        }
    }

    return false;
}

bool AnimationController::SetAnimationAutoStop(const String& name, bool enable)
{
    auto i = animations_.Find(name);
    if (i != animations_.End())
    {
        i->second_.auto_stop_ = enable;
        return true;
    }

    // Animation not active
    return false;
}

bool AnimationController::SetAnimationNumLoops(const String& name, uint repeats)
{
    auto i = animations_.Find(name);
    if (i != animations_.End())
    {
        i->second_.num_repeats_ = repeats;
        return true;
    }
    // Animation not active
    return false;
}

StringVector AnimationController::AvailableAnimations()
{
    StringVector availableList;
    
    return availableList;
}

StringVector AnimationController::ActiveAnimations() const
{
    StringVector activeList;

    for(auto i = animations_.Begin(); i != animations_.End(); ++i)
    {
        if (i->second_.phase_ != StopPhase)
            activeList.Push(i->first_);
    }
    
    return activeList;
}

bool AnimationController::DisableAnimation(const String& name, float fadeout)
{
    auto i = animations_.Find(name);
    if (i != animations_.End())
    {
        i->second_.phase_ = FadeOutPhase;
        i->second_.fade_period_ = fadeout;
        return true;
    }
    // Animation not active
    return false;
}

void AnimationController::DisableAllAnimations(float fadeout)
{
    auto i = animations_.Begin();
    while(i != animations_.End())
    {
        i->second_.phase_ = FadeOutPhase;
        i->second_.fade_period_ = fadeout;
        ++i;
    }
}

void AnimationController::SetAnimationToEnd(const String& name)
{
    auto i = animations_.Find(name);
    if (i != animations_.End() && i->second_.animationState_)
    {
        SetAnimationTimePosition(name, i->second_.animationState_->GetLength());
    }
}

bool AnimationController::SetAnimationSpeed(const String& name, float speedfactor)
{
    auto i = animations_.Find(name);
    if (i != animations_.End())
    {
        i->second_.speed_factor_ = speedfactor;
        return true;
    }
    // Animation not active
    return false;
}

bool AnimationController::SetAnimationWeight(const String& name, float weight)
{
    auto i = animations_.Find(name);
    if (i != animations_.End())
    {
        i->second_.weight_factor_ = weight;
        return true;
    }
    // Animation not active
    return false;
}

bool AnimationController::SetAnimationPriority(const String& name, bool high_priority)
{
    auto i = animations_.Find(name);
    if (i != animations_.End())
    {
        i->second_.high_priority_ = high_priority;
        return true;
    }
    // Animation not active
    return false;
}

bool AnimationController::SetAnimationTimePosition(const String& name, float newPosition)
{
    auto i = animations_.Find(name);
    if (i != animations_.End() && i->second_.animationState_)
    {
        i->second_.animationState_->SetTime(newPosition);
        return true;
    }
    // Animation not active
    return false;
}

bool AnimationController::SetAnimationRelativeTimePosition(const String& name, float newPosition)
{
    auto i = animations_.Find(name);
    if (i != animations_.End() && i->second_.animationState_)
    {
        i->second_.animationState_->SetTime(Clamp(newPosition, 0.0f, 1.0f) * i->second_.animationState_->GetLength());
        return true;
    }
    // Animation not active
    return false;
}

float AnimationController::AnimationLength(const String& name)
{
    auto i = animations_.Find(name);
    if (i != animations_.End() && i->second_.animationState_)
        return i->second_.animationState_->GetLength();
    else
        return 0.0f;
}

float AnimationController::AnimationTimePosition(const String& name)
{
    auto i = animations_.Find(name);
    if (i != animations_.End() && i->second_.animationState_)
        return i->second_.animationState_->GetTime();
    else
        return 0.0f;
}

float AnimationController::AnimationRelativeTimePosition(const String& name)
{
    auto i = animations_.Find(name);
    if (i != animations_.End() && i->second_.animationState_)
        return i->second_.animationState_->GetTime() / i->second_.animationState_->GetLength();
    else
        return 0.0f;
}

void AnimationController::PlayAnim(const String &name, const String &fadein, const String &exclusive)
{
    if (!ViewEnabled())
        return;

    if (!name.Length())
    {
        LogWarning("Empty animation name for PlayAnim");
        return;
    }
    
    float fadein_ = 0.0f;
    if (fadein.Length())
        fadein_ = Urho3D::ToFloat(fadein);
    bool exclusive_ = false;
    if (exclusive.Length())
        exclusive_ = Urho3D::ToBool(exclusive);

    bool success;
    if (exclusive_)
        success = EnableExclusiveAnimation(name, false, fadein_, false);
    else
        success = EnableAnimation(name, false, fadein_, false);
    if (!success)
    {
        StringVector anims = AvailableAnimations();
        void (*log)(const String &) = LogDebug; if (anims.Size() > 0) log = LogWarning;
        log("Failed to play animation \"" + name + "\" on entity " + ParentEntity()->Name());
        log("The entity has " + String(anims.Size()) + " animations available: " + Join(anims,","));
    }
}

void AnimationController::PlayLoopedAnim(const String &name, const String &fadein, const String &exclusive)
{
    if (!ViewEnabled())
        return;

    if (!name.Length())
    {
        LogWarning("Empty animation name for PlayLoopedAnim");
        return;
    }
    
    float fadein_ = 0.0f;
    if (fadein.Length())
        fadein_ = ToFloat(fadein);
    bool exclusive_ = false;
    if (exclusive.Length())
        exclusive_ = ToBool(exclusive);

    bool success;
    if (exclusive_)
        success = EnableExclusiveAnimation(name, true, fadein_, fadein_, false);
    else
        success = EnableAnimation(name, true, fadein_, false);
    if (!success)
    {
        StringVector anims = AvailableAnimations();
        void (*log)(const String &) = LogDebug; if (anims.Size() > 0) log = LogWarning;
        log("Failed to play looped animation \"" + name + "\" on entity " + ParentEntity()->Name());
        log("The entity has " + String(anims.Size()) + " animations available: " + Join(anims, ","));
    }
}

void AnimationController::PlayReverseAnim(const String &name, const String &fadein, const String &exclusive)
{
    if (!ViewEnabled())
        return;

    if (!name.Length())
    {
        LogWarning("Empty animation name for PlayReverseAnim");
        return;
    }
    
    float fadein_ = 0.0f;
    if (fadein.Length())
        fadein_ = ToFloat(fadein);
    bool exclusive_ = false;
    if (exclusive.Length())
        exclusive_ = ToBool(exclusive);
    bool success;
    if (exclusive_)
        success = EnableAnimation(name, true, fadein_, false);
    else
        success = EnableExclusiveAnimation(name, true, fadein_, fadein_, false);
    if (!success)
    {
        StringVector anims = AvailableAnimations();
        void (*log)(const String &) = LogDebug; if (anims.Size() > 0) log = LogWarning;
        log("Failed to play animation \"" + name + "\" in reverse on entity " + ParentEntity()->Name());
        log("The entity has " + String(anims.Size()) + " animations available: " + Join(anims, ","));

        SetAnimationToEnd(name);
        SetAnimationSpeed(name, -1.0f);
    }
}

void AnimationController::PlayAnimAutoStop(const String &name, const String &fadein, const String &exclusive)
{
    if (!name.Length())
    {
        LogWarning("Empty animation name for PlayAnimAutoStop");
        return;
    }
    
    float fadein_ = 0.0f;
    if (fadein.Length())
        fadein_ = ToFloat(fadein);
    bool exclusive_ = false;
    if (exclusive.Length())
        exclusive_ = ToBool(exclusive);
    bool success;
    if (exclusive_)
        success = EnableExclusiveAnimation(name, false, fadein_, false);
    else
        success = EnableAnimation(name, false, fadein_, false);

    if (!success)
    {
        StringVector anims = AvailableAnimations();
        void (*log)(const String &) = LogDebug; if (anims.Size() > 0) log = LogWarning;
        log("Failed to play animation \"" + name + "\" on entity " + ParentEntity()->Name());
        log("The entity has " + String(anims.Size()) + " animations available: " + Join(anims, ","));

        // Enable autostop, and start always from the beginning
        SetAnimationAutoStop(name, true);
        SetAnimationTimePosition(name, 0.0f);
    }
}

void AnimationController::StopAnim(const String &name, const String &fadeout)
{
    if (!name.Length())
    {
        LogWarning("Empty animation name for StopAnim");
        return;
    }
    
    float fadeout_ = 0.0f;
    if (fadeout.Length())
        fadeout_ = ToFloat(fadeout);
    DisableAnimation(name, fadeout_);
}

void AnimationController::StopAllAnims(const String &fadeout)
{
    float fadeout_ = 0.0f;
    if (fadeout.Length())
        fadeout_ = ToFloat(fadeout);
    DisableAllAnimations(fadeout_);
}

void AnimationController::SetAnimSpeed(const String &name, const String &animspeed)
{
    if (!name.Length())
    {
        LogWarning("Empty animation name for SetAnimSpeed");
        return;
    }
    if (!animspeed.Length())
    {
        LogWarning("No animation speed specified for SetAnimSpeed");
        return;
    }
    
    float speed = ToFloat(animspeed);
    SetAnimationSpeed(name, speed);
}

void AnimationController::SetAnimWeight(const String &name, const String &animweight)
{
    if (!name.Length())
    {
        LogWarning("Empty animation name for SetAnimWeight");
        return;
    }
    if (!animweight.Length())
    {
        LogWarning("No animation weight specified for SetAnimWeight");
        return;
    }
    
    float weight = ToFloat(animweight);
    SetAnimationWeight(name, weight);
}

}
