/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

#include "CoreTypes.h"
#include "Math/float3.h"
#include "Math/Quat.h"
#include "Math/float4x4.h"

#include <HashMap.h>
#include <HashSet.h>
#include <RefCounted.h>

namespace Tundra
{

namespace Ogre
{

/** @note Parts of this implementation, for example enums, deserialization constants and logic
    has been copied directly with minor modifications from the MIT licensed Ogre3D code base.
    See more from https://bitbucket.org/sinbad/ogre. */

// Forward decl
class Mesh;
class SubMesh;
class Skeleton;

typedef HashMap<u16, Vector<u8> > VertexBufferBindings;

// Ogre Vertex Element
class VertexElement
{
public:
    /// Vertex element semantics, used to identify the meaning of vertex buffer contents
    enum Semantic {
        /// Position, 3 reals per vertex
        VES_POSITION = 1,
        /// Blending weights
        VES_BLEND_WEIGHTS = 2,
        /// Blending indices
        VES_BLEND_INDICES = 3,
        /// Normal, 3 reals per vertex
        VES_NORMAL = 4,
        /// Diffuse colours
        VES_DIFFUSE = 5,
        /// Specular colours
        VES_SPECULAR = 6,
        /// Texture coordinates
        VES_TEXTURE_COORDINATES = 7,
        /// Binormal (Y axis if normal is Z)
        VES_BINORMAL = 8,
        /// Tangent (X axis if normal is Z)
        VES_TANGENT = 9,
        /// The  number of VertexElementSemantic elements (note - the first value VES_POSITION is 1) 
        VES_COUNT = 9
    };

    /// Vertex element type, used to identify the base types of the vertex contents
    enum Type
    {
        VET_FLOAT1 = 0,
        VET_FLOAT2 = 1,
        VET_FLOAT3 = 2,
        VET_FLOAT4 = 3,
        /// alias to more specific colour type - use the current rendersystem's colour packing
        VET_COLOUR = 4,
        VET_SHORT1 = 5,
        VET_SHORT2 = 6,
        VET_SHORT3 = 7,
        VET_SHORT4 = 8,
        VET_UBYTE4 = 9,
        /// D3D style compact colour
        VET_COLOUR_ARGB = 10,
        /// GL style compact colour
        VET_COLOUR_ABGR = 11,
        VET_DOUBLE1 = 12,
        VET_DOUBLE2 = 13,
        VET_DOUBLE3 = 14,
        VET_DOUBLE4 = 15,
        VET_USHORT1 = 16,
        VET_USHORT2 = 17,
        VET_USHORT3 = 18,
        VET_USHORT4 = 19,
        VET_INT1 = 20,
        VET_INT2 = 21,
        VET_INT3 = 22,
        VET_INT4 = 23,
        VET_UINT1 = 24,
        VET_UINT2 = 25,
        VET_UINT3 = 26,
        VET_UINT4 = 27
    };

    VertexElement();

    /// Size of the vertex element in bytes.
    uint Size() const;
    
    /// Count of components in this element, eg. VET_FLOAT3 return 3.
    uint ComponentCount() const;
    
    /// Type as string.
    String TypeToString();
    
    /// Semantic as string.
    String SemanticToString();
    
    static uint TypeSize(Type type);
    static uint ComponentCount(Type type);
    static String TypeToString(Type type);
    static String SemanticToString(Semantic semantic);

    u16 index;
    u16 source;
    u16 offset;
    Type type;
    Semantic semantic;
};
typedef Vector<VertexElement> VertexElementList;

/// Ogre Vertex Bone Assignment
struct VertexBoneAssignment
{
    VertexBoneAssignment() :
        vertexIndex(0),
        boneIndex(0),
        weight(0.f)
    {
    }

    uint vertexIndex;
    u16 boneIndex;
    float weight;
};
typedef Vector<VertexBoneAssignment> VertexBoneAssignmentList;
typedef HashMap<uint, VertexBoneAssignmentList > VertexBoneAssignmentsMap;

// Ogre Vertex Data interface, inherited by the binary and XML implementations.
class IVertexData
{
public:
    IVertexData();
    
    /// Returns if bone assignments are available.
    bool HasBoneAssignments() const;
    
    /// Add vertex mapping from old to new index.
    void AddVertexMapping(uint oldIndex, uint newIndex);

    /// Returns a set of bone indexes that are referenced by bone assignments (weights).
    HashSet<u16> ReferencedBonesByWeights() const;

    /// Vertex count.
    uint count;
    
    /// Bone assignments.
    VertexBoneAssignmentList boneAssignments;
    
private:
    void BoneAssignmentsForVertex(uint currentIndex, uint newIndex, VertexBoneAssignmentList &dest) const;
    
    HashMap<uint, Vector<u32> > vertexIndexMapping;
    VertexBoneAssignmentsMap boneAssignmentsMap;
};

// Ogre Vertex Data
class VertexData : public IVertexData
{
public:
    VertexData();
    ~VertexData();

    /// Releases all memory that this data structure owns.
    void Reset();

    /// Get vertex size for @c source.
    uint VertexSize(u16 source) const;

    /// Get vertex buffer for @c source.
    Vector<u8> *VertexBuffer(u16 source);

    /// Get vertex element for @c semantic for @c index.
    VertexElement *GetVertexElement(VertexElement::Semantic semantic, u16 index = 0);

    /// Vertex elements.
    VertexElementList vertexElements;

    /// Vertex buffers mapped to bind index.
    VertexBufferBindings vertexBindings;
};

// Ogre Index Data
class IndexData
{
public:
    IndexData();
    ~IndexData();

    /// Releases all memory that this data structure owns.
    void Reset();

    /// Index size in bytes.
    uint IndexSize() const;

    /// Face size in bytes.
    uint FaceSize() const;

    /// Index count.
    uint count;
    
    /// Face count.
    uint faceCount;

    /// If has 32-bit indexes.
    bool is32bit;

    /// Index buffer.
    Vector<u8> buffer;
};

/// Ogre Pose
class Pose
{
public:
    struct Vertex
    {
        uint index;
        float3 offset;
        float3 normal;
    };
    typedef HashMap<uint, Vertex> PoseVertexMap;

    Pose() : target(0), hasNormals(false) {}

    /// Name.
    String name;
    
    /// Target.
    u16 target;
    
    /// Does vertices map have normals.
    bool hasNormals;
    
    /// Vertex offset and normals.
    PoseVertexMap vertices;
};
typedef Vector<Pose*> PoseList;

/// Ogre Pose Key Frame Ref
struct PoseRef
{
    u16 index;
    float influence;
};
typedef PODVector<PoseRef> PoseRefList;

/// Ogre Pose Key Frame
struct PoseKeyFrame
{
    /// Time position in the animation.
    float timePos;

    PoseRefList references;
};
typedef PODVector<PoseKeyFrame> PoseKeyFrameList;

/// Ogre Morph Key Frame
struct MorphKeyFrame
{
    /// Time position in the animation.
    float timePos;

    Vector<u8> buffer;
};
typedef Vector<MorphKeyFrame> MorphKeyFrameList;

/// Ogre animation key frame
struct TransformKeyFrame
{
    TransformKeyFrame();
    
    float4x4 Transform();

    float timePos;
    
    Quat rotation;
    float3 position;
    float3 scale;
};
typedef Vector<TransformKeyFrame> TransformKeyFrameList;

/// Ogre Animation Track
struct VertexAnimationTrack
{
    enum Type
    {
        /// No animation
        VAT_NONE = 0,
        /// Morph animation is made up of many interpolated snapshot keyframes
        VAT_MORPH = 1,
        /// Pose animation is made up of a single delta pose keyframe
        VAT_POSE = 2,
        /// Keyframe that has its on pos, rot and scale for a time position
        VAT_TRANSFORM = 3
    };

    VertexAnimationTrack();
    
    // Animation type.
    Type type;
    
    /// Vertex data target.
    /**  0 == shared geometry
        >0 == submesh index + 1 */
    u16 target;
    
    /// Only valid for VAT_TRANSFORM.
    String boneName;

    /// Only one of these will contain key frames, depending on the type enum.
    PoseKeyFrameList poseKeyFrames;
    MorphKeyFrameList morphKeyFrames;
    TransformKeyFrameList transformKeyFrames;
};
typedef Vector<VertexAnimationTrack> VertexAnimationTrackList;

/// Ogre Animation
class Animation
{
public:
    Animation(Skeleton *parent);
    Animation(Mesh *parent);

    /// Returns the associated vertex data for a track in this animation.
    /** @note Only valid to call when parent Mesh is set. */
    VertexData *AssociatedVertexData(VertexAnimationTrack *track) const;

    /// Parent mesh.
    /** @note Set only when animation is read from a mesh. */
    Mesh *parentMesh;

    /// Parent skeleton.
    /** @note Set only when animation is read from a skeleton. */
    Skeleton *parentSkeleton;

    /// Animation name.
    String name;

    /// Base animation name.
    String baseName;

    /// Length in seconds.
    float length;

    /// Base animation key time.
    float baseTime;

    /// Animation tracks.
    VertexAnimationTrackList tracks;
};
typedef Vector<Animation*> AnimationList;

/// Ogre Bone
class Bone
{
public:
    Bone();

    /// Returns if this bone is parented.
    bool IsParented() const;

    /// Parent index as u16. Internally int as -1 means unparented.
    u16 ParentId() const;

    /// Add child bone.
    void AddChild(Bone *bone);
    
    /// Calculates the world matrix for bone and its children.
    void CalculateWorldMatrixAndDefaultPose(Skeleton *skeleton);
    
    u16 id;
    String name;

    Bone *parent;
    int parentId;
    std::vector<u16> children;

    float3 position;
    Quat rotation;
    float3 scale;
    
    float4x4 worldMatrix;
    float4x4 defaultPose;
};
typedef Vector<Bone*> BoneList;

/// Ogre Skeleton
class Skeleton : public Urho3D::RefCounted
{
public:
    enum BlendMode
    {
        /// Animations are applied by calculating a weighted average of all animations
        ANIMBLEND_AVERAGE = 0,
        /// Animations are applied by calculating a weighted cumulative total
        ANIMBLEND_CUMULATIVE = 1
    };

    Skeleton();
    ~Skeleton();

    /// Releases all memory that this data structure owns.
    void Reset();
    
    /// Returns unparented root bones.
    BoneList RootBones() const;
    
    /// Returns number of unparented root bones.
    uint NumRootBones() const;
    
    /// Get bone by name.
    Bone *BoneByName(const String &name) const;
    
    /// Get bone by id.
    Bone *BoneById(u16 id) const;
    
    BoneList bones;
    AnimationList animations;
    
    /// @todo Take blend mode into account, but where?
    BlendMode blendMode;
};

/// Ogre Sub Mesh interface, inherited by the binary and XML implementations.
class ISubMesh
{
public:
    /// @note Full list of Ogre types, not all of them are supported and exposed to Assimp.
    enum OperationType
    {
        /// A list of points, 1 vertex per point
        OT_POINT_LIST = 1,
        /// A list of lines, 2 vertices per line
        OT_LINE_LIST = 2,
        /// A strip of connected lines, 1 vertex per line plus 1 start vertex
        OT_LINE_STRIP = 3,
        /// A list of triangles, 3 vertices per triangle
        OT_TRIANGLE_LIST = 4,
        /// A strip of triangles, 3 vertices for the first triangle, and 1 per triangle after that 
        OT_TRIANGLE_STRIP = 5,
        /// A fan of triangles, 3 vertices for the first triangle, and 1 per triangle after that
        OT_TRIANGLE_FAN = 6
    };

    ISubMesh();

    /// SubMesh index.
    unsigned int index;

    /// SubMesh name.
    String name;

    /// Material used by this submesh.
    String materialRef;

    /// Texture alias information.
    String textureAliasName;
    String textureAliasRef;

    /// Assimp scene material index used by this submesh.
    /** -1 if no material or material could not be imported. */
    int materialIndex;
    
    /// If submesh uses shared geometry from parent mesh.
    bool usesSharedVertexData;

    /// Operation type.
    OperationType operationType;
};

/// Ogre SubMesh
class SubMesh : public ISubMesh
{
public:
    SubMesh();
    ~SubMesh();
    
    /// Releases all memory that this data structure owns.
    /** @note Vertex and index data contains shared ptrs
        that are freed automatically. In practice the ref count
        should be 0 after this reset. */
    void Reset();
    
    /// Vertex data.
    VertexData *vertexData;

    /// Index data.
    IndexData *indexData;
};
typedef Vector<SubMesh*> SubMeshList;

/// Ogre Mesh
class Mesh : public Urho3D::RefCounted
{
public:
    Mesh();
    ~Mesh();

    /// Releases all memory that this data structure owns.
    void Reset();

    /// Returns number of subMeshes.
    uint NumSubMeshes() const;

    /// Returns submesh for @c index.
    SubMesh *GetSubMesh(u16 index) const;

    /// Mesh has skeletal animations.
    bool hasSkeletalAnimations;
    
    /// Skeleton reference.
    String skeletonRef;

    /// Skeleton.
    Skeleton *skeleton;

    /// Vertex data
    VertexData *sharedVertexData;

    /// Sub meshes.
    SubMeshList subMeshes;

    /// Animations
    AnimationList animations;

    /// Poses
    PoseList poses;

    /// Mesh bounds
    float3 min;
    float3 max;
};

}

}
