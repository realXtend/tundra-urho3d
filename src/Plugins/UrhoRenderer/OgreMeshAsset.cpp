// For conditions of distribution and use, see copyright notice in LICENSE

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
#include "Renderer.h"
#include "AssetAPI.h"
#include "AssetCache.h"
#include "Profiler.h"
#include "LoggingFunctions.h"
#include "OgreMeshAsset.h"
#include "OgreStructs.h"

#include <Model.h>
#include <Profiler.h>
#include <MemoryBuffer.h>
#include <VertexBuffer.h>
#include <IndexBuffer.h>
#include <Geometry.h>

#include <cstring>
#include <stdexcept>

namespace Tundra
{

using namespace Ogre;

const String            MESH_VERSION_1_8        = "[MeshSerializer_v1.8]";

const unsigned short    HEADER_CHUNK_ID         = 0x1000;

const long              MSTREAM_OVERHEAD_SIZE   = sizeof(u16) + sizeof(uint);

static u32 currentLength;

void ReadMesh(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
void ReadMeshLodInfo(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
void ReadMeshSkeletonLink(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
void ReadMeshBounds(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
void ReadMeshExtremes(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
void ReadSubMesh(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
void ReadSubMeshNames(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
void ReadSubMeshOperation(Urho3D::Deserializer& stream, SubMesh *submesh);
void ReadSubMeshTextureAlias(Urho3D::Deserializer& stream, SubMesh *submesh);
void ReadBoneAssignment(Urho3D::Deserializer& stream, VertexData *dest);
void ReadGeometry(Urho3D::Deserializer& stream, VertexData *dest);
void ReadGeometryVertexDeclaration(Urho3D::Deserializer& stream, VertexData *dest);
void ReadGeometryVertexElement(Urho3D::Deserializer& stream, VertexData *dest);
void ReadGeometryVertexBuffer(Urho3D::Deserializer& stream, VertexData *dest);
void ReadEdgeList(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
void ReadPoses(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
void ReadPoseVertices(Urho3D::Deserializer& stream, Pose *pose);
void ReadAnimations(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
void ReadAnimation(Urho3D::Deserializer& stream, Animation *anim);
void ReadAnimationKeyFrames(Urho3D::Deserializer& stream, Animation *anim, VertexAnimationTrack *track);
void NormalizeBoneWeights(VertexData *vertexData);

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

static void RollbackHeader(Urho3D::Deserializer& stream)
{
    stream.Seek(stream.GetPosition() - MSTREAM_OVERHEAD_SIZE);
}

static void SkipBytes(Urho3D::Deserializer& stream, uint numBytes)
{
    stream.Seek(stream.GetPosition() + numBytes);
}

void ReadMesh(Urho3D::Deserializer& stream, Ogre::Mesh *mesh)
{
    mesh->hasSkeletalAnimations = stream.ReadBool();

    if (!stream.IsEof())
    {
        u16 id = ReadHeader(stream);
        while (!stream.IsEof() &&
            (id == M_GEOMETRY ||
             id == M_SUBMESH ||
             id == M_MESH_SKELETON_LINK ||
             id == M_MESH_BONE_ASSIGNMENT ||
             id == M_MESH_LOD ||
             id == M_MESH_BOUNDS ||
             id == M_SUBMESH_NAME_TABLE ||
             id == M_EDGE_LISTS ||
             id == M_POSES ||
             id == M_ANIMATIONS ||
             id == M_TABLE_EXTREMES))
        {
            switch(id)
            {
                case M_GEOMETRY:
                {
                    mesh->sharedVertexData = new VertexData();
                    ReadGeometry(stream, mesh->sharedVertexData);
                    break;
                }
                case M_SUBMESH:
                {
                    ReadSubMesh(stream, mesh);
                    break;
                }
                case M_MESH_SKELETON_LINK:
                {
                    ReadMeshSkeletonLink(stream, mesh);
                    break;
                }
                case M_MESH_BONE_ASSIGNMENT:
                {
                    ReadBoneAssignment(stream, mesh->sharedVertexData);
                    break;
                }
                case M_MESH_LOD:
                {
                    ReadMeshLodInfo(stream, mesh);
                    break;
                }
                case M_MESH_BOUNDS:
                {
                    ReadMeshBounds(stream, mesh);
                    break;
                }
                case M_SUBMESH_NAME_TABLE:
                {
                    ReadSubMeshNames(stream, mesh);
                    break;
                }
                case M_EDGE_LISTS:
                {
                    ReadEdgeList(stream, mesh);
                    break;
                }
                case M_POSES:
                {
                    ReadPoses(stream, mesh);
                    break;
                }
                case M_ANIMATIONS:
                {
                    ReadAnimations(stream, mesh);
                    break;
                }
                case M_TABLE_EXTREMES:
                {
                    ReadMeshExtremes(stream, mesh);
                    break;
                }
            }

            if (!stream.IsEof())
                id = ReadHeader(stream);
        }
        if (!stream.IsEof())
            RollbackHeader(stream);
    }

    NormalizeBoneWeights(mesh->sharedVertexData);
}

void ReadMeshLodInfo(Urho3D::Deserializer& stream, Ogre::Mesh *mesh)
{
    // Assimp does not acknowledge LOD levels as far as I can see it. This info is just skipped.
    // @todo Put this stuff to scene/mesh custom properties. If manual mesh the app can use the information.
    ReadLine(stream); // strategy name
    u16 numLods = stream.ReadUShort();
    bool manual = stream.ReadBool();
    
    /// @note Main mesh is considered as LOD 0, start from index 1.
    for (uint i=1; i<numLods; ++i)
    {
        u16 id = ReadHeader(stream);
        if (id != M_MESH_LOD_USAGE) {
            throw std::runtime_error("M_MESH_LOD does not contain a M_MESH_LOD_USAGE for each LOD level");
        }

        SkipBytes(stream, sizeof(float)); // User value

        if (manual)
        {
            id = ReadHeader(stream);
            if (id != M_MESH_LOD_MANUAL) {
                throw std::runtime_error("Manual M_MESH_LOD_USAGE does not contain M_MESH_LOD_MANUAL");
            }
                
            ReadLine(stream); // manual mesh name (ref to another mesh)
        }
        else
        {
            for(uint si=0, silen=mesh->NumSubMeshes(); si<silen; ++si)
            {
                id = ReadHeader(stream);
                if (id != M_MESH_LOD_GENERATED) {
                    throw std::runtime_error("Generated M_MESH_LOD_USAGE does not contain M_MESH_LOD_GENERATED");
                }

                uint indexCount = stream.ReadUInt();
                bool is32bit = stream.ReadBool();

                if (indexCount > 0)
                {
                    uint len = indexCount * (is32bit ? sizeof(uint) : sizeof(u16));
                    SkipBytes(stream, len);
                }
            }
        }
    }
}

void ReadMeshSkeletonLink(Urho3D::Deserializer& stream, Ogre::Mesh *mesh)
{
    mesh->skeletonRef = ReadLine(stream);
}

void ReadMeshBounds(Urho3D::Deserializer& stream, Ogre::Mesh *mesh)
{
    // 2x float vec3 + 1x float sphere radius
    mesh->min = float3(stream.ReadVector3());
    mesh->max = float3(stream.ReadVector3());
    SkipBytes(stream, sizeof(float));
}

void ReadMeshExtremes(Urho3D::Deserializer& stream, Ogre::Mesh * /*mesh*/)
{
    // Skip extremes, not compatible with Assimp.
    uint numBytes = currentLength - MSTREAM_OVERHEAD_SIZE; 
    SkipBytes(stream, numBytes);
}

void ReadBoneAssignment(Urho3D::Deserializer& stream, VertexData *dest)
{
    if (!dest) {
        throw std::runtime_error("Cannot read bone assignments, vertex data is null.");
    }
        
    VertexBoneAssignment ba;
    ba.vertexIndex = stream.ReadUInt();
    ba.boneIndex = stream.ReadUShort();
    ba.weight = stream.ReadFloat();

    dest->boneAssignments.Push(ba);
}

void ReadSubMesh(Urho3D::Deserializer& stream, Ogre::Mesh *mesh)
{
    u16 id = 0;
    
    SubMesh *submesh = new SubMesh();
    submesh->materialRef = ReadLine(stream);
    submesh->usesSharedVertexData = stream.ReadBool();

    submesh->indexData->count = stream.ReadUInt();
    submesh->indexData->faceCount = static_cast<uint>(submesh->indexData->count / 3);
    submesh->indexData->is32bit = stream.ReadBool();

    // Index buffer
    if (submesh->indexData->count > 0)
    {
        uint numBytes = submesh->indexData->count * (submesh->indexData->is32bit ? sizeof(uint) : sizeof(u16));
        submesh->indexData->buffer.Resize(numBytes);
        if (numBytes)
            stream.Read(&submesh->indexData->buffer[0], numBytes);
    }
    
    // Vertex buffer if not referencing the shared geometry
    if (!submesh->usesSharedVertexData)
    {
        id = ReadHeader(stream);
        if (id != M_GEOMETRY) {
            throw std::runtime_error("M_SUBMESH does not contain M_GEOMETRY, but shared geometry is set to false");
        }

        submesh->vertexData = new VertexData();
        ReadGeometry(stream, submesh->vertexData);
    }
    
    // Bone assignment, submesh operation and texture aliases
    if (!stream.IsEof())
    {
        id = ReadHeader(stream);
        while (!stream.IsEof() &&
            (id == M_SUBMESH_OPERATION ||
             id == M_SUBMESH_BONE_ASSIGNMENT ||
             id == M_SUBMESH_TEXTURE_ALIAS))
        {
            switch(id)
            {
                case M_SUBMESH_OPERATION:
                {
                    ReadSubMeshOperation(stream, submesh);
                    break;
                }
                case M_SUBMESH_BONE_ASSIGNMENT:
                {
                    ReadBoneAssignment(stream, submesh->vertexData);
                    break;
                }
                case M_SUBMESH_TEXTURE_ALIAS:
                {
                    ReadSubMeshTextureAlias(stream, submesh);
                    break;
                }
            }

            if (!stream.IsEof())
                id = ReadHeader(stream);
        }
        if (!stream.IsEof())
            RollbackHeader(stream);
    }

    NormalizeBoneWeights(submesh->vertexData);

    submesh->index = mesh->subMeshes.Size();
    mesh->subMeshes.Push(submesh);
}

void NormalizeBoneWeights(VertexData *vertexData)
{
    if (!vertexData || vertexData->boneAssignments.Empty())
        return;

    HashSet<uint> influencedVertices;
    for (VertexBoneAssignmentList::ConstIterator baIter=vertexData->boneAssignments.Begin(), baEnd=vertexData->boneAssignments.End(); baIter != baEnd; ++baIter) {
        influencedVertices.Insert(baIter->vertexIndex);
    }

    /** Normalize bone weights.
        Some exporters wont care if the sum of all bone weights
        for a single vertex equals 1 or not, so validate here. */
    const float epsilon = 0.05f;
    for(HashSet<uint>::ConstIterator iter=influencedVertices.Begin(), end=influencedVertices.End(); iter != end; ++iter)
    {
        const uint vertexIndex = (*iter);

        float sum = 0.0f;
        for (VertexBoneAssignmentList::ConstIterator baIter=vertexData->boneAssignments.Begin(), baEnd=vertexData->boneAssignments.End(); baIter != baEnd; ++baIter)
        {
            if (baIter->vertexIndex == vertexIndex)
                sum += baIter->weight;
        }
        if ((sum < (1.0f - epsilon)) || (sum > (1.0f + epsilon)))
        {
            for (VertexBoneAssignmentList::Iterator baIter=vertexData->boneAssignments.Begin(), baEnd=vertexData->boneAssignments.End(); baIter != baEnd; ++baIter)
            {
                if (baIter->vertexIndex == vertexIndex)
                    baIter->weight /= sum;
            }
        }
    }
}

void ReadSubMeshOperation(Urho3D::Deserializer& stream, SubMesh *submesh)
{
    submesh->operationType = static_cast<SubMesh::OperationType>(stream.ReadUShort());
}

void ReadSubMeshTextureAlias(Urho3D::Deserializer& stream, SubMesh *submesh)
{
    submesh->textureAliasName = ReadLine(stream);
    submesh->textureAliasRef = ReadLine(stream);
}

void ReadSubMeshNames(Urho3D::Deserializer& stream, Ogre::Mesh *mesh)
{
    u16 id = 0;
    u16 submeshIndex = 0;

    if (!stream.IsEof())
    {
        id = ReadHeader(stream);
        while (!stream.IsEof() && id == M_SUBMESH_NAME_TABLE_ELEMENT)
        {
            submeshIndex = stream.ReadUShort();
            SubMesh *submesh = mesh->GetSubMesh(submeshIndex);
            if (!submesh) {
                throw std::runtime_error("Ogre Mesh does not include submesh referenced in M_SUBMESH_NAME_TABLE_ELEMENT. Invalid mesh file.");
            }

            submesh->name = ReadLine(stream);

            if (!stream.IsEof())
                id = ReadHeader(stream);
        }
        if (!stream.IsEof())
            RollbackHeader(stream);
    }
}

void ReadGeometry(Urho3D::Deserializer& stream, VertexData *dest)
{
    dest->count = stream.ReadUInt();
    
    if (!stream.IsEof())
    {
        u16 id = ReadHeader(stream);
        while (!stream.IsEof() &&
            (id == M_GEOMETRY_VERTEX_DECLARATION ||
             id == M_GEOMETRY_VERTEX_BUFFER))
        {
            switch(id)
            {
                case M_GEOMETRY_VERTEX_DECLARATION:
                {
                    ReadGeometryVertexDeclaration(stream, dest);
                    break;
                }
                case M_GEOMETRY_VERTEX_BUFFER:
                {
                    ReadGeometryVertexBuffer(stream, dest);
                    break;
                }
            }

            if (!stream.IsEof())
                id = ReadHeader(stream);
        }
        if (!stream.IsEof())
            RollbackHeader(stream);
    }
}

void ReadGeometryVertexDeclaration(Urho3D::Deserializer& stream, VertexData *dest)
{
    if (!stream.IsEof())
    {
        u16 id = ReadHeader(stream);
        while (!stream.IsEof() && id == M_GEOMETRY_VERTEX_ELEMENT)
        {
            ReadGeometryVertexElement(stream, dest);

            if (!stream.IsEof())
                id = ReadHeader(stream);
        }
        if (!stream.IsEof())
            RollbackHeader(stream);
    }
}

void ReadGeometryVertexElement(Urho3D::Deserializer& stream, VertexData *dest)
{
    VertexElement element;
    element.source = stream.ReadUShort();
    element.type = static_cast<VertexElement::Type>(stream.ReadUShort());
    element.semantic = static_cast<VertexElement::Semantic>(stream.ReadUShort());
    element.offset = stream.ReadUShort();
    element.index = stream.ReadUShort();

    dest->vertexElements.Push(element);
}

void ReadGeometryVertexBuffer(Urho3D::Deserializer& stream, VertexData *dest)
{
    u16 bindIndex = stream.ReadUShort();
    u16 vertexSize = stream.ReadUShort();
    
    u16 id = ReadHeader(stream);
    if (id != M_GEOMETRY_VERTEX_BUFFER_DATA)
    {
        throw std::runtime_error("M_GEOMETRY_VERTEX_BUFFER_DATA not found in M_GEOMETRY_VERTEX_BUFFER");
    }
    if (dest->VertexSize(bindIndex) != vertexSize)
    {
        throw std::runtime_error("Vertex buffer size does not agree with vertex declaration in M_GEOMETRY_VERTEX_BUFFER");
    }
    uint numBytes = dest->count * vertexSize;
    dest->vertexBindings[bindIndex].Resize(numBytes);
    if (numBytes)
        stream.Read(&dest->vertexBindings[bindIndex][0], numBytes);
}

void ReadEdgeList(Urho3D::Deserializer& stream, Ogre::Mesh * /*mesh*/)
{
    if (!stream.IsEof())
    {
        u16 id = ReadHeader(stream);
        while (!stream.IsEof() && id == M_EDGE_LIST_LOD)
        {
            SkipBytes(stream, sizeof(u16)); // lod index
            bool manual = stream.ReadBool();

            if (!manual)
            {
                SkipBytes(stream, sizeof(u8));
                uint numTriangles = stream.ReadUInt();
                uint numEdgeGroups = stream.ReadUInt();
                
                uint skipBytes = (sizeof(uint) * 8 + sizeof(float) * 4) * numTriangles;
                SkipBytes(stream, skipBytes);

                for (uint i=0; i<numEdgeGroups; ++i)
                {
                    u16 id = ReadHeader(stream);
                    if (id != M_EDGE_GROUP)
                    {
                        throw std::runtime_error("M_EDGE_GROUP not found in M_EDGE_LIST_LOD");
                    }
                        
                    SkipBytes(stream, sizeof(uint) * 3);
                    uint numEdges = stream.ReadUInt();
                    for (uint j=0; j<numEdges; ++j)
                    {
                        SkipBytes(stream, sizeof(uint) * 6 + sizeof(u8));
                    }
                }
            }

            if (!stream.IsEof())
                id = ReadHeader(stream);
        }
        if (!stream.IsEof())
            RollbackHeader(stream);
    }
}

void ReadPoses(Urho3D::Deserializer& stream, Ogre::Mesh *mesh)
{
    if (!stream.IsEof())
    {
        u16 id = ReadHeader(stream);
        while (!stream.IsEof() && id == M_POSE)
        {
            Pose *pose = new Pose();
            pose->name = ReadLine(stream);
            pose->target = stream.ReadUShort();
            pose->hasNormals = stream.ReadBool();

            ReadPoseVertices(stream, pose);
            
            mesh->poses.Push(pose);

            if (!stream.IsEof())
                id = ReadHeader(stream);
        }
        if (!stream.IsEof())
            RollbackHeader(stream);
    }
}

void ReadPoseVertices(Urho3D::Deserializer& stream, Pose *pose)
{
    if (!stream.IsEof())
    {
        u16 id = ReadHeader(stream);
        while (!stream.IsEof() && id == M_POSE_VERTEX)
        {
            Pose::Vertex v;
            v.index = stream.ReadUInt();
            v.offset = float3(stream.ReadVector3());
            if (pose->hasNormals)
                v.normal = float3(stream.ReadVector3());

            pose->vertices[v.index] = v;

            if (!stream.IsEof())
                id = ReadHeader(stream);
        }
        if (!stream.IsEof())
            RollbackHeader(stream);
    }
}

void ReadAnimations(Urho3D::Deserializer& stream, Ogre::Mesh *mesh)
{
    if (!stream.IsEof())
    {
        u16 id = ReadHeader(stream);
        while (!stream.IsEof() && id == M_ANIMATION)
        {
            Animation *anim = new Animation(mesh);
            anim->name = ReadLine(stream);
            anim->length = stream.ReadFloat();
            
            ReadAnimation(stream, anim);

            mesh->animations.Push(anim);

            if (!stream.IsEof())
                id = ReadHeader(stream);
        }
        if (!stream.IsEof())
            RollbackHeader(stream);
    }
}

void ReadAnimation(Urho3D::Deserializer& stream, Animation *anim)
{    
    if (!stream.IsEof())
    {
        u16 id = ReadHeader(stream);
        if (id == M_ANIMATION_BASEINFO)
        {
            anim->baseName = ReadLine(stream);
            anim->baseTime = stream.ReadFloat();

            // Advance to first track
            id = ReadHeader(stream);
        }
        
        while (!stream.IsEof() && id == M_ANIMATION_TRACK)
        {
            VertexAnimationTrack track;
            track.type = static_cast<VertexAnimationTrack::Type>(stream.ReadUShort());
            track.target = stream.ReadUShort();

            ReadAnimationKeyFrames(stream, anim, &track);
            
            anim->tracks.Push(track);

            if (!stream.IsEof())
                id = ReadHeader(stream);
        }
        if (!stream.IsEof())
            RollbackHeader(stream);
    }
}

void ReadAnimationKeyFrames(Urho3D::Deserializer& stream, Animation *anim, VertexAnimationTrack *track)
{
    if (!stream.IsEof())
    {
        u16 id = ReadHeader(stream);
        while (!stream.IsEof() && 
            (id == M_ANIMATION_MORPH_KEYFRAME ||
             id == M_ANIMATION_POSE_KEYFRAME))
        {
            if (id == M_ANIMATION_MORPH_KEYFRAME)
            {
                MorphKeyFrame kf;
                kf.timePos = stream.ReadFloat();
                bool hasNormals = stream.ReadBool();
                
                uint vertexCount = anim->AssociatedVertexData(track)->count;
                uint vertexSize = sizeof(float) * (hasNormals ? 6 : 3);
                uint numBytes = vertexCount * vertexSize;

                kf.buffer.Resize(numBytes);
                if (numBytes)
                    stream.Read(&kf.buffer[0], numBytes);

                track->morphKeyFrames.Push(kf);
            }
            else if (id == M_ANIMATION_POSE_KEYFRAME)
            {
                PoseKeyFrame kf;
                kf.timePos = stream.ReadFloat();
                
                if (!stream.IsEof())
                {
                    id = ReadHeader(stream);
                    while (!stream.IsEof() && id == M_ANIMATION_POSE_REF)
                    {
                        PoseRef pr;
                        pr.index = stream.ReadUShort();
                        pr.influence = stream.ReadFloat();
                        kf.references.Push(pr);
                        
                        if (!stream.IsEof())
                            id = ReadHeader(stream);
                    }
                    if (!stream.IsEof())
                        RollbackHeader(stream);
                }
                
                track->poseKeyFrames.Push(kf);
            }

            if (!stream.IsEof())
                id = ReadHeader(stream);
        }
        if (!stream.IsEof())
            RollbackHeader(stream);
    }
}

Urho3D::PrimitiveType ConvertPrimitiveType(Ogre::ISubMesh::OperationType type)
{
    switch (type)
    {
    case Ogre::ISubMesh::OT_POINT_LIST:
        return Urho3D::POINT_LIST;
    case Ogre::ISubMesh::OT_LINE_LIST:
        return Urho3D::LINE_LIST;
    case Ogre::ISubMesh::OT_LINE_STRIP:
        return Urho3D::LINE_STRIP;
    case Ogre::ISubMesh::OT_TRIANGLE_LIST:
        return Urho3D::TRIANGLE_LIST;
    case Ogre::ISubMesh::OT_TRIANGLE_STRIP:
        return Urho3D::TRIANGLE_STRIP;
    case Ogre::ISubMesh::OT_TRIANGLE_FAN:
        return Urho3D::TRIANGLE_FAN;
    default:
        return Urho3D::TRIANGLE_LIST;
    }
}

struct VertexElementSource
{
    VertexElementSource() :
        enabled(false),
        src(0),
        stride(0)
    {
    }

    void MoveToNext()
    {
        src += (size_t)stride;
    }

    bool enabled;
    u8* src;
    uint stride;
};

void CheckVertexElement(unsigned& elementMask, VertexElementSource* sources, Ogre::VertexData* vertexData, Urho3D::VertexElement urhoElement, Ogre::VertexElement::Semantic ogreSemantic, Ogre::VertexElement::Type ogreType, uint ogreIndex = 0)
{
    VertexElementSource* desc = &sources[urhoElement];

    Ogre::VertexElement* ogreDesc = vertexData->GetVertexElement(ogreSemantic, (u16)ogreIndex);
    if (!ogreDesc)
        return;

    if (ogreDesc->type != ogreType)
    {
        LogWarning("Vertex element " + ogreDesc->SemanticToString() + " found, but type is " + ogreDesc->TypeToString() + " so can not use include in mesh asset");
        return;
    }

    Vector<u8>* ogreVb = vertexData->VertexBuffer(ogreDesc->source);
    if (!ogreVb || !ogreVb->Size())
    {
        LogWarning("Missing or zero-sized Ogre vertex buffer for source " + String(ogreDesc->source) + " used for semantic " + ogreDesc->SemanticToString());
        return;
    }

    elementMask |= 1 << ((uint)urhoElement);
    desc->enabled = true;
    desc->src = &ogreVb->At(0) + (size_t)(ogreDesc->offset);
    desc->stride = 0;

    // To find out the stride in this Ogre source buffer we need to iterate all the vertex elements in that particular buffer
    /// \todo Includes some repetitive work, however is only done once for each vertex element, not for each source
    for (VertexElementList::Iterator iter = vertexData->vertexElements.Begin(), end = vertexData->vertexElements.End(); iter != end; ++iter)
    {
        if (iter->source == ogreDesc->source)
            desc->stride += iter->Size();
    }
}

SharedPtr<Urho3D::VertexBuffer> MakeVertexBuffer(Urho3D::Context* context, Ogre::VertexData* vertexData)
{
    SharedPtr<Urho3D::VertexBuffer> ret;
    if (!vertexData || !vertexData->count)
        return ret;

    ret = new Urho3D::VertexBuffer(context);
    ret->SetShadowed(true); // Allow CPU raycasts and auto-restore on GPU context loss

    unsigned elementMask = 0; // All Ogre's vertex buffers will be combined into one with the proper element order
    VertexElementSource sources[Urho3D::MAX_VERTEX_ELEMENTS];
    /// \todo Check rest, like blend weights / indices
    CheckVertexElement(elementMask, sources, vertexData, Urho3D::ELEMENT_POSITION, Ogre::VertexElement::VES_POSITION, Ogre::VertexElement::VET_FLOAT3);
    CheckVertexElement(elementMask, sources, vertexData, Urho3D::ELEMENT_NORMAL, Ogre::VertexElement::VES_NORMAL, Ogre::VertexElement::VET_FLOAT3);
    CheckVertexElement(elementMask, sources, vertexData, Urho3D::ELEMENT_TEXCOORD1, Ogre::VertexElement::VES_TEXTURE_COORDINATES, Ogre::VertexElement::VET_FLOAT2, 0);
    CheckVertexElement(elementMask, sources, vertexData, Urho3D::ELEMENT_TEXCOORD2, Ogre::VertexElement::VES_TEXTURE_COORDINATES, Ogre::VertexElement::VET_FLOAT2, 1);
    CheckVertexElement(elementMask, sources, vertexData, Urho3D::ELEMENT_TANGENT, Ogre::VertexElement::VES_TANGENT, Ogre::VertexElement::VET_FLOAT4);
    CheckVertexElement(elementMask, sources, vertexData, Urho3D::ELEMENT_COLOR, Ogre::VertexElement::VES_DIFFUSE, Ogre::VertexElement::VET_COLOUR);
    
    ret->SetSize(vertexData->count, elementMask);
    void* data = ret->Lock(0, vertexData->count, true);
    uint count = vertexData->count;

    // Fill all enabled elements with source data, forming interleaved Urho vertices
    float* dest = (float*)data;
    while (count--)
    {
        if (elementMask & Urho3D::MASK_POSITION)
        {
            *dest++ = ((float*)sources[Urho3D::ELEMENT_POSITION].src)[0];
            *dest++ = ((float*)sources[Urho3D::ELEMENT_POSITION].src)[1];
            *dest++ = ((float*)sources[Urho3D::ELEMENT_POSITION].src)[2];
            sources[Urho3D::ELEMENT_POSITION].MoveToNext();
        }
        if (elementMask & Urho3D::MASK_NORMAL)
        {
            *dest++ = ((float*)sources[Urho3D::ELEMENT_NORMAL].src)[0];
            *dest++ = ((float*)sources[Urho3D::ELEMENT_NORMAL].src)[1];
            *dest++ = ((float*)sources[Urho3D::ELEMENT_NORMAL].src)[2];
            sources[Urho3D::ELEMENT_NORMAL].MoveToNext();
        }
        if (elementMask & Urho3D::MASK_COLOR)
        {
            *dest++ = ((float*)sources[Urho3D::ELEMENT_COLOR].src)[0];
            sources[Urho3D::ELEMENT_COLOR].MoveToNext();
        }
        if (elementMask & Urho3D::MASK_TEXCOORD1)
        {
            *dest++ = ((float*)sources[Urho3D::ELEMENT_TEXCOORD1].src)[0];
            *dest++ = ((float*)sources[Urho3D::ELEMENT_TEXCOORD1].src)[1];
            sources[Urho3D::ELEMENT_TEXCOORD1].MoveToNext();
        }
        if (elementMask & Urho3D::MASK_TEXCOORD2)
        {
            *dest++ = ((float*)sources[Urho3D::ELEMENT_TEXCOORD2].src)[0];
            *dest++ = ((float*)sources[Urho3D::ELEMENT_TEXCOORD2].src)[1];
            sources[Urho3D::ELEMENT_TEXCOORD2].MoveToNext();
        }
        if (elementMask & Urho3D::MASK_TANGENT)
        {
            *dest++ = ((float*)sources[Urho3D::ELEMENT_TANGENT].src)[0];
            *dest++ = ((float*)sources[Urho3D::ELEMENT_TANGENT].src)[1];
            *dest++ = ((float*)sources[Urho3D::ELEMENT_TANGENT].src)[2];
            *dest++ = ((float*)sources[Urho3D::ELEMENT_TANGENT].src)[3];
            sources[Urho3D::ELEMENT_TANGENT].MoveToNext();
        }
    }

    ret->Unlock();
    return ret;
}


OgreMeshAsset::OgreMeshAsset(AssetAPI *owner, const String &type_, const String &name_) :
    IMeshAsset(owner, type_, name_)
{
}

bool OgreMeshAsset::DeserializeFromData(const u8 *data_, uint numBytes, bool /*allowAsynchronous*/)
{
    PROFILE(OgreMeshAsset_LoadFromFileInMemory);

    /// Force an unload of previous data first.
    Unload();

    Urho3D::MemoryBuffer buffer(data_, numBytes);

    u16 id = ReadHeader(buffer, false);
    if (id != HEADER_CHUNK_ID)
    {
        LogError("OgreMeshAsset::DeserializeFromData: Invalid Ogre Mesh file header in " + Name());
        return false;
    }

    /// @todo Check what we can actually support.
    String version = ReadLine(buffer);
    /*
    if (version != MESH_VERSION_1_8)
    {
        LogError("OgreMeshAsset::DeserializeFromData: mesh version " + version + " not supported in " + Name());
        return false;
    }
    */

    id = ReadHeader(buffer);
    if (id != M_MESH)
    {
        LogError("OgreMeshAsset::DeserializeFromData: header was not followed by M_MESH chunk in " + Name());
        return false;
    }

    SharedPtr<Ogre::Mesh> mesh(new Ogre::Mesh());
    try
    {
        ReadMesh(buffer, mesh);
    }
    catch (std::exception& e)
    {
        LogError("OgreMeshAsset::DeserializeFromData: " + String(e.what()));
        return false;
    }

    model = new Urho3D::Model(GetContext());
    uint subMeshCount = mesh->NumSubMeshes();
    model->SetNumGeometries(subMeshCount);
    model->SetBoundingBox(Urho3D::BoundingBox(mesh->min, mesh->max));

    SharedPtr<Urho3D::VertexBuffer> sharedVb = MakeVertexBuffer(GetContext(), mesh->sharedVertexData);

    for (uint i = 0; i < subMeshCount; ++i)
    {
        Ogre::SubMesh* subMesh = mesh->subMeshes[i];
        if (!subMesh->indexData)
        {
            LogWarning("OgreMeshAsset::DeserializeFromData: missing index data on submesh " + String(i) + " in " + Name());
            continue;
        }

        SharedPtr<Urho3D::Geometry> geom(new Urho3D::Geometry(GetContext()));
        SharedPtr<Urho3D::IndexBuffer> ib(new Urho3D::IndexBuffer(GetContext()));
        ib->SetShadowed(true); // Allow CPU-side raycasts and auto-restore on GPU context loss
        ib->SetSize(subMesh->indexData->count, subMesh->indexData->is32bit);
        if (ib->GetIndexCount())
            ib->SetData(&subMesh->indexData->buffer[0]);
        geom->SetIndexBuffer(ib);
        geom->SetVertexBuffer(0, subMesh->usesSharedVertexData ? sharedVb : MakeVertexBuffer(GetContext(), subMesh->vertexData));
        geom->SetDrawRange(ConvertPrimitiveType(subMesh->operationType), 0, ib->GetIndexCount());
        model->SetNumGeometryLodLevels(i, 1);
        model->SetGeometry(i, 0, geom);
    }

    /// \todo Handle skinning data, morphs etc.

    assetAPI->AssetLoadCompleted(Name());
    return true;
}

}
