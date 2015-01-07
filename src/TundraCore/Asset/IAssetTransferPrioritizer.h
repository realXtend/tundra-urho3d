// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"
#include "AssetFwd.h"

#include <Urho3D/Container/RefCounted.h>

namespace Tundra
{

class TUNDRACORE_API IAssetTransferPrioritizer : public RefCounted
{
public:
    IAssetTransferPrioritizer() {}
    virtual ~IAssetTransferPrioritizer() {}

    /// Prioritizes @c transfers and return the result
    /** Called by AssetAPI. If the returned list does not match @c transfers size,
        the original will be used so that no transfers are lost. */
    virtual AssetTransferPtrVector Prioritize(const AssetTransferPtrVector &transfers) = 0;
};

}
