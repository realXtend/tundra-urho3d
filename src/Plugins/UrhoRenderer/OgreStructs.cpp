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

#include "StableHeaders.h"
#include "OgreStructs.h"
#include "CoreDefines.h"
#include "LoggingFunctions.h"
#include "Math/float3x3.h"
#include "Math/TransformOps.h"

namespace Tundra
{

namespace Ogre
{

// VertexElement

VertexElement::VertexElement() : 
    index(0),
    source(0),
    offset(0),
    type(VET_FLOAT1),
    semantic(VES_POSITION)
{
}

uint VertexElement::Size() const
{
    return TypeSize(type);
}

uint VertexElement::ComponentCount() const
{
    return ComponentCount(type);
}

uint VertexElement::ComponentCount(Type type)
{
    switch(type)
    {
        case VET_COLOUR:
        case VET_COLOUR_ABGR:
        case VET_COLOUR_ARGB:
        case VET_FLOAT1:
        case VET_DOUBLE1:
        case VET_SHORT1:
        case VET_USHORT1:
        case VET_INT1:
        case VET_UINT1:
            return 1;
        case VET_FLOAT2:
        case VET_DOUBLE2:
        case VET_SHORT2:
        case VET_USHORT2:
        case VET_INT2:
        case VET_UINT2:
            return 2;
        case VET_FLOAT3:
        case VET_DOUBLE3:
        case VET_SHORT3:
        case VET_USHORT3:
        case VET_INT3:
        case VET_UINT3:
            return 3;
        case VET_FLOAT4:
        case VET_DOUBLE4:
        case VET_SHORT4:
        case VET_USHORT4:
        case VET_INT4:
        case VET_UINT4:
        case VET_UBYTE4:
            return 4;
    }
    return 0;
}

uint VertexElement::TypeSize(Type type)
{
    switch(type)
    {
        case VET_COLOUR:
        case VET_COLOUR_ABGR:
        case VET_COLOUR_ARGB:
            return sizeof(unsigned int);
        case VET_FLOAT1:
            return sizeof(float);
        case VET_FLOAT2:
            return sizeof(float)*2;
        case VET_FLOAT3:
            return sizeof(float)*3;
        case VET_FLOAT4:
            return sizeof(float)*4;
        case VET_DOUBLE1:
            return sizeof(double);
        case VET_DOUBLE2:
            return sizeof(double)*2;
        case VET_DOUBLE3:
            return sizeof(double)*3;
        case VET_DOUBLE4:
            return sizeof(double)*4;
        case VET_SHORT1:
            return sizeof(short);
        case VET_SHORT2:
            return sizeof(short)*2;
        case VET_SHORT3:
            return sizeof(short)*3;
        case VET_SHORT4:
            return sizeof(short)*4;
        case VET_USHORT1:
            return sizeof(unsigned short);
        case VET_USHORT2:
            return sizeof(unsigned short)*2;
        case VET_USHORT3:
            return sizeof(unsigned short)*3;
        case VET_USHORT4:
            return sizeof(unsigned short)*4;
        case VET_INT1:
            return sizeof(int);
        case VET_INT2:
            return sizeof(int)*2;
        case VET_INT3:
            return sizeof(int)*3;
        case VET_INT4:
            return sizeof(int)*4;
        case VET_UINT1:
            return sizeof(unsigned int);
        case VET_UINT2:
            return sizeof(unsigned int)*2;
        case VET_UINT3:
            return sizeof(unsigned int)*3;
        case VET_UINT4:
            return sizeof(unsigned int)*4;
        case VET_UBYTE4:
            return sizeof(unsigned char)*4;
    }
    return 0;
}

String VertexElement::TypeToString()
{
    return TypeToString(type);
}

String VertexElement::TypeToString(Type type)
{
    switch(type)
    {
        case VET_COLOUR:        return "COLOUR";
        case VET_COLOUR_ABGR:    return "COLOUR_ABGR";
        case VET_COLOUR_ARGB:    return "COLOUR_ARGB";
        case VET_FLOAT1:        return "FLOAT1";
        case VET_FLOAT2:        return "FLOAT2";
        case VET_FLOAT3:        return "FLOAT3";
        case VET_FLOAT4:        return "FLOAT4";
        case VET_DOUBLE1:        return "DOUBLE1";
        case VET_DOUBLE2:        return "DOUBLE2";
        case VET_DOUBLE3:        return "DOUBLE3";
        case VET_DOUBLE4:        return "DOUBLE4";
        case VET_SHORT1:        return "SHORT1";
        case VET_SHORT2:        return "SHORT2";
        case VET_SHORT3:        return "SHORT3";
        case VET_SHORT4:        return "SHORT4";
        case VET_USHORT1:        return "USHORT1";
        case VET_USHORT2:        return "USHORT2";
        case VET_USHORT3:        return "USHORT3";
        case VET_USHORT4:        return "USHORT4";
        case VET_INT1:            return "INT1";
        case VET_INT2:            return "INT2";
        case VET_INT3:            return "INT3";
        case VET_INT4:            return "INT4";
        case VET_UINT1:            return "UINT1";
        case VET_UINT2:            return "UINT2";
        case VET_UINT3:            return "UINT3";
        case VET_UINT4:            return "UINT4";
        case VET_UBYTE4:        return "UBYTE4";
    }
    return "Unknown_VertexElement::Type";
}

String VertexElement::SemanticToString()
{
    return SemanticToString(semantic);
}

String VertexElement::SemanticToString(Semantic semantic)
{
    switch(semantic)
    {
        case VES_POSITION:                return "POSITION";
        case VES_BLEND_WEIGHTS:            return "BLEND_WEIGHTS";
        case VES_BLEND_INDICES:            return "BLEND_INDICES";
        case VES_NORMAL:                return "NORMAL";
        case VES_DIFFUSE:                return "DIFFUSE";
        case VES_SPECULAR:                return "SPECULAR";
        case VES_TEXTURE_COORDINATES:    return "TEXTURE_COORDINATES";
        case VES_BINORMAL:                return "BINORMAL";
        case VES_TANGENT:                return "TANGENT";
    }
    return "Unknown_VertexElement::Semantic";
}

// IVertexData

IVertexData::IVertexData() :
    count(0)
{
}

bool IVertexData::HasBoneAssignments() const
{
    return !boneAssignments.Empty();
}

void IVertexData::AddVertexMapping(uint oldIndex, uint newIndex)
{
    BoneAssignmentsForVertex(oldIndex, newIndex, boneAssignmentsMap[newIndex]);
    vertexIndexMapping[oldIndex].Push(newIndex);
}

void IVertexData::BoneAssignmentsForVertex(uint currentIndex, uint newIndex, VertexBoneAssignmentList &dest) const
{
    for (VertexBoneAssignmentList::ConstIterator iter=boneAssignments.Begin(), end=boneAssignments.End();
        iter!=end; ++iter)
    {
        if (iter->vertexIndex == currentIndex)
        {
            VertexBoneAssignment a = (*iter);
            a.vertexIndex = newIndex;
            dest.Push(a);
        }
    }
}

HashSet<u16> IVertexData::ReferencedBonesByWeights() const
{
    HashSet<u16> referenced;
    for (VertexBoneAssignmentList::ConstIterator iter=boneAssignments.Begin(), end=boneAssignments.End();
        iter!=end; ++iter)
    {
        referenced.Insert(iter->boneIndex);
    }
    return referenced;
}

// VertexData

VertexData::VertexData()
{
}

VertexData::~VertexData()
{
    Reset();
}

void VertexData::Reset()
{
    // Releases shared ptr memory streams.
    vertexBindings.Clear();
    vertexElements.Clear();
}

uint VertexData::VertexSize(u16 source) const
{
    uint size = 0;
    for(VertexElementList::ConstIterator iter=vertexElements.Begin(), end=vertexElements.End(); iter != end; ++iter)
    {
        if (iter->source == source)
            size += iter->Size();
    }
    return size;
}

Vector<u8> *VertexData::VertexBuffer(u16 source)
{
    if (vertexBindings.Find(source) != vertexBindings.End())
        return &vertexBindings[source];
    return 0;
}

VertexElement *VertexData::GetVertexElement(VertexElement::Semantic semantic, u16 index)
{
    for(VertexElementList::Iterator iter=vertexElements.Begin(), end=vertexElements.End(); iter != end; ++iter)
    {
        VertexElement &element = (*iter);
        if (element.semantic == semantic && element.index == index)
            return &element;
    }
    return 0;
}


// IndexData

IndexData::IndexData() :
    count(0),
    faceCount(0),
    is32bit(false)
{
}

IndexData::~IndexData()
{
    Reset();    
}

void IndexData::Reset()
{
    buffer.Clear();
}

uint IndexData::IndexSize() const
{
    return (is32bit ? sizeof(uint) : sizeof(u16));
}

uint IndexData::FaceSize() const
{
    return IndexSize() * 3;
}

// Mesh

Mesh::Mesh() :
    sharedVertexData(0),
    skeleton(0),
    hasSkeletalAnimations(false)
{
}

Mesh::~Mesh()
{
    Reset();
}

void Mesh::Reset()
{
    SAFE_DELETE(skeleton)
    SAFE_DELETE(sharedVertexData)

    for(uint i=0, len=subMeshes.Size(); i<len; ++i) {
        SAFE_DELETE(subMeshes[i])
    }
    subMeshes.Clear();
    for(uint i=0, len=animations.Size(); i<len; ++i) {
        SAFE_DELETE(animations[i])
    }
    animations.Clear();
    for(uint i=0, len=poses.Size(); i<len; ++i) {
        SAFE_DELETE(poses[i])
    }
    poses.Clear();
}

uint Mesh::NumSubMeshes() const
{
    return subMeshes.Size();
}

SubMesh *Mesh::GetSubMesh(u16 index) const
{
    for(uint i=0; i<subMeshes.Size(); ++i)
        if (subMeshes[i]->index == index)
            return subMeshes[i];
    return 0;
}

// ISubMesh

ISubMesh::ISubMesh() :
    index(0),
    materialIndex(-1),
    usesSharedVertexData(false),
    operationType(OT_POINT_LIST)
{
}

// SubMesh

SubMesh::SubMesh() :
    vertexData(0),
    indexData(new IndexData())
{
}

SubMesh::~SubMesh()
{
    Reset();
}

void SubMesh::Reset()
{
    SAFE_DELETE(vertexData)
    SAFE_DELETE(indexData)
}

// Animation

Animation::Animation(Skeleton *parent) :
    parentSkeleton(parent),
    parentMesh(0),
    length(0.0f),
    baseTime(-1.0f)
{
}

Animation::Animation(Mesh *parent) : 
    parentMesh(parent),
    parentSkeleton(0),
    length(0.0f),
    baseTime(-1.0f)
{
}

VertexData *Animation::AssociatedVertexData(VertexAnimationTrack *track) const
{
    if (!parentMesh)
        return 0;

    bool sharedGeom = (track->target == 0);
    if (sharedGeom)
        return parentMesh->sharedVertexData;
    else
        return parentMesh->GetSubMesh(track->target-1)->vertexData;
}

// Skeleton

Skeleton::Skeleton() :
    blendMode(ANIMBLEND_AVERAGE)
{
}

Skeleton::~Skeleton()
{
    Reset();
}

void Skeleton::Reset()
{
    for(uint i=0, len=bones.Size(); i<len; ++i) {
        SAFE_DELETE(bones[i])
    }
    bones.Clear();
    for(uint i=0, len=animations.Size(); i<len; ++i) {
        SAFE_DELETE(animations[i])
    }
    animations.Clear();
}

BoneList Skeleton::RootBones() const
{
    BoneList rootBones;
    for(BoneList::ConstIterator iter = bones.Begin(); iter != bones.End(); ++iter)
    {
        if (!(*iter)->IsParented())
            rootBones.Push((*iter));
    }
    return rootBones;
}

uint Skeleton::NumRootBones() const
{
    uint num = 0;
    for(BoneList::ConstIterator iter = bones.Begin(); iter != bones.End(); ++iter)
    {
        if (!(*iter)->IsParented())
            num++;
    }
    return num;
}

Bone *Skeleton::BoneByName(const String &name) const
{
    for(BoneList::ConstIterator iter = bones.Begin(); iter != bones.End(); ++iter)
    {
        if ((*iter)->name == name)
            return (*iter);
    }
    return 0;
}

Bone *Skeleton::BoneById(u16 id) const
{
    for(BoneList::ConstIterator iter = bones.Begin(); iter != bones.End(); ++iter)
    {
        if ((*iter)->id == id)
            return (*iter);
    }
    return 0;
}

// Bone

Bone::Bone() :
    id(0),
    parent(0),
    parentId(-1),
    scale(1.0f, 1.0f, 1.0f)
{
}

bool Bone::IsParented() const
{
    return (parentId != -1 && parent != 0);
}

u16 Bone::ParentId() const
{
    return static_cast<u16>(parentId);
}

void Bone::AddChild(Bone *bone)
{
    if (!bone)
        return;
    if (bone->IsParented())
    {
        LogError("Attaching child Bone that is already parented: " + bone->name);
        return;
    }
    bone->parent = this;
    bone->parentId = id;
    children.push_back(bone->id);
}

void Bone::CalculateWorldMatrixAndDefaultPose(Skeleton *skeleton)
{
    worldMatrix = float4x4::Translate(position) * float4x4::FromQuat(rotation) * float4x4::Scale(scale);
    if (IsParented())
        worldMatrix = worldMatrix * parent->worldMatrix;

    defaultPose = worldMatrix;

    if (IsParented())
        worldMatrix = parent->worldMatrix * worldMatrix;

    // Recursively for all children now that the parent matrix has been calculated.
    for (uint i=0, len=children.size(); i<len; ++i)
    {
        Bone *child = skeleton->BoneById(children[i]);
        if (!child) {
            LogError("CalculateWorldMatrixAndDefaultPose: Failed to find child bone " + String(children[i]) + " for parent " + String(id) + " " + name);
            return;
        }
        child->CalculateWorldMatrixAndDefaultPose(skeleton);
    }
}

// VertexAnimationTrack

VertexAnimationTrack::VertexAnimationTrack() :
    target(0),
    type(VAT_NONE)
{
}

// TransformKeyFrame

TransformKeyFrame::TransformKeyFrame() : 
    timePos(0.0f),
    scale(1.0f, 1.0f, 1.0f)
{
}

float4x4 TransformKeyFrame::Transform()
{
    return float4x4::Translate(position) * float4x4::FromQuat(rotation) * float4x4::Scale(scale);
}

} // Ogre

} // Assimp

