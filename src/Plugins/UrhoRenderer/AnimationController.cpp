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

#include <Engine/Scene/Node.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Graphics/Light.h>
#include "Engine/Graphics/Animation.h"
#include "Engine/Graphics/AnimatedModel.h"
#include "Engine/Graphics/AnimationState.h"
#include "Engine/Core/StringUtils.h"

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

    parent->ComponentAdded.Connect(this, &AnimationController::OnComponentStructureChanged);
    parent->ComponentRemoved.Connect(this, &AnimationController::OnComponentStructureChanged);

    if (parent->ParentScene())
        world_ = parent->ParentScene()->Subsystem<GraphicsWorld>();
}

void AnimationController::OnComponentStructureChanged(IComponent*, AttributeChange::Type)
{
    Entity *entity = ParentEntity();
    if (!entity)
        return;

    placeable_ = entity->Component<Placeable>();
    mesh_ = entity->Component<Mesh>();
}

void AnimationController::AttributesChanged()
{

}

Urho3D::AnimationState* AnimationController::UrhoAnimationState(const String& name)
{
    if (!mesh_)
        return 0;

    Urho3D::AnimatedModel* model = mesh_.Get()->UrhoMesh();
    if (!model)
        return 0;

    int animIndex = -1;
    const auto& animationStates = model->GetAnimationStates();
    for (unsigned int i=0 ; i<animationStates.Size() ; ++i)
    {
        if (name.Compare(animationStates[i]->GetAnimation()->GetAnimationName(), false))
        {
            animIndex = i;
            break;
        }
    }
    if (animIndex < 0)
        return 0;

    return animationStates[animIndex];
}

void AnimationController::Update(float frametime)
{
    for (auto iter = animationStates_.Begin() ; iter != animationStates_.End() ; ++iter)
    {
        animationStates_[iter->first_]->AddTime(frametime);
    }

    /// \todo implement
}

void AnimationController::DrawSkeleton(float frametime)
{
    if (!world_ || !mesh_)
        return;

    Urho3D::AnimatedModel* model = mesh_->UrhoMesh();
    if (!model)
        return;

    //Urho3D::Skeleton &skeleton = model->GetSkeleton();

    /// \todo draw skeleton
}

bool AnimationController::EnableAnimation(const String& name, bool looped, float fadein, bool high_priority)
{
    Urho3D::AnimationState* animstate = UrhoAnimationState(name);
    if (!animstate) 
        return false;

    animstate->SetLooped(looped);

    // See if we already have this animation
    auto i = animations_.Find(name);
    if (i != animations_.End())
    {
        i->second_.phase_ = FadeInPhase;
        i->second_.num_repeats_ = (looped ? 0: 1);
        i->second_.fade_period_ = fadein;
        i->second_.high_priority_ = high_priority;
        // If animation is nonlooped and has already reached end, rewind to beginning
        if ((!looped) && (i->second_.speed_factor_ > 0.0f))
        {
            if (animstate->GetTime() >= animstate->GetLength())
                animstate->SetTime(0.0f);
        }
        return true;
    }
    
    // Start new animation from zero weight & speed factor 1, also reset time position
    animstate->SetTime(0.0f);
    
    Animation newanim;
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
        if (other_name.Compare(name, false) == 0)
        {
            i->second_.phase_ = FadeOutPhase;
            i->second_.fade_period_ = fadeout;
        }
        ++i;
    }

    // Then enable this
    return EnableAnimation(name, looped, fadein, high_priority);
}

bool AnimationController::HasAnimationFinished(const String& name)
{
    Urho3D::AnimationState* animstate = UrhoAnimationState(name);

    if (!animstate) 
        return true;

    auto i = animations_.Find(name);
    if (i != animations_.End())
    {
        if ((!animstate->IsLooped()) && ((i->second_.speed_factor_ >= 0.f && animstate->GetTime() >= animstate->GetLength()) ||
            (i->second_.speed_factor_ < 0.f && animstate->GetTime() <= 0.f)))
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

StringList AnimationController::AvailableAnimations()
{
    //Ogre::Entity* entity = GetEntity();
    //if (!entity) 
    //    return availableList;
    //Ogre::AnimationStateSet* anims = entity->getAllAnimationStates();
    //if (!anims) 
    //    return availableList;
    //Ogre::AnimationStateIterator i = anims->UrhoAnimationStateIterator();
    //while(i.hasMoreElements()) 
    //{
    //    Ogre::AnimationState *animstate = i.getNext();
    //    availableList << String(animstate->getAnimationName().c_str());
    //}

    StringList availableList;
    if (!mesh_ || !mesh_->UrhoMesh())
        return availableList;

    Urho3D::AnimatedModel *animatedModel = mesh_->UrhoMesh();
    auto animstates = animatedModel->GetAnimationStates();
    availableList.Resize(animstates.Size());
    for (unsigned int i = 0 ; i<animstates.Size() ; ++i)
    {
        availableList[i] = animstates[i]->GetAnimation()->GetAnimationName();
    }

    return availableList;
}

StringList AnimationController::ActiveAnimations() const
{
    StringList activeList;

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
    Urho3D::AnimationState* animstate = UrhoAnimationState(name);
    if (!animstate)
        return;
        
    if (animstate)
    {
        SetAnimationTimePosition(name, animstate->GetLength());
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
    Urho3D::AnimationState* animstate = UrhoAnimationState(name);
    if (!animstate) 
        return false;
        
    // See if we find this animation in the list of active animations
    auto i = animations_.Find(name);
    if (i != animations_.End())
    {
        animstate->SetTime(newPosition);
        return true;
    }
    // Animation not active
    return false;
}

bool AnimationController::SetAnimationRelativeTimePosition(const String& name, float newPosition)
{
    Urho3D::AnimationState* animstate = UrhoAnimationState(name);
    if (!animstate) 
        return false;
        
    // See if we find this animation in the list of active animations
    auto i = animations_.Find(name);
    if (i != animations_.End())
    {
        animstate->SetTime(Clamp(newPosition, 0.0f, 1.0f) * animstate->GetLength());
        return true;
    }
    // Animation not active
    return false;
}

float AnimationController::AnimationLength(const String& name)
{
    Urho3D::AnimationState* animstate = UrhoAnimationState(name);
    if (!animstate)
        return 0.0f;
    else
        return animstate->GetLength();
}

float AnimationController::AnimationTimePosition(const String& name)
{
    Urho3D::AnimationState* animstate = UrhoAnimationState(name);
    if (!animstate)
        return 0.0f;
    
    // See if we find this animation in the list of active animations
    auto i = animations_.Find(name);
    if (i != animations_.End())
        return animstate->GetTime();
    else return 0.0f;
}

float AnimationController::AnimationRelativeTimePosition(const String& name)
{
    Urho3D::AnimationState* animstate = UrhoAnimationState(name);
    if (!animstate)
        return 0.0f;
    
    // See if we find this animation in the list of active animations
    auto i = animations_.Find(name);
    if (i != animations_.End())
        return animstate->GetTime() / animstate->GetLength();
    else return 0.0f;
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
        StringList anims = AvailableAnimations();
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
        StringList anims = AvailableAnimations();
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
        StringList anims = AvailableAnimations();
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
        StringList anims = AvailableAnimations();
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