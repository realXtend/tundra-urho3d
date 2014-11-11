// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IAssetTransferPrioritizer.h"

namespace Tundra
{

class TUNDRACORE_API DefaultAssetTransferPrioritizer : public IAssetTransferPrioritizer
{
public:
    DefaultAssetTransferPrioritizer();
    
    /// IAssetTransferPrioritizer override
    AssetTransferPtrVector Prioritize(const AssetTransferPtrVector &transfers) override;
};

}
