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
#include "AssetAPI.h"
#include "AssetCache.h"
#include "LoggingFunctions.h"
#include "OgreMeshAsset.h"
#include "OgreMeshDefines.h"

#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Graphics/Geometry.h>

#include <cstring>
#include <stdexcept>

namespace Tundra
{

using namespace Ogre;

enum MeshChunkId
{
    M_HEADER = 0x1000,
        // char*          version           : Version number check
    M_MESH   = 0x3000,
        // bool skeletallyAnimated   // important flag which affects h/w buffer policies
        // Optional M_GEOMETRY chunk
        M_SUBMESH             = 0x4000, 
            // char* materialName
            // bool useSharedVertices
            // unsigned int indexCount
            // bool indexes32Bit
            // unsigned int* faceVertexIndices (indexCount)
            // OR
            // unsigned short* faceVertexIndices (indexCount)
            // M_GEOMETRY chunk (Optional: present only if useSharedVertices = false)
            M_SUBMESH_OPERATION = 0x4010, // optional, trilist assumed if missing
                // unsigned short operationType
            M_SUBMESH_BONE_ASSIGNMENT = 0x4100,
                // Optional bone weights (repeating section)
                // unsigned int vertexIndex;
                // unsigned short boneIndex;
                // float weight;
            // Optional chunk that matches a texture name to an alias
            // a texture alias is sent to the submesh material to use this texture name
            // instead of the one in the texture unit with a matching alias name
            M_SUBMESH_TEXTURE_ALIAS = 0x4200, // Repeating section
                // char* aliasName;
                // char* textureName;

        M_GEOMETRY          = 0x5000, // NB this chunk is embedded within M_MESH and M_SUBMESH
            // unsigned int vertexCount
            M_GEOMETRY_VERTEX_DECLARATION = 0x5100,
                M_GEOMETRY_VERTEX_ELEMENT = 0x5110, // Repeating section
                    // unsigned short source;      // buffer bind source
                    // unsigned short type;        // VertexElementType
                    // unsigned short semantic; // VertexElementSemantic
                    // unsigned short offset;    // start offset in buffer in bytes
                    // unsigned short index;    // index of the semantic (for colours and texture coords)
            M_GEOMETRY_VERTEX_BUFFER = 0x5200, // Repeating section
                // unsigned short bindIndex;    // Index to bind this buffer to
                // unsigned short vertexSize;    // Per-vertex size, must agree with declaration at this index
                M_GEOMETRY_VERTEX_BUFFER_DATA = 0x5210,
                    // raw buffer data
        M_MESH_SKELETON_LINK = 0x6000,
            // Optional link to skeleton
            // char* skeletonName           : name of .skeleton to use
        M_MESH_BONE_ASSIGNMENT = 0x7000,
            // Optional bone weights (repeating section)
            // unsigned int vertexIndex;
            // unsigned short boneIndex;
            // float weight;
        M_MESH_LOD = 0x8000,
            // Optional LOD information
            // string strategyName;
            // unsigned short numLevels;
            // bool manual;  (true for manual alternate meshes, false for generated)
            M_MESH_LOD_USAGE = 0x8100,
            // Repeating section, ordered in increasing depth
            // NB LOD 0 (full detail from 0 depth) is omitted
            // LOD value - this is a distance, a pixel count etc, based on strategy
            // float lodValue;
                M_MESH_LOD_MANUAL = 0x8110,
                // Required if M_MESH_LOD section manual = true
                // String manualMeshName;
                M_MESH_LOD_GENERATED = 0x8120,
                // Required if M_MESH_LOD section manual = false
                // Repeating section (1 per submesh)
                // unsigned int indexCount;
                // bool indexes32Bit
                // unsigned short* faceIndexes;  (indexCount)
                // OR
                // unsigned int* faceIndexes;  (indexCount)
        M_MESH_BOUNDS = 0x9000,
            // float minx, miny, minz
            // float maxx, maxy, maxz
            // float radius
                
        // Added By DrEvil
        // optional chunk that contains a table of submesh indexes and the names of
        // the sub-meshes.
        M_SUBMESH_NAME_TABLE = 0xA000,
            // Subchunks of the name table. Each chunk contains an index & string
            M_SUBMESH_NAME_TABLE_ELEMENT = 0xA100,
                // short index
                // char* name
        // Optional chunk which stores precomputed edge data                     
        M_EDGE_LISTS = 0xB000,
            // Each LOD has a separate edge list
            M_EDGE_LIST_LOD = 0xB100,
                // unsigned short lodIndex
                // bool isManual            // If manual, no edge data here, loaded from manual mesh
                    // bool isClosed
                    // unsigned long numTriangles
                    // unsigned long numEdgeGroups
                    // Triangle* triangleList
                        // unsigned long indexSet
                        // unsigned long vertexSet
                        // unsigned long vertIndex[3]
                        // unsigned long sharedVertIndex[3] 
                        // float normal[4] 

                    M_EDGE_GROUP = 0xB110,
                        // unsigned long vertexSet
                        // unsigned long triStart
                        // unsigned long triCount
                        // unsigned long numEdges
                        // Edge* edgeList
                            // unsigned long  triIndex[2]
                            // unsigned long  vertIndex[2]
                            // unsigned long  sharedVertIndex[2]
                            // bool degenerate
        // Optional poses section, referred to by pose keyframes
        M_POSES = 0xC000,
            M_POSE = 0xC100,
                // char* name (may be blank)
                // unsigned short target    // 0 for shared geometry, 
                                            // 1+ for submesh index + 1
                // bool includesNormals [1.8+]
                M_POSE_VERTEX = 0xC111,
                    // unsigned long vertexIndex
                    // float xoffset, yoffset, zoffset
                    // float xnormal, ynormal, znormal (optional, 1.8+)
        // Optional vertex animation chunk
        M_ANIMATIONS = 0xD000, 
            M_ANIMATION = 0xD100,
            // char* name
            // float length
            M_ANIMATION_BASEINFO = 0xD105,
            // [Optional] base keyframe information (pose animation only)
            // char* baseAnimationName (blank for self)
            // float baseKeyFrameTime
            M_ANIMATION_TRACK = 0xD110,
                // unsigned short type            // 1 == morph, 2 == pose
                // unsigned short target        // 0 for shared geometry, 
                                                // 1+ for submesh index + 1
                M_ANIMATION_MORPH_KEYFRAME = 0xD111,
                    // float time
                    // bool includesNormals [1.8+]
                    // float x,y,z            // repeat by number of vertices in original geometry
                M_ANIMATION_POSE_KEYFRAME = 0xD112,
                    // float time
                    M_ANIMATION_POSE_REF = 0xD113, // repeat for number of referenced poses
                        // unsigned short poseIndex 
                        // float influence
        // Optional submesh extreme vertex list chink
        M_TABLE_EXTREMES = 0xE000,
        // unsigned short submesh_index;
        // float extremes [n_extremes][3];
};

static const String            MESH_VERSION_1_8        = "[MeshSerializer_v1.8]";

static const unsigned short    HEADER_CHUNK_ID         = 0x1000;

static const long              MSTREAM_OVERHEAD_SIZE   = sizeof(u16) + sizeof(uint);

static u32 currentLength;

static void ReadMesh(Urho3D::Deserializer& stream, Ogre::Mesh *mesh, float version);
static void ReadMeshLodInfo(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
static void ReadMeshSkeletonLink(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
static void ReadMeshBounds(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
static void ReadMeshExtremes(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
static void ReadSubMesh(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
static void ReadSubMeshNames(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
static void ReadSubMeshOperation(Urho3D::Deserializer& stream, SubMesh *submesh);
static void ReadSubMeshTextureAlias(Urho3D::Deserializer& stream, SubMesh *submesh);
static void ReadBoneAssignment(Urho3D::Deserializer& stream, VertexData *dest);
static void ReadGeometry(Urho3D::Deserializer& stream, VertexData *dest);
static void ReadGeometryVertexDeclaration(Urho3D::Deserializer& stream, VertexData *dest);
static void ReadGeometryVertexElement(Urho3D::Deserializer& stream, VertexData *dest);
static void ReadGeometryVertexBuffer(Urho3D::Deserializer& stream, VertexData *dest);
static void ReadEdgeList(Urho3D::Deserializer& stream, Ogre::Mesh *mesh);
static void ReadPoses(Urho3D::Deserializer& stream, Ogre::Mesh *mesh, float version);
static void ReadPoseVertices(Urho3D::Deserializer& stream, Pose *pose);
static void ReadAnimations(Urho3D::Deserializer& stream, Ogre::Mesh *mesh, float version);
static void ReadAnimation(Urho3D::Deserializer& stream, Animation *anim, float version);
static void ReadAnimationKeyFrames(Urho3D::Deserializer& stream, Animation *anim, VertexAnimationTrack *track, float version);

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

static void ReadMesh(Urho3D::Deserializer& stream, Ogre::Mesh *mesh, float version)
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
                    ReadPoses(stream, mesh, version);
                    break;
                }
                case M_ANIMATIONS:
                {
                    ReadAnimations(stream, mesh, version);
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
}

static void ReadMeshLodInfo(Urho3D::Deserializer& stream, Ogre::Mesh *mesh)
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

static void ReadMeshSkeletonLink(Urho3D::Deserializer& stream, Ogre::Mesh *mesh)
{
    mesh->skeletonRef = ReadLine(stream);
}

static void ReadMeshBounds(Urho3D::Deserializer& stream, Ogre::Mesh *mesh)
{
    // 2x float vec3 + 1x float sphere radius
    mesh->min = float3(stream.ReadVector3());
    mesh->max = float3(stream.ReadVector3());
    SkipBytes(stream, sizeof(float));
}

static void ReadMeshExtremes(Urho3D::Deserializer& stream, Ogre::Mesh * /*mesh*/)
{
    // Skip extremes, not compatible with Assimp.
    uint numBytes = currentLength - MSTREAM_OVERHEAD_SIZE; 
    SkipBytes(stream, numBytes);
}

static void ReadBoneAssignment(Urho3D::Deserializer& stream, VertexData *dest)
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

    submesh->index = mesh->subMeshes.Size();
    mesh->subMeshes.Push(submesh);
}

static void ReadSubMeshOperation(Urho3D::Deserializer& stream, SubMesh *submesh)
{
    submesh->operationType = static_cast<SubMesh::OperationType>(stream.ReadUShort());
}

static void ReadSubMeshTextureAlias(Urho3D::Deserializer& stream, SubMesh *submesh)
{
    submesh->textureAliasName = ReadLine(stream);
    submesh->textureAliasRef = ReadLine(stream);
}

static void ReadSubMeshNames(Urho3D::Deserializer& stream, Ogre::Mesh *mesh)
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

static void ReadGeometry(Urho3D::Deserializer& stream, VertexData *dest)
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

static void ReadGeometryVertexDeclaration(Urho3D::Deserializer& stream, VertexData *dest)
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

static void ReadGeometryVertexElement(Urho3D::Deserializer& stream, VertexData *dest)
{
    VertexElement element;
    element.source = stream.ReadUShort();
    element.type = static_cast<VertexElement::Type>(stream.ReadUShort());
    element.semantic = static_cast<VertexElement::Semantic>(stream.ReadUShort());
    element.offset = stream.ReadUShort();
    element.index = stream.ReadUShort();

    dest->vertexElements.Push(element);
}

static void ReadGeometryVertexBuffer(Urho3D::Deserializer& stream, VertexData *dest)
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

static void ReadEdgeList(Urho3D::Deserializer& stream, Ogre::Mesh * /*mesh*/)
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

static void ReadPoses(Urho3D::Deserializer& stream, Ogre::Mesh *mesh, float version)
{
    if (!stream.IsEof())
    {
        u16 id = ReadHeader(stream);
        while (!stream.IsEof() && id == M_POSE)
        {
            Pose *pose = new Pose();
            pose->name = ReadLine(stream);
            pose->target = stream.ReadUShort();
            if (version >= 1.8f)
                pose->hasNormals = stream.ReadBool();
            else
                pose->hasNormals = false;

            ReadPoseVertices(stream, pose);
            
            mesh->poses.Push(pose);

            if (!stream.IsEof())
                id = ReadHeader(stream);
        }
        if (!stream.IsEof())
            RollbackHeader(stream);
    }
}

static void ReadPoseVertices(Urho3D::Deserializer& stream, Pose *pose)
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

static void ReadAnimations(Urho3D::Deserializer& stream, Ogre::Mesh *mesh, float version)
{
    if (!stream.IsEof())
    {
        u16 id = ReadHeader(stream);
        while (!stream.IsEof() && id == M_ANIMATION)
        {
            Animation *anim = new Animation(mesh);
            anim->name = ReadLine(stream);
            anim->length = stream.ReadFloat();
            
            ReadAnimation(stream, anim, version);

            mesh->animations.Push(anim);

            if (!stream.IsEof())
                id = ReadHeader(stream);
        }
        if (!stream.IsEof())
            RollbackHeader(stream);
    }
}

static void ReadAnimation(Urho3D::Deserializer& stream, Animation *anim, float version)
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

            ReadAnimationKeyFrames(stream, anim, &track, version);
            
            anim->tracks.Push(track);

            if (!stream.IsEof())
                id = ReadHeader(stream);
        }
        if (!stream.IsEof())
            RollbackHeader(stream);
    }
}

static void ReadAnimationKeyFrames(Urho3D::Deserializer& stream, Animation *anim, VertexAnimationTrack *track, float version)
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
                bool hasNormals = false;
                if (version >= 1.8f)
                    hasNormals = stream.ReadBool();
                
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

static Urho3D::PrimitiveType ConvertPrimitiveType(Ogre::ISubMesh::OperationType type)
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
        stride(0),
        ogreType(Ogre::VertexElement::VET_FLOAT1)
    {
    }

    void MoveToNext()
    {
        src += (size_t)stride;
    }

    bool enabled;
    u8* src;
    Ogre::VertexElement::Type ogreType;
    uint stride;
};

struct VertexBlendWeights
{
    VertexBlendWeights()
    {
        for (uint i = 0; i < 4; ++i)
        {
            weights[i] = 0.0f;
            indices[i] = 0;
        }
    }

    void AddBoneInfluence(unsigned char index, float weight)
    {
        if (weight <= 0.0f)
            return;

        for (uint i = 0; i < 4; ++i)
        {
            if (weights[i] == 0.0f)
            {
                weights[i] = weight;
                indices[i] = index;
                return;
            }
        }

        // No empty space found, drop the lowest influence
        int lowestIndex = -1;
        float lowestWeight = 0.0f;
        for (uint i = 0; i < 4; ++i)
        {
            if (lowestIndex < 0 || weights[i] < lowestWeight)
            {
                lowestIndex = i;
                lowestWeight = weights[i];
            }
        }

        if (lowestIndex != -1 && weight > lowestWeight)
        {
            weights[lowestIndex] = weight;
            indices[lowestIndex] = index;
        }
    }

    void Normalize()
    {
        float sum = 0.0f;
        for (uint i = 0; i < 4; ++i)
            sum += weights[i];
        
        if (sum == 0.0f)
            return;

        for (uint i = 0; i < 4; ++i)
        {
            if (weights[i] > 0.0f)
                weights[i] /= sum; 
        }
    }

    float weights[4];
    unsigned char indices[4];
};

static void CheckVertexElement(unsigned& elementMask, VertexElementSource* sources, Ogre::VertexData* vertexData, Urho3D::VertexElement urhoElement, Ogre::VertexElement::Semantic ogreSemantic, Ogre::VertexElement::Type ogreType, uint ogreIndex = 0)
{
    VertexElementSource* desc = &sources[urhoElement];

    Ogre::VertexElement* ogreDesc = vertexData->GetVertexElement(ogreSemantic, (u16)ogreIndex);
    if (!ogreDesc)
        return;

    if (ogreDesc->type != ogreType)
    {
        bool typeChangeOk = false;
        // Allow tangents to also be FLOAT3 if needed
        if (ogreDesc->semantic == Ogre::VertexElement::VES_TANGENT && ogreDesc->type == Ogre::VertexElement::VET_FLOAT3)
            typeChangeOk = true;

        if (!typeChangeOk)
        {
            LogWarning("Vertex element " + ogreDesc->SemanticToString() + " found, but type is " + ogreDesc->TypeToString() + " so can not use include in mesh asset");
            return;
        }
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
    desc->ogreType = ogreType;
    desc->stride = 0;

    // To find out the stride in this Ogre source buffer we need to iterate all the vertex elements in that particular buffer
    /// \todo Includes some repetitive work, however is only done once for each vertex element, not for each source
    for (VertexElementList::Iterator iter = vertexData->vertexElements.Begin(), end = vertexData->vertexElements.End(); iter != end; ++iter)
    {
        if (iter->source == ogreDesc->source)
            desc->stride += iter->Size();
    }
}

static SharedPtr<Urho3D::VertexBuffer> MakeVertexBuffer(Urho3D::Context* context, Ogre::VertexData* vertexData, Urho3D::BoundingBox& outBox, PODVector<uint>& localToGlobalBoneMapping, Vector<Urho3D::BoundingBox>& boneBoundingBoxes)
{
    SharedPtr<Urho3D::VertexBuffer> ret;
    if (!vertexData || !vertexData->count)
        return ret;

    ret = new Urho3D::VertexBuffer(context);
    ret->SetShadowed(true); // Allow CPU raycasts and auto-restore on GPU context loss

    unsigned elementMask = 0; // All Ogre's vertex buffers will be combined into one with the proper element order
    VertexElementSource sources[Urho3D::MAX_VERTEX_ELEMENTS];
    CheckVertexElement(elementMask, sources, vertexData, Urho3D::ELEMENT_POSITION, Ogre::VertexElement::VES_POSITION, Ogre::VertexElement::VET_FLOAT3);
    CheckVertexElement(elementMask, sources, vertexData, Urho3D::ELEMENT_NORMAL, Ogre::VertexElement::VES_NORMAL, Ogre::VertexElement::VET_FLOAT3);
    CheckVertexElement(elementMask, sources, vertexData, Urho3D::ELEMENT_TEXCOORD1, Ogre::VertexElement::VES_TEXTURE_COORDINATES, Ogre::VertexElement::VET_FLOAT2, 0);
    CheckVertexElement(elementMask, sources, vertexData, Urho3D::ELEMENT_TEXCOORD2, Ogre::VertexElement::VES_TEXTURE_COORDINATES, Ogre::VertexElement::VET_FLOAT2, 1);
    CheckVertexElement(elementMask, sources, vertexData, Urho3D::ELEMENT_TANGENT, Ogre::VertexElement::VES_TANGENT, Ogre::VertexElement::VET_FLOAT4);
    CheckVertexElement(elementMask, sources, vertexData, Urho3D::ELEMENT_COLOR, Ogre::VertexElement::VES_DIFFUSE, Ogre::VertexElement::VET_COLOUR);
    
    // Skinning data is not contained in Ogre's vertex data, but must be created here manually from vertex bone assignments
    Vector<VertexBlendWeights> blendWeights;
    localToGlobalBoneMapping.Clear();
    HashMap<uint, uint> globalToLocalBoneMapping;
    uint numBones = 0;
    bool bonesExceeded = false;

    if (vertexData->boneAssignments.Size())
    {
        elementMask |= Urho3D::MASK_BLENDWEIGHTS;
        elementMask |= Urho3D::MASK_BLENDINDICES;
        blendWeights.Resize(vertexData->count);
        uint localBone = 0;

        for (VertexBoneAssignmentList::ConstIterator baIter=vertexData->boneAssignments.Begin(), baEnd=vertexData->boneAssignments.End(); baIter != baEnd; ++baIter)
        {
            if (baIter->vertexIndex >= vertexData->count)
            {
                LogWarning("Found out of range vertex index in bone assignments");
                continue;
            }

            uint globalBone = baIter->boneIndex;

            // If vertex is influenced by the bone strongly enough (somewhat arbitrary limit), add to the bone's bounding box
            // Note: these vertices are in model space and need to be transformed into bone space when applying the skeleton
            if (baIter->weight >= 0.33f && sources[Urho3D::ELEMENT_POSITION].enabled)
            {
                if (boneBoundingBoxes.Size() < globalBone + 1)
                    boneBoundingBoxes.Resize(globalBone + 1);
                Urho3D::Vector3 vertex = *((Urho3D::Vector3*)(sources[Urho3D::ELEMENT_POSITION].src + baIter->vertexIndex * sources[Urho3D::ELEMENT_POSITION].stride));
                boneBoundingBoxes[globalBone].Merge(vertex);
            }

            HashMap<uint, uint>::Iterator i = globalToLocalBoneMapping.Find(globalBone);
            if (i == globalToLocalBoneMapping.End())
            {
                if (numBones >= 64)
                {
                    bonesExceeded = true;
                    continue;
                }
                globalToLocalBoneMapping[globalBone] = numBones;
                localToGlobalBoneMapping.Push(globalBone);
                localBone = numBones;
                ++numBones;
            }
            else
                localBone = i->second_;
            
            blendWeights[baIter->vertexIndex].AddBoneInfluence((unsigned char)localBone, baIter->weight);
        }
        for (uint i = 0; i < blendWeights.Size(); ++i)
            blendWeights[i].Normalize();
    }

    if (bonesExceeded)
        LogWarning("Submesh uses more than 64 bones for skinning and may render incorrectly");

    ret->SetSize(vertexData->count, elementMask);
    void* data = ret->Lock(0, vertexData->count, true);
    uint count = vertexData->count;

    // Fill all enabled elements with source data, forming interleaved Urho vertices
    float* dest = (float*)data;
    bool tangentIsFloat4 = sources[Urho3D::ELEMENT_TANGENT].ogreType == Ogre::VertexElement::VET_FLOAT4;
    for (uint index = 0; index < count; ++index)
    {
        if (elementMask & Urho3D::MASK_POSITION)
        {
            *dest++ = ((float*)sources[Urho3D::ELEMENT_POSITION].src)[0];
            *dest++ = ((float*)sources[Urho3D::ELEMENT_POSITION].src)[1];
            *dest++ = ((float*)sources[Urho3D::ELEMENT_POSITION].src)[2];
            outBox.Merge(*(reinterpret_cast<Urho3D::Vector3*>(sources[Urho3D::ELEMENT_POSITION].src)));
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
            if (tangentIsFloat4)
                *dest++ = ((float*)sources[Urho3D::ELEMENT_TANGENT].src)[3];
            else
                *dest++ = 1.0f;
            sources[Urho3D::ELEMENT_TANGENT].MoveToNext();
        }
        if (elementMask & Urho3D::MASK_BLENDWEIGHTS)
        {
            *dest++ = blendWeights[index].weights[0];
            *dest++ = blendWeights[index].weights[1];
            *dest++ = blendWeights[index].weights[2];
            *dest++ = blendWeights[index].weights[3];
            *dest++ = *(reinterpret_cast<float*>(blendWeights[index].indices));
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
    String versionStr = ReadLine(buffer);
    versionStr = versionStr.Substring(versionStr.Find('v') + 1);
    float version = Urho3D::ToFloat(versionStr);

    id = ReadHeader(buffer);
    if (id != M_MESH)
    {
        LogError("OgreMeshAsset::DeserializeFromData: header was not followed by M_MESH chunk in " + Name());
        return false;
    }

    SharedPtr<Ogre::Mesh> mesh(new Ogre::Mesh());
    try
    {
        ReadMesh(buffer, mesh, version);
    }
    catch (std::exception& e)
    {
        LogError("OgreMeshAsset::DeserializeFromData: " + String(e.what()));
        return false;
    }

    model = new Urho3D::Model(GetContext());
    uint subMeshCount = mesh->NumSubMeshes();
    model->SetNumGeometries(subMeshCount);
    Urho3D::BoundingBox bounds;

    Vector<PODVector<uint> > allBoneMappings;
    PODVector<uint> sharedBoneMapping;
    SharedPtr<Urho3D::VertexBuffer> sharedVb = MakeVertexBuffer(GetContext(), mesh->sharedVertexData, bounds, sharedBoneMapping, boneBoundingBoxes);

    Vector<SharedPtr<Urho3D::VertexBuffer> > vbs;
    Vector<SharedPtr<Urho3D::IndexBuffer> > ibs;
    HashMap<int, int> poseVbMapping;
    PODVector<unsigned> morphRangeStarts;
    PODVector<unsigned> morphRangeCounts;
    if (sharedVb)
    {
        vbs.Push(sharedVb);
        poseVbMapping[0] = 0; // Shared VB is always first
    }

    for (uint i = 0; i < subMeshCount; ++i)
    {
        Ogre::SubMesh* subMesh = mesh->subMeshes[i];
        if (!subMesh->indexData)
        {
            LogWarning("OgreMeshAsset::DeserializeFromData: missing index data on submesh " + String(i) + " in " + Name());
            continue;
        }

        PODVector<uint> submeshBoneMapping;
        SharedPtr<Urho3D::Geometry> geom(new Urho3D::Geometry(GetContext()));
        SharedPtr<Urho3D::IndexBuffer> ib(new Urho3D::IndexBuffer(GetContext()));
        ib->SetShadowed(true); // Allow CPU-side raycasts and auto-restore on GPU context loss
        ib->SetSize(subMesh->indexData->count, subMesh->indexData->is32bit);
        if (ib->GetIndexCount())
            ib->SetData(&subMesh->indexData->buffer[0]);
        geom->SetIndexBuffer(ib);
        ibs.Push(ib);
        if (!subMesh->usesSharedVertexData)
        {
            SharedPtr<Urho3D::VertexBuffer> submeshVb = MakeVertexBuffer(GetContext(), subMesh->vertexData, bounds, submeshBoneMapping, boneBoundingBoxes);
            geom->SetVertexBuffer(0, submeshVb);
            poseVbMapping[i+1] = vbs.Size();
            vbs.Push(submeshVb);
        }
        else
        {
            // When shared VB is used, it'll always be the first index
            geom->SetVertexBuffer(0, sharedVb);
        }

        geom->SetDrawRange(ConvertPrimitiveType(subMesh->operationType), 0, ib->GetIndexCount());
        allBoneMappings.Push(subMesh->usesSharedVertexData ? sharedBoneMapping : submeshBoneMapping);
        model->SetNumGeometryLodLevels(i, 1);
        model->SetGeometry(i, 0, geom);
        model->SetGeometryBoneMappings(allBoneMappings);
    }

    model->SetBoundingBox(bounds);

    // Set initial inactive morph ranges. Will be clarified once morph poses are read in
    Vector<Urho3D::ModelMorph> morphs;
    for (uint i = 0; i < vbs.Size(); ++i)
    {
        morphRangeStarts.Push(0);
        morphRangeCounts.Push(0);
    }
    for (uint i = 0; i < mesh->animations.Size(); ++i)
    {
        HashSet<uint> poseIndices;
        Ogre::Animation* anim = mesh->animations[i];
        for (uint t = 0; t < anim->tracks.Size(); ++t)
        {
            if (anim->tracks[t].type != Ogre::VertexAnimationTrack::VAT_POSE)
                continue;
            for (uint kf = 0; kf < anim->tracks[t].poseKeyFrames.Size(); ++kf)
            {
                for (uint r = 0; r < anim->tracks[t].poseKeyFrames[kf].references.Size(); ++r)
                {
                    // Take only full influences
                    if (anim->tracks[t].poseKeyFrames[kf].references[r].influence > 0.999f && anim->tracks[t].poseKeyFrames[kf].references[r].index < mesh->poses.Size())
                        poseIndices.Insert(anim->tracks[t].poseKeyFrames[kf].references[r].index);
                }
            }
        }

        if (poseIndices.Empty())
            continue;

        Urho3D::ModelMorph targetMorph;
        targetMorph.name_ = anim->name;
        targetMorph.nameHash_ = StringHash(targetMorph.name_);
        targetMorph.weight_ = 0.0f;

        for (auto it = poseIndices.Begin(); it != poseIndices.End(); ++it)
        {
            Ogre::Pose* pose = mesh->poses[*it];

            // Destination vertex buffer index
            if (poseVbMapping.Find(pose->target) == poseVbMapping.End())
            {
                LogWarning("OgreMeshAsset::DeserializeFromData: found pose referring to unknown vertex buffer target");
                continue;
            }
            
            uint dest = poseVbMapping[pose->target];
            if (dest >= vbs.Size())
            {
                LogWarning("OgreMeshAsset::DeserializeFromData: found pose referring to out-of-range vertex buffer target");
                continue;
            }

            Urho3D::VertexBufferMorph bufferMorph;
            Urho3D::VectorBuffer morphData;
            bufferMorph.elementMask_ = Urho3D::MASK_POSITION;
            if (pose->hasNormals)
                bufferMorph.elementMask_ |= Urho3D::MASK_NORMAL;
            bool hasOutOfRangeVertices = false;
            uint goodVertices = 0;

            auto v = pose->vertices.Begin();
            while (v != pose->vertices.End())
            {
                if (v->first_ < vbs[dest]->GetVertexCount())
                {
                    morphData.WriteUInt(v->first_);
                    morphData.WriteVector3(v->second_.offset);
                    if (pose->hasNormals)
                        morphData.WriteVector3(v->second_.normal);
    
                    if (!morphRangeCounts[dest])
                    {
                        // Define initial morph vertex range
                        morphRangeStarts[dest] = v->first_;
                        morphRangeCounts[dest] = 1;
                    }
                    else
                    {
                        // Expand morph vertex range
                        uint last = morphRangeStarts[dest] + morphRangeCounts[dest];
    
                        if (v->first_ < morphRangeStarts[dest])
                        {
                            morphRangeStarts[dest] = v->first_;
                            morphRangeCounts[dest] = last - morphRangeStarts[dest];
                        }
                        else if (v->first_ > last)
                        {
                            last = v->first_ + 1;
                            morphRangeCounts[dest] = last - morphRangeStarts[dest];
                        }
                    }

                    ++goodVertices;
                }
                else
                    hasOutOfRangeVertices = true;
                
                ++v;
            }

            if (hasOutOfRangeVertices)
                LogWarning("OgreMeshAsset::DeserializeFromData: pose had references to out-of-range vertices. These have been skipped.");
            bufferMorph.dataSize_ = morphData.GetSize();
            bufferMorph.vertexCount_ = goodVertices;
            if (bufferMorph.dataSize_)
            {
                bufferMorph.morphData_ = Urho3D::SharedArrayPtr<unsigned char>(new unsigned char[bufferMorph.dataSize_]);
                /// \todo Build the data directly to the final array instead of copying
                memcpy(bufferMorph.morphData_.Get(), morphData.GetData(), bufferMorph.dataSize_);
            }
            
            targetMorph.buffers_[dest] = bufferMorph;
        }

        morphs.Push(targetMorph);
    }

    if (morphs.Size())
        model->SetMorphs(morphs);

    // Set the vertex & index buffers so that morph data copying and model saving will work correctly
    model->SetVertexBuffers(vbs, morphRangeStarts, morphRangeCounts);
    model->SetIndexBuffers(ibs);

    assetAPI->AssetLoadCompleted(Name());
    return true;
}

}
