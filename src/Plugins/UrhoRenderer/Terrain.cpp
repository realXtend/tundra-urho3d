// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Terrain.h"
#include "GraphicsWorld.h"
#include "Scene/Scene.h"
#include "AttributeMetadata.h"
#include "LoggingFunctions.h"
#include "AssetRefListener.h"
#include "Framework.h"
#include <Urho3D/Core/Profiler.h>
#include "Math/Transform.h"
#include "BinaryAsset.h"
#include "TextureAsset.h"
#include "IMaterialAsset.h"
#include "Placeable.h"
#include "AssetAPI.h"

#include <Math/MathFunc.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/IO/MemoryBuffer.h>


namespace Tundra
{

Terrain::Terrain(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(nodeTransformation, "Transform", Transform(float3(0,0,0),float3(0,0,0),float3(1,1,1))),
    INIT_ATTRIBUTE_VALUE(xPatches, "Grid width", 0),
    INIT_ATTRIBUTE_VALUE(yPatches, "Grid height", 0),
    INIT_ATTRIBUTE_VALUE(uScale, "Tex. U scale", 0.13f),
    INIT_ATTRIBUTE_VALUE(vScale, "Tex. V scale", 0.13f),
    INIT_ATTRIBUTE_VALUE(material, "Material", AssetReference("", "Material")),
    INIT_ATTRIBUTE_VALUE(heightMap, "Heightmap", AssetReference("", "Heightmap")),
    patchWidth_(1),
    patchHeight_(1)
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

    
    rootNode_->Remove();
    rootNode_.Reset();

    foreach (Patch patch, patches_)
    {
        patch.urhoModel.Reset();
    }
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
        materialAsset_->TransferFailed.Connect(this, &Terrain::OnMaterialAssetFailed);
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
        if (rootNode_)
            AttachTerrainRootNode();

        UpdateRootNodeTransform();
    }
}

void Terrain::UpdateRootNodeTransform()
{
    if (!rootNode_)
        return;

    const Transform &tm = nodeTransformation.Get();

    rootNode_->SetRotation(Urho3D::Quaternion(tm.Orientation()));
    rootNode_->SetPosition(Urho3D::Vector3(tm.pos));
    rootNode_->SetScale(Urho3D::Vector3(tm.scale));
}

void Terrain::ResizeTerrain(uint newPatchWidth, uint newPatchHeight)
{
    URHO3D_PROFILE(Terrain_ResizeTerrain);

    const uint maxPatchSize = 256;
    // Do an artificial limit to a preset N patches per side. (This limit is way too large already for the current terrain vertex LOD management.)
    newPatchWidth = Max(1U, Min(maxPatchSize, newPatchWidth));
    newPatchHeight = Max(1U, Min(maxPatchSize, newPatchHeight));

    if (newPatchWidth == patchWidth_ && newPatchHeight == patchHeight_)
        return;

    // If the width changes, we need to also regenerate the old right-most column to generate the new seams. (If we are shrinking, this is not necessary)
    if (patchWidth_ < newPatchWidth)
        for(uint y = 0; y < patchHeight_; ++y)
            GetPatch(patchWidth_ - 1, y).patch_geometry_dirty = true;

    // If the height changes, we need to also regenerate the old top-most row to generate the new seams. (If we are shrinking, this is not necessary)
    if (patchHeight_ < newPatchHeight)
        for(uint x = 0; x < patchWidth_; ++x)
            GetPatch(x, patchHeight_ - 1).patch_geometry_dirty = true;

    // First delete all the patches that will not be part of the newly-sized terrain (user shrinked the terrain in one or two dimensions)
    for(uint y = newPatchHeight; y < patchHeight_; ++y)
        for(uint x = 0; x < patchWidth_; ++x)
            DestroyPatch(x, y);
    for(uint x = newPatchWidth; x < patchWidth_; ++x) // We hav esome overlap here with above, but it's ok since DestroyPatch is benign.
        for(uint y = 0; y < patchHeight_; ++y)
            DestroyPatch(x, y);

    // Now create the new terrain patch storage and copy the old height values over.
    Vector<Patch> newPatches(newPatchWidth * newPatchHeight);
    for(uint y = 0; y < Min(patchHeight_, newPatchHeight); ++y)
        for(uint x = 0; x < Min(patchWidth_, newPatchWidth); ++x)
            newPatches[y * newPatchWidth + x] = GetPatch(x, y);
    patches_ = newPatches;
    uint oldPatchWidth = patchWidth_;
    uint oldPatchHeight = patchHeight_;
    patchWidth_ = newPatchWidth;
    patchHeight_ = newPatchHeight;

    // Init any new patches to flat planes with the given fixed height.

    const float initialPatchHeight = 0.f;

    for(uint y = oldPatchHeight; y < newPatchHeight; ++y)
        for(uint x = 0; x < patchWidth_; ++x)
            MakePatchFlat(x, y, initialPatchHeight);
    for(uint x = oldPatchWidth; x < newPatchWidth; ++x) // We have some overlap here with above, but it's ok since DestroyPatch is benign.
        for(uint y = 0; y < patchHeight_; ++y)
            MakePatchFlat(x, y, initialPatchHeight);

    // Tell each patch which coordinate in the grid they lie in.
    for(uint y = 0; y < patchHeight_; ++y)
        for(uint x = 0; x < patchWidth_; ++x)
        {
            GetPatch(x,y).x = x;
            GetPatch(x,y).y = y;
        }
}

void Terrain::DestroyPatch(uint x, uint y)
{
    if (x >= patchWidth_ || y >= patchHeight_)
        return;

    if (!GetFramework() || world_.Expired()) // Already destroyed or not initialized at all.
        return;
    
    Terrain::Patch &patch = GetPatch(x, y);

    if (patch.node)
    {
        patch.node->Remove();
        patch.urhoModel.Reset();
        patch.node = 0;
    }
}

void Terrain::Destroy()
{
    for(uint y = 0; y < patchHeight_; ++y)
        for(uint x = 0; x < patchWidth_; ++x)
            DestroyPatch(x, y);

    if (!GetFramework() || world_.Expired()) // Already destroyed or not initialized at all.
        return;

    if (rootNode_)
    {
        rootNode_->Remove();
        rootNode_ = 0;
    }
}


void Terrain::MakePatchFlat(uint x, uint y, float heightValue)
{
    Patch &patch = GetPatch(x, y);
    patch.heightData.Clear();
    patch.heightData.Reserve(cPatchSize * cPatchSize);
    for (uint i=0 ; i<cPatchSize * cPatchSize ; ++i)
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

    bool sizeChanged = xPatches.ValueChanged() || yPatches.ValueChanged();
    bool needFullRecreate = uScale.ValueChanged() || vScale.ValueChanged();
    bool needIncrementalRecreate = needFullRecreate || sizeChanged;

    // If the height map source has changed, we are going to request the new terrain asset,
    // which means that any changes to the current terrain attributes (size, scale, etc.) can
    // be ignored - we will be doing a full reload of the terrain from the asset when it completes loading.
    if (heightMap.ValueChanged())
        sizeChanged = needFullRecreate = needIncrementalRecreate = false;
   
    if (nodeTransformation.ValueChanged())
        UpdateRootNodeTransform();
    
    if (needFullRecreate)
        DirtyAllTerrainPatches();
    if (sizeChanged)
        ResizeTerrain(xPatches.Get(), yPatches.Get());
    if (needIncrementalRecreate)
        RegenerateDirtyTerrainPatches();
    
    
    if (heightMap.ValueChanged())
    {
        if (heightMap.Get().ref.Trimmed().Empty())
            LogDebug("Warning: Terrain \"" + this->parentEntity->Name() + "\" heightmap ref was set to an empty reference!");
        heightMapAsset_->HandleAssetRefChange(&heightMap);
    }
    if (material.ValueChanged())
    {
        materialAsset_->HandleAssetRefChange(&material);
        materialAsset_->Loaded.Connect(this, &Terrain::OnMaterialAssetLoaded);
    }
}

void Terrain::OnMaterialAssetLoaded(AssetPtr asset_)
{
    IMaterialAsset* mAsset = dynamic_cast<IMaterialAsset*>(asset_.Get());
    if (!mAsset)
    {
        LogErrorF("Terrain: Material asset load finished for '%s', but downloaded asset was not of type IMaterialAsset!", asset_->Name().CString());
        return;
    }

    for (uint i = 0; i < patches_.Size(); ++i)
    {
        if (patches_[i].node)
        {
            Urho3D::StaticModel* sm = patches_[i].node->GetComponent<Urho3D::StaticModel>();
            sm->SetMaterial(mAsset->UrhoMaterial());
        }
    }
}

void Terrain::OnMaterialAssetFailed(IAssetTransfer* /*transfer*/, String /*reason*/)
{
    if (!GetFramework()->HasCommandLineParameter("--useErrorAsset"))
        return;

    Urho3D::Material* mat = GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::Material>("Materials/AssetLoadError.xml");
    for (uint i = 0; i < patches_.Size(); ++i)
    {
        if (patches_[i].node)
        {
            Urho3D::StaticModel* sm = patches_[i].node->GetComponent<Urho3D::StaticModel>();
            sm->SetMaterial(mat);
        }
    }
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

    if (textureData)
    {
        if (textureData->DiskSource().Empty())
        {
            LogError("Tried to load terrain from texture file \"" + textureData->Name() + "\", but this requires a disk source of the asset for the load to work properly!");
            return;
        }
        HashMap<String, String> args = ParseAssetRefArgs(heightMap.Get().ref, 0);
        float scale = 30.f;
        float offset = 0.f;
        if (args.Find("scale") != args.End())
            scale = ToFloat(args["scale"]);
        if (args.Find("offset") != args.End())
            offset = ToFloat(args["offset"]);
        ///\todo Add support for converting the loaded image file directly to a .ntf asset and refer to that one.
        bool success = LoadFromImageFile(textureData->DiskSource(), offset, scale);
        if (!success)
        {
            LogError("Failed to load terrain from texture source \"" + textureData->Name() + "\"! Loading the file \"" + textureData->DiskSource() + "\" failed!");
        }
    }
}

void Terrain::Recreate()
{
    Destroy();
    DirtyAllTerrainPatches();
    RegenerateDirtyTerrainPatches();
}

void Terrain::GetTerrainHeightRange(float &minHeight, float &maxHeight) const
{
    minHeight = GetTerrainMinHeight();
    maxHeight = GetTerrainMaxHeight();
}

float Terrain::GetTerrainMinHeight() const
{
    float minHeight = std::numeric_limits<float>::max();

    for(uint i = 0; i < patches_.Size(); ++i)
        for(uint j = 0; j < patches_[i].heightData.Size(); ++j)
            minHeight = Min(minHeight, patches_[i].heightData[j]);

    return minHeight;
}

float Terrain::GetTerrainMaxHeight() const
{
    float maxHeight = std::numeric_limits<float>::min();

    for(uint i = 0; i < patches_.Size(); ++i)
        for(uint j = 0; j < patches_[i].heightData.Size(); ++j)
            maxHeight = Max(maxHeight, patches_[i].heightData[j]);

    return maxHeight;
}

void Terrain::Resize(uint newWidth, uint newHeight, uint oldPatchStartX, uint oldPatchStartY)
{
    Vector<Patch> newPatches(newWidth * newHeight);
    for(uint y = 0; y < newHeight && y + oldPatchStartY < yPatches.Get(); ++y)
        for(uint x = 0; x < newWidth && x + oldPatchStartX < xPatches.Get(); ++x)
            newPatches[y * newWidth + x] = patches_[(y + oldPatchStartY) * xPatches.Get() + x + oldPatchStartX];

    patches_ = newPatches;
    xPatches.Set(newWidth, AttributeChange::Disconnected);
    yPatches.Set(newHeight, AttributeChange::Disconnected);
    patchWidth_ = newWidth;
    patchHeight_ = newHeight;
    DirtyAllTerrainPatches();
    RegenerateDirtyTerrainPatches();
}

float Terrain::GetPoint(uint x, uint y) const
{
    if (x >= cPatchSize * patchWidth_)
        x = cPatchSize * patchWidth_ - 1;
    if (y >= cPatchSize * patchHeight_)
        y = cPatchSize * patchHeight_ - 1;

    return GetPatch(x / cPatchSize, y / cPatchSize).heightData[(y % cPatchSize) * cPatchSize + (x % cPatchSize)];
}

void Terrain::SetPointHeight(uint x, uint y, float height)
{
    if (x >= cPatchSize * patchWidth_ || y >= cPatchSize * patchHeight_)
        return; // Out of bounds signals are silently ignored.

    GetPatch(x / cPatchSize, y / cPatchSize).heightData[(y % cPatchSize) * cPatchSize + (x % cPatchSize)] = height;
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
    return float3(x_slope, 2.0f, y_slope).Normalized();
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

bool Terrain::LoadFromImageFile(String filename, float offset, float scale)
{
    SharedPtr<Urho3D::Image> image = SharedPtr<Urho3D::Image>(new Urho3D::Image(GetContext()));
    Vector<u8> data;
    if (!LoadFileToVector(filename, data))
        return false;

    Urho3D::MemoryBuffer imageBuffer(&data[0], data.Size());
    if (!image->Load(imageBuffer))
        return false;

    // Note: In the following, we round down, so if the image size is not a multiple of cPatchSize (== 16),
    // we will not use the whole image contents.
    xPatches.Set((uint)image->GetWidth() / cPatchSize, AttributeChange::Disconnected);
    yPatches.Set((uint)image->GetHeight() / cPatchSize, AttributeChange::Disconnected);
    ResizeTerrain(xPatches.Get(), yPatches.Get());

    for(uint y = 0; y < yPatches.Get() * cPatchSize; ++y)
        for(uint x = 0; x < xPatches.Get() * cPatchSize; ++x)
        {
            Urho3D::Color c = image->GetPixel(x, y);
            float height = offset + scale * ((c.r_ + c.g_ + c.b_) / 3.f); // Treat the image as a grayscale heightmap field with the color in range [0,1].
            SetPointHeight(x, y, height);
        }

    xPatches.Changed(AttributeChange::LocalOnly);
    yPatches.Changed(AttributeChange::LocalOnly);

    DirtyAllTerrainPatches();
    RegenerateDirtyTerrainPatches();

    return true;
}

bool Terrain::LoadFromDataInMemory(const char *data, size_t numBytes)
{
    int offset = 0;
    u32 xPatches = ReadU32(data, numBytes, offset);
    u32 yPatches = ReadU32(data, numBytes, offset);

    // Load all the data from the file to an intermediate buffer first, so that we can first see
    // if the file is not broken, and reject it without losing the old terrain.
    Vector<Patch> newPatches(xPatches * yPatches);

    // Initialize the new height data structure.
    for(u32 y = 0; y < yPatches; ++y)
        for(u32 x = 0; x < xPatches; ++x)
        {
            newPatches[y * xPatches + x].x = x;
            newPatches[y * xPatches + x].y = y;
        }

    assert(sizeof(float) == 4);

    // Load the new data.
    for(uint i = 0; i < newPatches.Size(); ++i)
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
    Destroy();
    CreateRootNode();

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

void Terrain::DirtyAllTerrainPatches()
{
    for(uint i = 0; i < patches_.Size(); ++i)
        patches_[i].patch_geometry_dirty = true;
}

void Terrain::RegenerateDirtyTerrainPatches()
{
    URHO3D_PROFILE(Terrain_RegenerateDirtyTerrainPatches);

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

    if (!rootNode_)
    {
        // CreateRootNode calls this function once the root node has been created.
        CreateRootNode();
        return;
    }

    Urho3D::Scene* urhoScene = world_.Lock()->UrhoScene();

    // If this entity has a Placeable, make sure it is the parent of this terrain component.
    SharedPtr<Placeable> pos = (ParentEntity() ? ParentEntity()->Component<Placeable>() : SharedPtr<Placeable>());
    if (pos != nullptr)
    {
        pos->UrhoSceneNode()->AddChild(rootNode_);
        rootNode_->SetEnabled(pos->visible.Get()); // Re-apply visibility on all the geometry.
    }
    else
    {
        // No Placeable: the root node is attached to the scene directly.
        urhoScene->AddChild(rootNode_);
    }
}

Urho3D::Node* Terrain::CreateUrho3DTerrainPatchNode(Urho3D::Node* parent, uint patchX, uint patchY) const
{
    Urho3D::Node* node = 0;
    if (world_.Expired())
        return node;

    String name = String("Terrain_Patch_") + String(patchX) + "_" + String(patchY);
    node = parent->CreateChild(name);
     
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
    URHO3D_PROFILE(Terrain_GenerateTerrainGeometryForOnePatch);

    Terrain::Patch &patch = GetPatch(patchX, patchY);

    if (!ViewEnabled())
        return;
    if (world_.Expired())
        return;


    if (patch.node)
    {
        patch.node->Remove();
        patch.node = 0;
        patch.urhoModel.Reset();
    }

    patch.node = CreateUrho3DTerrainPatchNode(rootNode_, patch.x, patch.y);
    assert(patch.node);

    Urho3D::StaticModel* staticModel = patch.node->CreateComponent<Urho3D::StaticModel>();
    staticModel->SetCastShadows(false);
    SharedPtr<Urho3D::Model> manual = SharedPtr<Urho3D::Model>(new Urho3D::Model(GetContext()));
    patch.urhoModel = manual;
    manual->SetNumGeometries(1);

    Vector<unsigned short> indexData;
    Vector<float> vertexData;

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

    const float cFloatMax = std::numeric_limits<float>::max();
    const float cFloatMin = std::numeric_limits<float>::min();
    float3 boundsMin = float3(cFloatMax, cFloatMax, cFloatMax);
    float3 boundsMax = float3(cFloatMin, cFloatMin, cFloatMin);
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

    staticModel->SetModel(manual);

    // Make the entity & component links for identifying raycasts
    patch.node->SetVar(GraphicsWorld::entityLink, Variant(WeakPtr<RefCounted>(ParentEntity())));
    patch.node->SetVar(GraphicsWorld::componentLink, Variant(WeakPtr<RefCounted>(this)));

    patch.patch_geometry_dirty = false;

    // Set material if available
    IMaterialAsset* mAsset = dynamic_cast<IMaterialAsset*>(materialAsset_->Asset().Get());
    if (mAsset)
        staticModel->SetMaterial(mAsset->UrhoMaterial());

    AttachTerrainRootNode();
}

}
