// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "OgreSkeletonAsset.h"
#include "LoggingFunctions.h"
#include "Framework.h"
#include "AssetAPI.h"
#include "UrhoRenderer.h"
#include "OgreMeshDefines.h"
#include "Math/float3.h"
#include "Math/Quat.h"

#include <Profiler.h>
#include <StringUtils.h>

#include <MemoryBuffer.h>
#include <Engine/Graphics/Animation.h>
#include <stdexcept>

namespace Tundra
{

using namespace Ogre;

static uint currentLength;

static const unsigned short    HEADER_CHUNK_ID         = 0x1000;

static const uint MSTREAM_OVERHEAD_SIZE               = sizeof(u16) + sizeof(uint);
static const uint MSTREAM_BONE_SIZE_WITHOUT_SCALE     = MSTREAM_OVERHEAD_SIZE + sizeof(u16) + (sizeof(float) * 7);
static const uint MSTREAM_KEYFRAME_SIZE_WITHOUT_SCALE = MSTREAM_OVERHEAD_SIZE + (sizeof(float) * 8);

enum SkeletonChunkId
{
    SKELETON_HEADER                = 0x1000,
        // char* version           : Version number check
        SKELETON_BLENDMODE        = 0x1010, // optional
            // unsigned short blendmode        : SkeletonAnimationBlendMode
    SKELETON_BONE                = 0x2000,
    // Repeating section defining each bone in the system. 
    // Bones are assigned indexes automatically based on their order of declaration
    // starting with 0.
        // char* name                       : name of the bone
        // unsigned short handle            : handle of the bone, should be contiguous & start at 0
        // Vector3 position                 : position of this bone relative to parent 
        // Quaternion orientation           : orientation of this bone relative to parent 
        // Vector3 scale                    : scale of this bone relative to parent 
    SKELETON_BONE_PARENT        = 0x3000,
    // Record of the parent of a single bone, used to build the node tree
    // Repeating section, listed in Bone Index order, one per Bone
        // unsigned short handle             : child bone
        // unsigned short parentHandle   : parent bone
    SKELETON_ANIMATION            = 0x4000,
    // A single animation for this skeleton
        // char* name                       : Name of the animation
        // float length                      : Length of the animation in seconds
        SKELETON_ANIMATION_BASEINFO = 0x4010,
        // [Optional] base keyframe information
        // char* baseAnimationName (blank for self)
        // float baseKeyFrameTime
        SKELETON_ANIMATION_TRACK    = 0x4100,
        // A single animation track (relates to a single bone)
        // Repeating section (within SKELETON_ANIMATION)
            // unsigned short boneIndex     : Index of bone to apply to
            SKELETON_ANIMATION_TRACK_KEYFRAME = 0x4110,
            // A single keyframe within the track
            // Repeating section
                // float time                    : The time position (seconds)
                // Quaternion rotate            : Rotation to apply at this keyframe
                // Vector3 translate            : Translation to apply at this keyframe
                // Vector3 scale                : Scale to apply at this keyframe
    SKELETON_ANIMATION_LINK        = 0x5000
    // Link to another skeleton, to re-use its animations
        // char* skeletonName                    : name of skeleton to get animations from
        // float scale                            : scale to apply to trans/scale keys
};

static String ReadLine(Urho3D::Deserializer& stream)
{
    String str;
    while(!stream.IsEof())
    {
        char c = stream.ReadByte();
        if (c == '\n')
            break;
        str += c;
    }
    return str;
}

static u16 ReadHeader(Urho3D::Deserializer& stream, bool readLength = true)
{
    u16 id = stream.ReadUShort();
    if (readLength)
        currentLength = stream.ReadUInt();

    return id;
}

static Quat ReadQuat(Urho3D::Deserializer& stream)
{
    Quat result;
    stream.Read(&result.x, sizeof result);
    return result;
}

static void RollbackHeader(Urho3D::Deserializer& stream)
{
    stream.Seek(stream.GetPosition() - MSTREAM_OVERHEAD_SIZE);
}

static void SkipBytes(Urho3D::Deserializer& stream, uint numBytes)
{
    stream.Seek(stream.GetPosition() + numBytes);
}

static void ReadSkeleton(Urho3D::Deserializer& stream, Skeleton *skeleton);
static void ReadBone(Urho3D::Deserializer& stream, Skeleton *skeleton);
static void ReadBoneParent(Urho3D::Deserializer& stream, Skeleton *skeleton);
static void ReadSkeletonAnimation(Urho3D::Deserializer& stream, Skeleton *skeleton);
static void ReadSkeletonAnimationTrack(Urho3D::Deserializer& stream, Skeleton *skeleton, Animation *dest);
static void ReadSkeletonAnimationKeyFrame(Urho3D::Deserializer& stream, VertexAnimationTrack *dest);
static void ReadSkeletonAnimationLink(Urho3D::Deserializer& stream, Skeleton *skeleton);

static void ReadSkeleton(Urho3D::Deserializer& stream, Skeleton *skeleton)
{
    u16 id = ReadHeader(stream, false);
    if (id != HEADER_CHUNK_ID) {
        throw std::runtime_error("Invalid Ogre Skeleton file header.");
    }

    // This deserialization supports both versions of the skeleton spec
    String version = ReadLine(stream);
    /*
    if (version != SKELETON_VERSION_1_8 && version != SKELETON_VERSION_1_1)
    {
        throw std::runtime_error(Formatter::format() << "Skeleton version " << version << " not supported by this importer."
            << " Supported versions: " << SKELETON_VERSION_1_8 << " and " << SKELETON_VERSION_1_1);
    }
    */
    
    while (!stream.IsEof())
    {
        id = ReadHeader(stream);
        switch(id)
        {
            case SKELETON_BLENDMODE:
            {
                skeleton->blendMode = static_cast<Skeleton::BlendMode>(stream.ReadUShort());
                break;
            }
            case SKELETON_BONE:
            {
                ReadBone(stream, skeleton);
                break;
            }
            case SKELETON_BONE_PARENT:
            {
                ReadBoneParent(stream, skeleton);
                break;
            }
            case SKELETON_ANIMATION:
            {
                ReadSkeletonAnimation(stream, skeleton);
                break;
            }
            case SKELETON_ANIMATION_LINK:
            {
                ReadSkeletonAnimationLink(stream, skeleton);
                break;
            }
        }
    }
    
    // Calculate bone matrices for root bones. Recursively calculates their children.
    for (uint i=0, len=skeleton->bones.Size(); i<len; ++i)
    {
        Bone *bone = skeleton->bones[i];
        if (!bone->IsParented())
            bone->CalculateWorldMatrixAndDefaultPose(skeleton);
    }
}

static void ReadBone(Urho3D::Deserializer& stream, Skeleton *skeleton)
{
    Bone *bone = new Bone();
    bone->name = ReadLine(stream);
    bone->id = stream.ReadUShort();

    // Pos and rot
    bone->position = stream.ReadVector3();
    bone->rotation = ReadQuat(stream);

    // Scale (optional)
    if (currentLength > MSTREAM_BONE_SIZE_WITHOUT_SCALE)
        bone->scale = stream.ReadVector3();

    // Bone indexes need to start from 0 and be contiguous
    if (bone->id != skeleton->bones.Size()) {
        throw std::runtime_error("Ogre Skeleton bone indexes not contiguous.");
    }

    skeleton->bones.Push(bone);
}

static void ReadBoneParent(Urho3D::Deserializer& stream, Skeleton *skeleton)
{
    u16 childId = stream.ReadUShort();
    u16 parentId = stream.ReadUShort();
    
    Bone *child = skeleton->BoneById(childId);
    Bone *parent = skeleton->BoneById(parentId);
    
    if (child && parent)
        parent->AddChild(child);
    else
        LogWarning("Failed to find bones for parenting: Child id " + String(childId) + " for parent id " + String(parentId));
}

static void ReadSkeletonAnimation(Urho3D::Deserializer& stream, Skeleton *skeleton)
{
    Animation *anim = new Animation(skeleton);
    anim->name = ReadLine(stream);
    anim->length = stream.ReadFloat();
    
    if (!stream.IsEof())
    {
        u16 id = ReadHeader(stream);
        if (id == SKELETON_ANIMATION_BASEINFO)
        {
            anim->baseName = ReadLine(stream);
            anim->baseTime = stream.ReadFloat();

            // Advance to first track
            id = ReadHeader(stream);
        }

        while (!stream.IsEof() && id == SKELETON_ANIMATION_TRACK)
        {
            ReadSkeletonAnimationTrack(stream, skeleton, anim);

            if (!stream.IsEof())
                id = ReadHeader(stream);
        }
        if (!stream.IsEof())
            RollbackHeader(stream);
    }
    
    skeleton->animations.Push(anim);
}

static void ReadSkeletonAnimationTrack(Urho3D::Deserializer& stream, Skeleton * /*skeleton*/, Animation *dest)
{
    u16 boneId = stream.ReadUShort();
    Bone *bone = dest->parentSkeleton->BoneById(boneId);
    if (!bone) {
        throw std::runtime_error("Cannot read animation track, target bone not in target Skeleton");
    }
    
    VertexAnimationTrack track;
    track.type = VertexAnimationTrack::VAT_TRANSFORM;
    track.boneName = bone->name;
    
    u16 id = ReadHeader(stream);
    while (!stream.IsEof() && id == SKELETON_ANIMATION_TRACK_KEYFRAME)
    {
        ReadSkeletonAnimationKeyFrame(stream, &track);

        if (!stream.IsEof())
            id = ReadHeader(stream);
    }
    if (!stream.IsEof())
        RollbackHeader(stream);

    dest->tracks.Push(track);
}

static void ReadSkeletonAnimationKeyFrame(Urho3D::Deserializer& stream, VertexAnimationTrack *dest)
{
    TransformKeyFrame keyframe;
    keyframe.timePos = stream.ReadFloat();
    
    // Rot and pos
    keyframe.rotation = ReadQuat(stream);
    keyframe.position = stream.ReadVector3();
    
    // Scale (optional)
    if (currentLength > MSTREAM_KEYFRAME_SIZE_WITHOUT_SCALE)
        keyframe.scale = stream.ReadVector3();
    
    // Note: transforms are now in Ogre's offset transform format. The bone's transform needs to be applied when converting to Urho animation
    dest->transformKeyFrames.Push(keyframe);
}

static void ReadSkeletonAnimationLink(Urho3D::Deserializer& stream, Skeleton * /*skeleton*/)
{
    // Skip bounds, not compatible with Assimp.
    ReadLine(stream); // skeleton name
    SkipBytes(stream, sizeof(float) * 3); // scale
}

OgreSkeletonAsset::OgreSkeletonAsset(AssetAPI *owner, const String &type_, const String &name_) :
    IAsset(owner, type_, name_)
{
}

OgreSkeletonAsset::~OgreSkeletonAsset()
{
}

bool OgreSkeletonAsset::DeserializeFromData(const u8 *data_, uint numBytes, bool /*allowAsynchronous*/)
{
    PROFILE(OgreSkeletonAsset_LoadFromFileInMemory);

    /// Force an unload of previous data first.
    Unload();

    Urho3D::MemoryBuffer buffer(data_, numBytes);

    SharedPtr<Ogre::Skeleton> ogreSkel(new Ogre::Skeleton());
    try
    {
        ReadSkeleton(buffer, ogreSkel);
    }
    catch (std::exception& e)
    {
        LogError("OgreSkeletonAsset::DeserializeFromData: " + String(e.what()));
        return false;
    }

    // Fill Urho bone structure
    Vector<Urho3D::Bone>& bones = skeleton.GetModifiableBones();
    bones.Resize(ogreSkel->bones.Size());
    for (uint i = 0; i < ogreSkel->bones.Size(); ++i)
    {
        bones[i].name_ = ogreSkel->bones[i]->name;
        bones[i].nameHash_ = StringHash(bones[i].name_);

        if (ogreSkel->bones[i]->IsParented())
            bones[i].parentIndex_ = ogreSkel->bones[i]->parentId;
        else
        {
            bones[i].parentIndex_ = i;
            skeleton.SetRootBoneIndex(i);
        }
        Urho3D::Matrix4 pose = ogreSkel->bones[i]->defaultPose;
        Urho3D::Vector3 pos, scale;
        Urho3D::Quaternion rot;
        pose.Decompose(pos, rot, scale);

        bones[i].animated_ = true;
        bones[i].initialPosition_ = pos;
        bones[i].initialRotation_ = rot;
        bones[i].initialScale_ = scale;
        bones[i].offsetMatrix_ = Urho3D::Matrix3x4(ogreSkel->bones[i]->worldMatrix).Inverse();
        // The skeleton can not know the vertex information necessary to calculate bone bounding boxes. Therefore that data
        // must be combined later from the mesh's data
    }

    // Create animations
    for (uint i = 0; i < ogreSkel->animations.Size(); ++i)
    {
        Ogre::Animation* ogreAnim = ogreSkel->animations[i];
        if (!ogreAnim)
            continue;
        String animName = ogreAnim->name;
        if (animName.Empty())
            continue;

        SharedPtr<Urho3D::Animation> urhoAnim(new Urho3D::Animation(context_));
        urhoAnim->SetLength(ogreAnim->length);
        // Set both animation & resource name, same for now
        urhoAnim->SetAnimationName(animName);
        urhoAnim->SetName(animName);

        Vector<Urho3D::AnimationTrack> urhoTracks;
        for (uint j = 0; j < ogreAnim->tracks.Size(); ++j)
        {
            const Ogre::VertexAnimationTrack& ogreTrack = ogreAnim->tracks[j];
            if (ogreTrack.type != Ogre::VertexAnimationTrack::VAT_TRANSFORM)
                continue;
            Urho3D::AnimationTrack urhoTrack;
            urhoTrack.channelMask_ = Urho3D::CHANNEL_POSITION | Urho3D::CHANNEL_ROTATION;
            urhoTrack.name_ = ogreTrack.boneName;
            urhoTrack.nameHash_ = StringHash(ogreTrack.boneName);

            Urho3D::Bone* urhoBone = 0;
            for (uint k = 0; k < bones.Size(); ++k)
            {
                if (bones[k].name_ == urhoTrack.name_)
                {
                    urhoBone = &bones[k];
                    break;
                }
            }
            if (!urhoBone)
            {
                LogWarning("OgreSkeletonAsset::DeserializeFromData: found animation track referring to a non-existent bone " + urhoTrack.name_ + ", skipping");
                continue;
            }

            for (uint k = 0; k < ogreTrack.transformKeyFrames.Size(); ++k)
            {
                const Ogre::TransformKeyFrame& ogreKeyframe = ogreTrack.transformKeyFrames[k];
                Urho3D::AnimationKeyFrame urhoKeyframe;
                urhoKeyframe.time_ = ogreKeyframe.timePos;

                // Urho uses absolute bone poses in animation, while Ogre uses additive. Convert to absolute now.
                urhoKeyframe.position_ = urhoBone->initialPosition_ + urhoBone->initialRotation_ * ogreKeyframe.position;
                urhoKeyframe.rotation_ = urhoBone->initialRotation_ * ogreKeyframe.rotation;
                urhoKeyframe.scale_ = ogreKeyframe.scale;
                if (!ogreKeyframe.scale.Equals(float3(1.0f, 1.0f, 1.0f)))
                    urhoTrack.channelMask_ |= Urho3D::CHANNEL_SCALE;
                urhoTrack.keyFrames_.Push(urhoKeyframe);
            }
            urhoTracks.Push(urhoTrack);
        }

        urhoAnim->SetTracks(urhoTracks);
        animations[animName] = urhoAnim;
    }

    // Inform load has finished.
    assetAPI->AssetLoadCompleted(Name());
    return true;
}

void OgreSkeletonAsset::DoUnload()
{
    skeleton = Urho3D::Skeleton();
    animations.Clear();
}

bool OgreSkeletonAsset::IsLoaded() const
{
    return skeleton.GetNumBones() > 0;
}

Urho3D::Animation* OgreSkeletonAsset::AnimationByName(const String& name) const
{
    HashMap<String, SharedPtr<Urho3D::Animation> >::ConstIterator i = animations.Find(name);
    return i != animations.End() ? i->second_.Get() : nullptr;
}

}
