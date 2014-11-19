// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IComponent.h"
#include "IAttribute.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"
#include "AssetReference.h"
#include "AssetRefListener.h"
#include "CoreTypes.h"
#include "Math/Transform.h"

#include <Math/float3.h>
#include <Model.h>

namespace Tundra
{

typedef WeakPtr<Placeable> PlaceableWeakPtr;


/// Adds a heightmap-based terrain to the scene.
/** <table class="header">

    <tr>
    <td>
    <h2>Terrain</h2>
    Adds a heightmap-based terrain to the scene. A Terrain is composed of a rectangular grid of adjacent "patches".
    Each patch is a fixed-size 16x16 height map.

    Registered by UrhoRenderer plugin.

    <b>Attributes:</b>
    <ul>
    <li>Transform: nodeTransformation
    <div> @copydoc nodeTransformation </div>
    <li>int: xPatches
    <div> @copydoc xPatches</div>
    <li>int: yPatches
    <div> @copydoc yPatches</div>
    <li>float: uScale
    <div> @copydoc uScale </div>
    <li>float : vScale
    <div> @copydoc vScale </div>
    <li>AssetReference: material
    <div> @copydoc material </div>
    <li>AssetReference: heightMap
    <div> @copydoc heightMap </div>
    </ul>

    Note that the way the textures are used depends completely on the material. For example, the default height-based terrain material "Rex/TerrainPCF"
    only uses the texture channels 0-3, and blends between those based on the terrain height values.

    Emits TerrainRegenerated-signal once terrain has been succesfully generated.
    
    <b>Does not depend on any other components</b>. Currently Terrain stores its own transform matrix, so it does not depend on the Placeable component. It might be more consistent
    to create a dependency to Placeable, so that the position of the terrain is editable in the same way the position of other placeables is done.
    </table> */
class URHO_MODULE_API Terrain : public IComponent
{
    COMPONENT_NAME(Terrain, 11)

public:
    /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit Terrain(Urho3D::Context* context, Scene* scene);
    /// @endcond
    virtual ~Terrain();

    /// Specifies the transformation matrix of the terrain
    Attribute<Transform> nodeTransformation;

    /// The number of patches to generate in the terrain in the horizontal direction, in the range [0, 256].
    Attribute<uint> xPatches;

    /// The number of patches to generate in the terrain in the vertical direction, in the range [0, 256].
    Attribute<uint> yPatches;

    /// Texture U coordinate scaling factor.
    Attribute<float> uScale;

    /// Texture V coordinate scaling factor.
    Attribute<float> vScale;

    /// Specifies the material to use when rendering the terrain.
    Attribute<AssetReference> material;
    
    /// Specifies the height map used to generate the terrain.
    Attribute<AssetReference> heightMap;

   /// Returns the minimum and maximum extents of terrain heights.
    void GetTerrainHeightRange(float &minHeight, float &maxHeight) const;

    /// Each patch is a square containing this many vertices per side.
    static const uint cPatchSize = 16;

    /// Describes a single patch that is present in the scene.
    /** A patch can be in one of the following three states:
        - not loaded. The height data nor the GPU data is present, but the Patch struct itself is initialized. heightData.size() == 0, node == entity == 0. meshGeometryName == "".
        - heightmap data loaded. The heightData vector contains the heightmap data, but the visible GPU vertex data itself has not been generated yet, due to the neighbors
          of this patch not being present yet. node == entity == 0, meshGeometryName == "". patch_geometry_dirty == true.
        - fully loaded. The GPU data is also loaded and the node, entity and meshGeometryName fields specify the used GPU resources. */
    struct Patch
    {
        Patch():x(0), y(0), node(0), patch_geometry_dirty(true) {}

        /// X-coordinate on the grid of patches. In the range [0, Terrain::PatchWidth()].
        uint x;

        /// Y-coordinate on the grid of patches. In the range [0, Terrain::PatchHeight()].
        uint y;

        /// Typically this will be a 16x16 array of height values in world coordinates.
        /// If the length is zero, this patch hasn't been loaded in yet.
        Urho3D::Vector<float> heightData;

        /// Urho3D -specific: Store a reference to the actual render hierarchy node.
        Urho3D::Node *node;

        /// Urho3D -specific: Store a reference to the mesh that is attached to the above SceneNode.
        SharedPtr<Urho3D::Model> urhoModel;

        /// If true, the CPU-side heightmap data has changed, but we haven't yet updated
        /// the GPU-side geometry resources since the neighboring patches haven't been loaded
        /// in yet.
        bool patch_geometry_dirty;

        /// Call only when you've checked that this patch has been loaded in.
        float GetHeightValue(uint x, uint y) const { return heightData[y * cPatchSize + x]; }
    };
    
    /// @return The patch at given (x,y) coordinates. Pass in values in range [0, PatchWidth()/PatchHeight[.
    Patch &GetPatch(uint patchX, uint patchY)
    {
        assert(patchX >= 0);
        assert(patchY >= 0);
        assert(patchX < patchWidth_);
        assert(patchY < patchHeight_);
        return patches_[patchY * patchWidth_ + patchX];
    }

    /// @return The patch at given (x,y) coordinates. Pass in values in range [0, PatchWidth()/PatchHeight[. Read only.
    const Patch &GetPatch(uint patchX, uint patchY) const
    {
        assert(patchX >= 0);
        assert(patchY >= 0);
        assert(patchX < patchWidth_);
        assert(patchY < patchHeight_);
        return patches_[patchY * patchWidth_ + patchX];
    }

    /// Returns true if the given patch exists, i.e. whether the given coordinates are within the current terrain patch dimensions.
    /** This function does not tell whether the data for the patch is actually loaded on the CPU or the GPU. */
    bool PatchExists(uint patchX, uint patchY) const
    {
        return patchX < patchWidth_ && patchY < patchHeight_ && patchY * patchWidth_ + patchX < (int)patches_.Size();
    }

    /// Returns true if all the patches on the terrain are loaded on the CPU, i.e. if all the terrain height data has been streamed in from the server side.
    bool AllPatchesLoaded() const
    {
        for(uint y = 0; y < patchHeight_; ++y)
            for(uint x = 0; x < patchWidth_; ++x)
                if (!PatchExists(x,y) || GetPatch(x,y).heightData.Size() == 0 || GetPatch(x,y).node == 0)
                    return false;

        return true;
    }

    /// Returns how many patches there are in the terrain in the x-direction.
    /// This differs from the xPatches attribute in that the PatchWidth tells how many patches there
    /// are *currently initialized*, whereas xPatches tells the number of patches wide the terrain is
    /// going to be, as soon as we get a chance to update the new geometry data.
    uint PatchWidth() const { return patchWidth_; }

    /// Returns how many patches there are in the terrain in the y-direction.
    /// This value is understood in the similar way as above.
    uint PatchHeight() const { return patchHeight_; }

    /// Returns the number of vertices in the whole terrain in the local X-direction.
    uint VerticesWidth() const { return PatchWidth() * cPatchSize; }

    /// Returns the number of vertices in the whole terrain in the local Y-direction.
    uint VerticesHeight() const { return PatchHeight() * cPatchSize; }

    /// Removes all stored terrain patches and the associated Urho3D scene nodes.
    void Destroy();

    /// Returns the height value on the given terrain grid point.
    /// @param x In the range [0, Terrain::PatchWidth * Terrain::cPatchSize [.
    /// @param y In the range [0, Terrain::PatchHeight * Terrain::cPatchSize [.
    float GetPoint(uint x, uint y) const;

    /// Sets a new height value to the given terrain map vertex. Marks the patch that vertex is part of dirty,
    /// but does not immediately recreate the GPU surfaces. Use the RegenerateDirtyTerrainPatches() function
    /// to regenerate the visible Ogre mesh geometry.
    void SetPointHeight(uint x, uint y, float height);

    /// Loads the terrain from the given image file.
    /** Adjusts the xPatches and yPatches properties to that of the image file, 
        and clears the heightMap source attribute. This function is intended to be used as a processing tool. Calling this  
        function will get the terrain contents desynchronized between the local system and network. The file is loaded using Urho3D, so
        this supports all the file formats Urho3D has codecs loaded for(you can see a list of those in the console at startup).
        Calling this function will regenerate all terrain patches on the GPU. */
    bool LoadFromImageFile(String filename, float offset, float scale);

    /// Loads the terrain height map data from the given in-memory .ntf file buffer.
    bool LoadFromDataInMemory(const char *data, size_t numBytes);

    /// Marks all terrain patches dirty.
    void DirtyAllTerrainPatches();

    /// Recreate terrain patches that are marked dirty.
    void RegenerateDirtyTerrainPatches();

    /// Returns the minimum height value in the whole terrain.
    /** This function blindly iterates through the whole terrain, so avoid calling it in performance-critical code. */
    float GetTerrainMinHeight() const;

    /// Returns the maximum height value in the whole terrain.
    /** This function blindly iterates through the whole terrain, so avoid calling it in performance-critical code. */
    float GetTerrainMaxHeight() const;

    /// Resizes the terrain and recreates it.
    /// newWidth and newHeight are the size of the new terrain, in # patches.
    /// oldPatchStartX&Y specify the patch offset to copy the old terrain height values from.
    void Resize(uint newWidth, uint newHeight, uint oldPatchStartX = 0, uint oldPatchStartY = 0);

    /// Makes all the vertices of the given patch flat with the given height value.
    /** Dirties the patch, but does not regenerate it. */
    void MakePatchFlat(uint patchX, uint patchY, float heightValue);

     /// Emitted when the terrain data is regenerated.
    Signal0<void> TerrainRegenerated;

private:
    /// Called when the parent entity has been set.
    void UpdateSignals();

    /// Called when component has been added or removed from the parent entity. Checks the existence of the Placeable component, and attaches this light to it.
    void OnComponentStructureChanged(IComponent*, AttributeChange::Type);

    void AttributesChanged() override;

    /// Recreate whole terrain
    void Recreate();

    void OnMaterialAssetLoaded(AssetPtr asset);
    void OnTerrainAssetLoaded(AssetPtr asset);

    /// (Re)checks whether this entity has Placeable (or if it was just added or removed), and reparents the rootNode of this component to it or the scene root.
    /** Additionally re-applies the visibility of each terrain patch that is currently attached to the terrain node. */
    void AttachTerrainRootNode();

    /// Creates the patch parent/root node if it does not exist.
    /** After this function returns, the 'root' member node will exist. */
    void CreateRootNode();

    /// Create a scene node for a terrain patch under the specified parent and at the specified x/y position
    /** Sets local position of the node based on the patchX and patchY params */
    Urho3D::Node* CreateUrho3DTerrainPatchNode(Urho3D::Node* parent, uint patchX, uint patchY) const;

    /// Calculates and returns the vertex normal for the given terrain vertex.
    /// @param patchX The patch to read from, [0, PatchWidth()[.
    /// @param patchY The patch to read from, [0, PatchHeight()[.
    /// @param xinside The vertex inside the patch to compute the normal for, [0, cPatchSize[.
    /// @param yinside The vertex inside the patch to compute the normal for, [0, cPatchSize[.
    float3 CalculateNormal(uint x, uint y, uint xinside, uint yinside) const;

    /// Sets the given patch to use the currently set material and textures.
    void UpdateTerrainPatchMaterial(uint patchX, uint patchY);

    /// Updates the root node transform from the current attribute values, if the root node exists.
    void UpdateRootNodeTransform();

    /// Readjusts the terrain to contain the given number of patches in the horizontal and vertical directions.
    /** Preserves as much of the terrain height data as possible. Dirties the patches, but does not regenerate them.
        @note This function does not adjust the xPatches or yPatches attributes. */
    void ResizeTerrain(uint newPatchWidth, uint newPatchHeight);

    /// Releases all resources used for the given patch.
    void DestroyPatch(uint x, uint y);

    /// Updates the terrain material with the new texture on the given texture unit index.
    /// @param index The texture unit index to set the new texture to.
    /// @param textureName The Ogre texture resource name to set.
    void SetTerrainMaterialTexture(uint index, const String &textureName);

    /// Creates Ogre geometry data for the single given patch, or updates the geometry for an existing
    /// patch if the associated Ogre resources already exist.
    void GenerateTerrainGeometryForOnePatch(uint patchX, uint patchY);

    SharedPtr<AssetRefListener> materialAsset_;
    SharedPtr<AssetRefListener> heightMapAsset_;

    /// For all terrain patches, we maintain a global parent/root node to be able to transform the whole terrain at one go.
    SharedPtr<Urho3D::Node> rootNode_;

    uint patchWidth_;
    uint patchHeight_;

    /// Specifies the asset source from which the height map is currently loaded from. Used to shadow the heightMap attribute so that if
    /// the same value is received from the network, reloading the terrain can be avoided.
    String currentHeightmapAssetSource_;

    /// Specifies the Ogre material name of the material that is currently being used to display the terrain.
    String currentMaterial_;

    /// Stores the actual height patches.
    Vector<Patch> patches_;
    
     /// Graphics world ptr
    GraphicsWorldWeakPtr world_;
};

}
