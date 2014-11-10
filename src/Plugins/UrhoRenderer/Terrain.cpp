// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Terrain.h"
#include "GraphicsWorld.h"
#include "Scene/Scene.h"
#include "AttributeMetadata.h"
#include "LoggingFunctions.h"
#include "AssetRefListener.h"
#include "Framework.h"
#include "Profiler.h"
#include "Math/Transform.h"
#include "BinaryAsset.h"
#include "TextureAsset.h"
#include "Placeable.h"

#include <Engine/Scene/Scene.h>
#include <Node.h>
#include <StaticModel.h>
#include <Model.h>
#include <VertexBuffer.h>
#include <IndexBuffer.h>
#include <Geometry.h>
#include <Math/MathFunc.h>


namespace Tundra
{

Terrain::Terrain(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(nodeTransformation, "Transform", Transform(float3(0,0,0),float3(0,0,0),float3(1,1,1))),
    INIT_ATTRIBUTE_VALUE(xPatches, "Grid width", 0),
    INIT_ATTRIBUTE_VALUE(yPatches, "Grid height", 0),
    INIT_ATTRIBUTE_VALUE(uScale, "Tex. U scale", 0.0f),
    INIT_ATTRIBUTE_VALUE(vScale, "Tex. V scale", 0.0f),
    INIT_ATTRIBUTE_VALUE(material, "Material", AssetReference("", "Material")),
    INIT_ATTRIBUTE_VALUE(heightMap, "Heightmap", AssetReference("", "Heightmap"))
{
    patches_.Resize(1);
    MakePatchFlat(0, 0, 0.f);

    materialAsset_ = new AssetRefListener();
    heightMapAsset_ = new AssetRefListener();

    ParentEntitySet.Connect(this, &Terrain::UpdateSignals);
}

Terrain::~Terrain()
{
    if (world_.Expired())
    {
        if (rootNode_ != 0)
            LogError("Mesh: World has expired, skipping uninitialization!");
        return;
    }

    foreach (Patch patch, patches_)
    {
        patch.urhoMesh.Reset();
    }
    
    rootNode_.Reset();
}

void Terrain::UpdateSignals()
{
    Entity* parent = ParentEntity();
    if (!parent)
        return;

    // If scene is not view-enabled, no further action
    if (!ViewEnabled())
        return;
    
    parent->ComponentAdded.Connect(this, &Terrain::OnComponentStructureChanged);
    parent->ComponentRemoved.Connect(this, &Terrain::OnComponentStructureChanged);

    if (parent->ParentScene())
        world_ = parent->ParentScene()->Subsystem<GraphicsWorld>();

    if (world_ && !rootNode_)
    {
        CreateRootNode();
        assert(rootNode_);
       
        materialAsset_->Loaded.Connect(this, &Terrain::OnMaterialAssetLoaded);
        heightMapAsset_->Loaded.Connect(this, &Terrain::OnTerrainAssetLoaded);
    }
}

void Terrain::CreateRootNode()
{
    if (world_ && !rootNode_)
    {
        Urho3D::Scene* urhoScene = world_->UrhoScene();
        rootNode_ = urhoScene->CreateChild("RootNode");
        assert(rootNode_);

        // Add the newly created node to scene or to a parent EC_Placeable.
        AttachTerrainRootNode();

        //UpdateRootNodeTransform();
    }
}

//void Terrain::UpdateRootNodeTransform()
//{
//    if (!rootNode)
//        return;
//
//    const Transform &tm = nodeTransformation.Get();
//
//    Ogre::Matrix3 rot_new;
//    rot_new.FromEulerAnglesXYZ(Ogre::Degree(tm.rot.x), Ogre::Degree(tm.rot.y), Ogre::Degree(tm.rot.z));
//    Ogre::Quaternion q_new(rot_new);
//
//    rootNode->setOrientation(Ogre::Quaternion(rot_new));
//    rootNode->setPosition(tm.pos.x, tm.pos.y, tm.pos.z);
//    rootNode->setScale(tm.scale.x, tm.scale.y, tm.scale.z);
//}

void Terrain::MakePatchFlat(uint x, uint y, float heightValue)
{
    Patch &patch = GetPatch(x, y);
    patch.heightData.Clear();
    patch.heightData.Reserve(cPatchSize * cPatchSize);
    for (size_t i=0 ; i<patch.heightData.Size() ; ++i)
        patch.heightData.Push(heightValue);
    
    patch.patch_geometry_dirty = true;
}

void Terrain::OnComponentStructureChanged(IComponent*, AttributeChange::Type)
{
    AttachTerrainRootNode();
}

void Terrain::AttributesChanged()
{
    // None of the attributes have an effect when the scene is not viewenabled and there is no root node
    if (!rootNode_)
        return;

   
    if (nodeTransformation.ValueChanged())
    {
        const Transform &newTransform = nodeTransformation.Get();
        rootNode_->SetPosition(newTransform.pos);
        rootNode_->SetRotation(newTransform.Orientation());
        rootNode_->SetScale(newTransform.scale);
    }
    //if (xPatches.ValueChanged())
    //    mesh_->SetDrawDistance(drawDistance.Get());
    //if (xPatches.ValueChanged())
    //    mesh_->SetCastShadows(castShadows.Get());
    if (uScale.ValueChanged())
    {
        /// \todo Implement
    }
    if (vScale.ValueChanged())
    {
        /// \todo Implement
    }
    if (heightMap.ValueChanged())
    {
        if (heightMap.Get().ref.Trimmed().Empty())
            LogDebug("Warning: Terrain \"" + this->parentEntity->Name() + "\" heightmap ref was set to an empty reference!");
        heightMapAsset_->HandleAssetRefChange(&heightMap);
    }
    if (material.ValueChanged())
    {
        /// \todo Implement
    }
    
}

void Terrain::OnMaterialAssetLoaded(AssetPtr asset_)
{
    /// \todo implement
}

void Terrain::OnTerrainAssetLoaded(AssetPtr asset_)
{
    BinaryAssetPtr assetData = Urho3D::DynamicCast<BinaryAsset>(asset_);
    TextureAssetPtr textureData = Urho3D::DynamicCast<TextureAsset>(asset_);
    if ((!assetData.Get() || assetData->data.Size() == 0) && !textureData)
    {
        LogError("Failed to load terrain asset from file!");
        return;
    }

    if (assetData)
        LoadFromDataInMemory((const char*)&assetData->data[0], assetData->data.Size());

    /// \todo allow terrain loading from image heightmap
    //if (textureData)
    //{
    //    if (textureData->DiskSource().Empty())
    //    {
    //        LogError("Tried to load terrain from texture file \"" + textureData->Name() + "\", but this requires a disk source of the asset for the load to work properly!");
    //        return;
    //    }
    //    std::map<String, String> args = ParseAssetRefArgs(heightMap.Get().ref, 0);
    //    float scale = 30.f;
    //    float offset = 0.f;
    //    if (args.find("scale") != args.end())
    //        scale = ToFloat(args["scale"]);
    //    if (args.find("offset") != args.end())
    //        offset = ToFloat(args["offset"]);
    //    ///\todo Add support for converting the loaded image file directly to a .ntf asset and refer to that one.
    //    bool success = LoadFromImageFile(textureData->DiskSource(), offset, scale);
    //    if (!success)
    //    {
    //        LogError("Failed to load terrain from texture source \"" + textureData->Name() + "\"! Loading the file \"" + textureData->DiskSource() + "\" failed!");
    //    }
    //}
}

float Terrain::GetPoint(uint x, uint y) const
{
    if (x >= cPatchSize * patchWidth_)
        x = cPatchSize * patchWidth_ - 1;
    if (y >= cPatchSize * patchHeight_)
        y = cPatchSize * patchHeight_ - 1;

    return GetPatch(x / cPatchSize, y / cPatchSize).heightData[(y % cPatchSize) * cPatchSize + (x % cPatchSize)];
}

float3 Terrain::CalculateNormal(uint x, uint y, uint xinside, uint yinside) const
{
    uint px = x * cPatchSize + xinside;
    uint py = y * cPatchSize + yinside;

    uint xNext = Clamp(px+1, 0U, patchWidth_ * cPatchSize - 1);
    uint yNext = Clamp(py+1, 0U, patchHeight_ * cPatchSize - 1);
    uint xPrev = Clamp(px-1, 0U, patchWidth_ * cPatchSize - 1);
    uint yPrev = Clamp(py-1, 0U, patchHeight_ * cPatchSize - 1);

    float x_slope = GetPoint(xPrev, py) - GetPoint(xNext, py);
    if ((px <= 0) || (px >= patchWidth_ * cPatchSize))
        x_slope *= 2;
    float y_slope = GetPoint(px, yPrev) - GetPoint(px, yNext);
    if ((py <= 0) || (py >= patchHeight_ * cPatchSize))
        y_slope *= 2;

    // Note: heightmap X & Y correspond to X & Z world axes, while height is world Y
    return float3(x_slope, 2.0, y_slope).Normalized();
}

u32 ReadU32(const char *dataPtr, size_t numBytes, int &offset)
{
    if (offset + 4 > (int)numBytes)
    {
        LogError("Terrain::ReadU32: Not enough bytes to deserialize!");
        return 0;
    }
    u32 data = *(u32*)(dataPtr + offset); ///@note Requires unaligned load support from the CPU and assumes data storage endianness to be the same for loader and saver.
    offset += 4;
    return data;
}

bool Terrain::LoadFromDataInMemory(const char *data, size_t numBytes)
{
    int offset = 0;
    u32 xPatches = ReadU32(data, numBytes, offset);
    u32 yPatches = ReadU32(data, numBytes, offset);

    // Load all the data from the file to an intermediate buffer first, so that we can first see
    // if the file is not broken, and reject it without losing the old terrain.
    Urho3D::Vector<Patch> newPatches(xPatches * yPatches);

    // Initialize the new height data structure.
    for(u32 y = 0; y < yPatches; ++y)
        for(u32 x = 0; x < xPatches; ++x)
        {
            newPatches[y * xPatches + x].x = x;
            newPatches[y * xPatches + x].y = y;
        }

    assert(sizeof(float) == 4);

    // Load the new data.
    for(size_t i = 0; i < newPatches.Size(); ++i)
    {
        newPatches[i].heightData.Resize(cPatchSize * cPatchSize);
        newPatches[i].patch_geometry_dirty = true;
        if ((offset + cPatchSize * cPatchSize * sizeof(float)) > numBytes)
        {
            LogError("Terrain::LoadFromDataInMemory: Not enough bytes to deserialize!");
            //throw Exception("Not enough bytes to deserialize!");
            return false;
        }

        memcpy(&newPatches[i].heightData[0], data + offset, cPatchSize * cPatchSize * sizeof(float));
        offset += cPatchSize * cPatchSize * sizeof(float);
    }

    // The terrain asset loaded ok. We are good to set that terrain as the active terrain.
    /// \todo enable
//    Destroy();

    patches_ = newPatches;
    patchWidth_ = xPatches;
    patchHeight_ = yPatches;

    // Re-do all the geometry on the GPU.
    RegenerateDirtyTerrainPatches();

    // Set the new number of patches this terrain has. These changes only need to be done locally, since the other
    // peers have loaded the terrain from the same file, and they will also locally do this change. This change is also
    // performed in "batched" mode, i.e. first the values are set, and only after that the signals are emitted manually.
    this->xPatches.Set(patchWidth_, AttributeChange::Disconnected);
    this->yPatches.Set(patchHeight_, AttributeChange::Disconnected);

    this->xPatches.Changed(AttributeChange::LocalOnly);
    this->yPatches.Changed(AttributeChange::LocalOnly);

    return true;
}

void Terrain::RegenerateDirtyTerrainPatches()
{
    PROFILE(Terrain_RegenerateDirtyTerrainPatches);

    Entity *parentEntity = ParentEntity();
    if (!parentEntity)
        return;
    Placeable *position = parentEntity->Component<Placeable>().Get();
    if (!GetFramework()->IsHeadless() && (!position || position->visible.Get())) // Only need to create GPU resources if the placeable itself is visible.
    {
        for(uint y = 0; y < patchHeight_; ++y)
            for(uint x = 0; x < patchWidth_; ++x)
            {
                Terrain::Patch &scenePatch = GetPatch(x, y);
                if (!scenePatch.patch_geometry_dirty || scenePatch.heightData.Size() == 0)
                    continue;

                bool neighborsLoaded = true;

                const int neighbors[8][2] = 
                { 
                    { -1, -1 }, { -1, 0 }, { -1, 1 },
                    {  0, -1 },            {  0, 1 },
                    {  1, -1 }, {  1, 0 }, {  1, 1 }
                };

                for(uint i = 0; i < 8; ++i)
                {
                    uint nX = x + neighbors[i][0];
                    uint nY = y + neighbors[i][1];
                    if (nX < patchWidth_ &&
                        nY < patchHeight_ &&
                        GetPatch(nX, nY).heightData.Size() == 0)
                    {
                        neighborsLoaded = false;
                        break;
                    }
                }

                if (neighborsLoaded)
                    GenerateTerrainGeometryForOnePatch(x, y);
            }
    }
    
    // All the new geometry we created will be visible for Urho3D by default. If the Placeable's visible attribute is false,
    // we need to hide all newly created geometry.
    AttachTerrainRootNode();

    TerrainRegenerated.Emit();
}

void Terrain::AttachTerrainRootNode()
{
    if (world_.Expired()) 
        return;

    //if (!rootNode)
    //{
    //    // CreateRootNode calls this function once the root node has been created.
    //    CreateRootNode();
    //    return;
    //}

    //Ogre::SceneManager *sceneMgr = world_.lock()->OgreSceneManager();

    //// Detach the terrain root node from any previous EC_Placeable scenenode.
    //if (rootNode->getParentSceneNode())
    //    rootNode->getParentSceneNode()->removeChild(rootNode);

    // If this entity has an EC_Placeable, make sure it is the parent of this terrain component.
    SharedPtr<Placeable> pos = (ParentEntity() ? ParentEntity()->Component<Placeable>() : SharedPtr<Placeable>());
    if (pos != nullptr)
    {
        //Ogre::SceneNode *parent = pos->GetSceneNode();
        //parent->addChild(rootNode);
        rootNode_->SetEnabled(pos->visible.Get()); // Re-apply visibility on all the geometry.
    }
    //else
    //{
    //    // No EC_Placeable: the root node is attached to the scene directly.
    //    sceneMgr->getRootSceneNode()->addChild(rootNode);
    //}
}

Urho3D::Node* Terrain::CreateUrhoTerrainPatchNode(Urho3D::Node* parent, uint patchX, uint patchY) const
{
    Urho3D::Node* node = 0;
    if (world_.Expired())
        return node;

    String name = String("Terrain_Patch_") + String(patchX) + "_" + String(patchY);
    node = parent->CreateChild(name);
      //  sceneMgr->createSceneNode(world->GetUniqueObjectName(name));
    if (!node)
        return node;

    const float vertexSpacingX = 1.f;
    const float vertexSpacingY = 1.f;
    const float patchSpacingX = 16 * vertexSpacingX;
    const float patchSpacingY = 16 * vertexSpacingY;
    const Urho3D::Vector3 patchOrigin(patchX * patchSpacingX, 0.f, patchY * patchSpacingY);

    node->SetPosition(patchOrigin);

    return node;
}

void Terrain::GenerateTerrainGeometryForOnePatch(uint patchX, uint patchY)
{
    PROFILE(Terrain_GenerateTerrainGeometryForOnePatch);

    Terrain::Patch &patch = GetPatch(patchX, patchY);

    if (!ViewEnabled())
        return;
    if (world_.Expired())
        return;
//    GraphicsWorldPtr world = world_.Lock();
//    Urho3D::Scene *scene = world->UrhoScene();

    //Urho3D::Node *node = patch.node;
    //bool firstTimeFill = (node == 0);
    //UNREFERENCED_PARAM(firstTimeFill);
    //if (!node)
    //{
    //    CreateOgreTerrainPatchNode(node, patch.x, patch.y);
    //    patch.node = node;
    //}
    //assert(node);

    /// \todo material
    //Ogre::MaterialPtr terrainMaterial = Ogre::MaterialManager::getSingleton().getByName(currentMaterial.toStdString().c_str());
    //if (!terrainMaterial.get()) // If we could not find the material we were supposed to use, just use the default system terrain material.
    //    terrainMaterial = OgreRenderer::GetOrCreateLitTexturedMaterial("Rex/TerrainPCF");

    if (patch.node)
        rootNode_->RemoveChild(patch.node);
    patch.node = CreateUrhoTerrainPatchNode(rootNode_, patch.x, patch.y);//rootNode_->CreateChild("Terrain_patch");
    assert(patch.node);
    patch.urhoMesh = patch.node->CreateComponent<Urho3D::StaticModel>();
    patch.urhoMesh->SetCastShadows(false);
    Urho3D::Model *manual = new Urho3D::Model(GetContext());
    manual->SetNumGeometries(1);

    Urho3D::Vector<unsigned short> indexData;
    Urho3D::Vector<float> vertexData;

    const float vertexSpacingX = 1.f;
    const float vertexSpacingY = 1.f;
    const float patchSpacingX = cPatchSize * vertexSpacingX;
    const float patchSpacingY = cPatchSize * vertexSpacingY;
    const Urho3D::Vector3 patchOrigin(patch.x * patchSpacingX, 0.f, patch.y * patchSpacingY);

    unsigned short curIndex = 0;

    const int cPatchVertexWidth = cPatchSize; // The number of vertices in the patch in horizontal direction. We use the fixed value of cPatchSize==16.
    const int cPatchVertexHeight = cPatchSize; // The number of vertices in the patch in vertical  direction. We use the fixed value of cPatchSize==16.
    // If we assume each patch is 16x16 vertices, then all the internal patches will get a 17x17 grid, since we need to connect seams.
    // But, the outermost patch row and column at the terrain edge will not have this, since they do not need to connect to a next patch.
    // This is the vertex stride for the terrain.
    const unsigned short stride = (patch.x + 1 >= patchWidth_) ? cPatchVertexWidth : (cPatchVertexWidth + 1);

    float3 boundsMin = float3(FLOAT_INF, FLOAT_INF, FLOAT_INF);
    float3 boundsMax = float3(-FLOAT_INF, -FLOAT_INF, -FLOAT_INF);
    const float uScale = this->uScale.Get();
    const float vScale = this->vScale.Get();
    int skip = 0;
    for(int y = 0; y <= cPatchVertexHeight; ++y)
    {
        for(int x = 0; x <= cPatchVertexWidth; ++x)
        {
            if ((patch.x + 1 >= patchWidth_ && x == cPatchVertexWidth) ||
                (patch.y + 1 >= patchHeight_ && y == cPatchVertexHeight))
            {
                skip++;
                continue; // We are at the single corner-most vertex of the whole terrain. That is to be skipped.
            }

            Urho3D::Vector3 pos;
            pos.x_ = vertexSpacingX * x;
            pos.z_ = vertexSpacingY * y;

            Terrain::Patch *thisPatch;
            int X = x;
            int Y = y;
            if (x < cPatchVertexWidth && y < cPatchVertexHeight)
            {
                thisPatch = &patch;

                if ((patch.x + 1 < patchWidth_ || x + 1 < cPatchVertexWidth) &&
                    (patch.y + 1 < patchHeight_ || y + 1 < cPatchVertexHeight))
                {
                    // Note: winding needs to be flipped when terrain X axis goes along world X axis and terrain Y axis along world Z
                    indexData.Push(curIndex + stride);
                    indexData.Push(curIndex + 1);
                    indexData.Push(curIndex);

                    indexData.Push(curIndex + stride);
                    indexData.Push(curIndex + stride + 1);
                    indexData.Push(curIndex + 1);
                }
            }
            else if (x == cPatchVertexWidth && y == cPatchVertexHeight)
            {
                thisPatch = &GetPatch(patch.x + 1, patch.y + 1);
                X = 0;
                Y = 0;
            }
            else if (x == cPatchVertexWidth)
            {
                thisPatch = &GetPatch(patch.x + 1, patch.y);
                X = 0;
            }
            else // (y == patchHeight)
            {
                thisPatch = &GetPatch(patch.x, patch.y + 1);
                Y = 0;
            }

            pos.y_ = thisPatch->heightData[Y * cPatchVertexWidth + X];

            vertexData.Push(pos.x_);
            vertexData.Push(pos.y_);
            vertexData.Push(pos.z_);
            boundsMin = boundsMin.Min(float3(pos));
            boundsMax = boundsMax.Max(float3(pos));

            float3 normal = CalculateNormal(thisPatch->x, thisPatch->y, X, Y);
            vertexData.Push(normal.x);
            vertexData.Push(normal.y);
            vertexData.Push(normal.z);

            vertexData.Push((patchOrigin.x_ + pos.x_) * uScale);
            vertexData.Push((patchOrigin.z_ + pos.z_) * vScale);

            vertexData.Push((float)(patch.x * cPatchSize + x) / (VerticesWidth() - 1));
            vertexData.Push((float)(patch.y * cPatchSize + y) / (VerticesHeight() - 1));

            ++curIndex;
        }
    }
    int numVertices = curIndex;
    //LogInfo("numInd " + String(indexData.Size()) + " " + String(cPatchVertexWidth) + " " + String(cPatchVertexHeight) + " skip " + String(skip));
    //LogInfo("numVertices " + String(numVertices));
    //LogInfo(String("min ") + String(boundsMin.ToString().c_str()) + "  " + String(boundsMax.ToString().c_str()));

    /// \todo is cleanup necessary?
    // If there exists a previously generated GPU Mesh resource, delete it before creating a new one.
    //if (patch.meshGeometryName.Length() > 0)
    //{

    //    try
    //    {
    //        Ogre::MeshManager::getSingleton().remove(patch.meshGeometryName);
    //    }
    //    catch(...) {}
    //}

    SharedPtr<Urho3D::Geometry> geom(new Urho3D::Geometry(GetContext()));
    SharedPtr<Urho3D::IndexBuffer> ib(new Urho3D::IndexBuffer(GetContext()));
    SharedPtr<Urho3D::VertexBuffer> vb(new Urho3D::VertexBuffer(GetContext()));
    
    ib->SetShadowed(true);  // Allow CPU-side raycasts and auto-restore on GPU context loss
    vb->SetShadowed(true); // Allow CPU raycasts and auto-restore on GPU context loss

    ib->SetSize(indexData.Size(), false);
    ib->SetData(&indexData[0]);
    vb->SetSize(numVertices, Urho3D::MASK_POSITION | Urho3D::MASK_NORMAL | Urho3D::MASK_TEXCOORD1 | Urho3D::MASK_TEXCOORD2);
    vb->SetData(&vertexData[0]);
    geom->SetIndexBuffer(ib);
    geom->SetVertexBuffer(0, vb);
    geom->SetDrawRange(Urho3D::TRIANGLE_LIST, 0, ib->GetIndexCount());
    manual->SetNumGeometryLodLevels(0, 1);
    manual->SetNumGeometries(1);
    manual->SetGeometry(0, 0, geom);
    manual->SetBoundingBox(Urho3D::BoundingBox(Urho3D::Vector3(boundsMin), Urho3D::Vector3(boundsMax)));

    patch.urhoMesh->SetModel(manual);

//    LogInfo("index counct " + String(ib->GetIndexCount()));

    /// \todo Add reference to entity and component to Urho node user vars (patch.node->SetVar)
    //patch.node->SetVar("key", Urho3D::Variant(static_cast<IComponent *>(this)));

    //patch.meshGeometryName = world->GetUniqueObjectName("EC_Terrain_patchmesh");
    //Ogre::MeshPtr terrainMesh = manual->convertToMesh(patch.meshGeometryName);

    // Odd: destroyManualObject seems to leave behind a memory leak if we don't call manualObject->clear first.
    //manual->clear();
    //sceneMgr->destroyManualObject(manual);

    /*patch.entity = sceneMgr->createEntity(world->GetUniqueObjectName("EC_Terrain_patchentity"), patch.meshGeometryName);
    patch.entity->setUserAny(Ogre::Any(static_cast<IComponent *>(this)));
    patch.entity->setCastShadows(false);*/
    // Set UserAny also on subentities
    //for(uint i = 0; i < patch.entity->getNumSubEntities(); ++i)
    //    patch.entity->getSubEntity(i)->setUserAny(patch.entity->getUserAny());

    // Explicitly destroy all attached MovableObjects previously bound to this terrain node.
    //Ogre::SceneNode::ObjectIterator iter = node->getAttachedObjectIterator();
    //while(iter.hasMoreElements())
    //{
    //    Ogre::MovableObject *obj = iter.getNext();
    //    sceneMgr->destroyMovableObject(obj);
    //}
    //node->detachAllObjects();
    //// Now attach the new built terrain mesh.
    //node->attachObject(patch.entity);

    patch.patch_geometry_dirty = false;

    AttachTerrainRootNode();
}

}
