// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "AssetFwd.h"
#include "AssetReference.h"
#include "Signals.h"

#include <RefCounted.h>

namespace Tundra
{

class IAttribute;

/// Tracks and notifies about asset change events.
class TUNDRACORE_API AssetRefListener : public RefCounted
{
public:
    AssetRefListener();

    /// Issues a new asset request to the given AssetReference.
    /// @param assetRef A pointer to an attribute of type AssetReference.
    /// @param assetType Optional asset type name
    void HandleAssetRefChange(IAttribute *assetRef, const String& assetType = "");

    /// Issues a new asset request to the given assetRef URL.
    /// @param assetApi Pass a pointer to the system Asset API into this function (This utility object doesn't keep reference to framework).
    /// @param assetType Optional asset type name
    void HandleAssetRefChange(AssetAPI *assetApi, String assetRef, const String& assetType = "");
    
    /// Returns the asset currently stored in this asset reference.
    AssetPtr Asset() const;

    /// Emitted when the raw byte download of this asset finishes.
    Signal1<IAssetTransfer*> Downloaded;

    /// Emitted when this asset is ready to be used in the system.
    Signal1<AssetPtr> Loaded;

    /// Emitted when the transfer failed
    Signal2<IAssetTransfer *, String> TransferFailed;

private:
    void OnTransferSucceeded(AssetPtr assetData);
    void OnAssetLoaded(AssetPtr assetData);
    void OnTransferFailed(IAssetTransfer *transfer, String reason);
    void OnAssetCreated(AssetPtr assetData);
    
    void EmitLoaded(float time);

private:
    AssetAPI *myAssetAPI;
    AssetWeakPtr asset;
    AssetTransferWeakPtr currentTransfer;
    String currentWaitingRef;
};

}

