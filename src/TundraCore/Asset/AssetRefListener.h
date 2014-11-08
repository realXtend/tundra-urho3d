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

/// Tracks and notifies about asset change events.
class TUNDRACORE_API AssetRefListListener : public RefCounted
{
public:
    AssetRefListListener(AssetAPI *assetAPI);

    /// Handles change to refs.
    /** Checks if there are actual changes against last change.
        Requests Assets and emits signals. */
    void HandleChange(const AssetReferenceList &refs);

    /// Returns current known states assets.
    /** Returned vector will match in size with known state.
        If index is still loading a null AssePtr is assigned to the index. */
    Vector<AssetPtr> Assets() const;

    /// Returns Asset for index.
    AssetPtr Asset(int index) const;

    /// Emitted when list changes.
    Signal1<const AssetReferenceList&> Changed;

    /// Emitted when a previously non-empty index was cleared to a empty string.
    /** @note Fired after Changed. */
    Signal1<int> Cleared;

    /// Corresponding AssetRefListener signals with additional asset index.
    //Signal2<int, IAssetTransfer*> Downloaded; /// @todo Implement when there is need for this
    Signal2<int, AssetPtr> Loaded;
    Signal3<int, IAssetTransfer*, String> Failed;

private:
    void OnAssetFailed(IAssetTransfer *transfer, String reason);
    void OnAssetLoaded(AssetPtr asset);

    AssetAPI *assetAPI_;
    AssetReferenceList current_;
    Vector<AssetRefListenerPtr > listeners_;
};

}
