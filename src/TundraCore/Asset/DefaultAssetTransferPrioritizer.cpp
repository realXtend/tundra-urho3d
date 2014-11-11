// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "DefaultAssetTransferPrioritizer.h"
#include "IAssetTransfer.h"

namespace Tundra
{

struct TypeIndex
{
    uint index;
    uint num;

    TypeIndex() : index(0), num(0) {}

    uint Current()
    {
        return index + num;
    }
};

DefaultAssetTransferPrioritizer::DefaultAssetTransferPrioritizer()
{
}

AssetTransferPtrVector DefaultAssetTransferPrioritizer::Prioritize(const AssetTransferPtrVector &transfers)
{
    /** @todo Add prioritizing with distance to active camera when EntityWeakPtr info is available in transfers.
        @todo Add more types? Should scripts go last or first?
        @todo Possibly add option for this function to communicate pending tranfers for a longer time if they are eg. 1km away from camera. */
    TypeIndex iMesh;
    TypeIndex iMaterial;

    AssetTransferPtrVector sorted;
    for(auto iter = transfers.Begin(); iter != transfers.End(); ++iter)
    {
        const AssetTransferPtr &transfer = (*iter);
        if (transfer->assetType.Contains("mesh", false))
        {
            sorted.Insert(iMesh.Current(), transfer);
            iMesh.num++;
        }
        else if (transfer->assetType.Contains("material", false))
        {
            sorted.Insert(iMesh.Current() + iMaterial.Current(), transfer);
            iMaterial.num++;
        }
        else
            sorted.Push(transfer);
    }
    return sorted;
}

}
