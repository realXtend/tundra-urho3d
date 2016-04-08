// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "AssetRefListener.h"
#include "IAttribute.h"
#include "AssetReference.h"
#include "IComponent.h"
#include "Framework.h"
#include "AssetAPI.h"
#include "FrameAPI.h"
#include "IAsset.h"
#include "IAssetTransfer.h"
#include "LoggingFunctions.h"

namespace Tundra
{

// AssetRefListener

AssetRefListener::AssetRefListener() : 
    myAssetAPI(0), 
    currentWaitingRef("")
{
}

AssetPtr AssetRefListener::Asset() const
{
    return asset.Lock();
}

void AssetRefListener::HandleAssetRefChange(IAttribute *assetRef, const String& assetType)
{
    Attribute<AssetReference> *attr = dynamic_cast<Attribute<AssetReference> *>(assetRef);
    if (!attr)
    {
        LogWarning("AssetRefListener::HandleAssetRefChange: Attribute's type not AssetReference (was " +
            (assetRef == 0 ? "null" : assetRef->TypeName()) + " instead).");
        return;
    }
    HandleAssetRefChange(attr->Owner()->GetFramework()->Asset(), attr->Get().ref, assetType);
}

void AssetRefListener::HandleAssetRefChange(AssetAPI *assetApi, String assetRef, const String& assetType)
{
    // Disconnect from any previous transfer we might be listening to
    if (!currentTransfer.Expired())
    {
        IAssetTransfer* current = currentTransfer.Get();
        current->Succeeded.Disconnect(this, &AssetRefListener::OnTransferSucceeded);
        current->Failed.Disconnect(this, &AssetRefListener::OnTransferFailed);
        currentTransfer.Reset();
    }
    
    assert(assetApi);

    // Store AssetAPI ptr for later signal hooking.
    if (!myAssetAPI)
        myAssetAPI = assetApi;

    // If the ref is empty, don't go any further as it will just trigger the LogWarning below.
    assetRef = assetRef.Trimmed();
    if (assetRef.Empty())
    {
        asset = AssetPtr();
        return;
    }
    currentWaitingRef = "";

    // Resolve the protocol for generated:// assets. These assets are never meant to be
    // requested from AssetAPI, they cannot be fetched from anywhere. They can only be either
    // loaded or we must wait for something to load/create them.
    String protocolPart = "";
    assetApi->ParseAssetRef(assetRef, &protocolPart);
    if (protocolPart.ToLower() == "generated")
    {
        AssetPtr loadedAsset = assetApi->FindAsset(assetRef);
        if (loadedAsset.Get() && loadedAsset->IsLoaded())
        {
            // Asset is loaded, emit Loaded with 1 msec delay to preserve the logic
            // that HandleAssetRefChange won't emit anything itself as before.
            // Otherwise existing connection can break/be too late after calling this function.
            asset = loadedAsset;
            assetApi->GetFramework()->Frame()->DelayedExecute(0.0f).Connect(this, &AssetRefListener::EmitLoaded);
            return;
        }
        else
        {
            // Wait for it to be created.
            currentWaitingRef = assetRef;
            myAssetAPI->AssetCreated.Connect(this, &AssetRefListener::OnAssetCreated);
        }
    }
    else
    {
        // This is not a generated asset, request normally from asset api.
        AssetTransferPtr transfer = assetApi->RequestAsset(assetRef, assetType);
        if (!transfer)
        {
            LogWarning("AssetRefListener::HandleAssetRefChange: Asset request for asset \"" + assetRef + "\" failed.");
            return;
        }
        currentWaitingRef = assetRef;

        transfer->Succeeded.Connect(this, &AssetRefListener::OnTransferSucceeded);
        transfer->Failed.Connect(this, &AssetRefListener::OnTransferFailed);

        currentTransfer = transfer;
    }
    
    // Disconnect from the old asset's load signal
    if (asset)
        asset->Loaded.Disconnect(this, &AssetRefListener::OnAssetLoaded);
    asset = AssetPtr();
}

void AssetRefListener::OnTransferSucceeded(AssetPtr assetData)
{
    if (!assetData)
        return;
    
    // Connect to further reloads of the asset to be able to notify of them.
    asset = assetData;
    assetData->Loaded.Connect(this, &AssetRefListener::OnAssetLoaded);
    Loaded.Emit(assetData);
}

void AssetRefListener::OnAssetLoaded(AssetPtr assetData)
{
    if (assetData == asset.Lock())
        Loaded.Emit(assetData);
}

void AssetRefListener::OnTransferFailed(IAssetTransfer* transfer, String reason)
{
    /// @todo Remove this logic once a EC_Material + EC_Mesh behaves correctly without failed requests, see generated:// logic in HandleAssetRefChange.
    if (myAssetAPI)
        myAssetAPI->AssetCreated.Connect(this, &AssetRefListener::OnAssetCreated);
    TransferFailed.Emit(transfer, reason);
}

void AssetRefListener::OnAssetCreated(AssetPtr assetData)
{
    if (assetData.Get() && !currentWaitingRef.Empty() && currentWaitingRef == assetData->Name())
    {
        /// @todo Remove this logic once a EC_Material + EC_Mesh behaves correctly without failed requests, see generated:// logic in HandleAssetRefChange.
        /** Log the same message as before for non generated:// refs. This is good to do
            because AssetAPI has now said the request failed, so user might be confused when it still works. */
        if (!currentWaitingRef.ToLower().StartsWith("generated://"))
            LogInfo("AssetRefListener: Asset \"" + assetData->Name() + "\" was created, applying after it loads.");

        // The asset we are waiting for has been created, hook to the IAsset::Loaded signal.
        currentWaitingRef = "";
        asset = assetData;
        assetData->Loaded.Connect(this, &AssetRefListener::OnAssetLoaded);
        if (myAssetAPI)
            myAssetAPI->AssetCreated.Disconnect(this, &AssetRefListener::OnAssetCreated);
    }
}

void AssetRefListener::EmitLoaded(float /*time*/)
{
    AssetPtr currentAsset = asset.Lock();
    if (currentAsset.Get())
        Loaded.Emit(currentAsset);
}

// AssetRefListListener

AssetRefListListener::AssetRefListListener(AssetAPI *assetAPI) :
    assetAPI_(assetAPI)
{
    if (!assetAPI_)
        LogError("AssetRefListListener: Null AssetAPI* given to ctor!");
}

void AssetRefListListener::HandleChange(const AssetReferenceList &refs)
{
    if (!assetAPI_)
        return;

    // Does a size check and compares each element with case-sensitive compare.
    if (refs == current_)
        return;

    AssetReferenceList prev = current_;

    // Set current internal refs
    current_ = refs;
    uint numRefs = current_.Size();

    // Trim and resolve internal refs
    for (uint i=0; i<numRefs; ++i)
    {
        AssetReference &ref = current_[i];
        ref.ref = ref.ref.Trimmed();
        if (!ref.ref.Empty())
            ref.ref = assetAPI_->ResolveAssetRef("", ref.ref);
    }

    // Changed
    Changed.Emit(current_);

    // Cleared
    for (uint i=0; i<numRefs; ++i)
    {
        const AssetReference &ref = current_[i];
        const AssetReference &prevRef = (i < prev.Size() ? prev[i] : AssetReference());
        if (ref.ref.Empty() && !prevRef.ref.Empty())
            Cleared.Emit(i);
    }
    for(uint i = numRefs; i < prev.Size(); ++i)
    {
        if (!prev[i].ref.Empty())
            Cleared.Emit(i);
    }

    // Request
    while(listeners_.Size() > numRefs)
        listeners_.Pop();
    while(listeners_.Size() < numRefs)
    {
        // Connect to signals once on creation. AssetRefListeners
        // are reusable for multiple refs/requests.
        AssetRefListenerPtr listener(new AssetRefListener());
        listener->TransferFailed.Connect(this, &AssetRefListListener::OnAssetFailed);
        listener->Loaded.Connect(this, &AssetRefListListener::OnAssetLoaded);
        listeners_.Push(listener);
    }
    for (uint i=0; i<numRefs; ++i)
    {
        const AssetReference &ref = current_[i];
        if (!ref.ref.Empty())
            listeners_[i]->HandleAssetRefChange(assetAPI_, ref.ref, ref.type);
    }
}

Vector<AssetPtr> AssetRefListListener::Assets() const
{
    Vector<AssetPtr> assets;
    for (uint i = 0; i < current_.Size(); ++i)
    {
        const AssetReference &ref = current_[i];
        if (!ref.ref.Empty())
            assets.Push(assetAPI_->FindAsset(ref.ref));
        else
            assets.Push(AssetPtr());
    }
    return assets;
}

AssetPtr AssetRefListListener::Asset(uint index) const
{
    if (index < current_.Size() && !current_[index].ref.Empty())
        return assetAPI_->FindAsset(current_[index].ref);
    return AssetPtr();
}

void AssetRefListListener::OnAssetFailed(IAssetTransfer *transfer, String reason)
{
    if (!transfer)
        return;

    bool signaled = false;
    String failedRef = transfer->SourceUrl();
    for(uint i = 0; i < current_.Size(); ++i)
    {
        const AssetReference &ref = current_[i];
        if (ref.ref.Compare(failedRef, false) == 0)
        {
            // Don't break/return. Same asset might be in multiple indexes!
            Failed.Emit(i, transfer, reason);
            signaled = true;
        }
    }
    if (!signaled)
        LogWarning("AssetRefListListener: Failed to signal failure to load " + failedRef + ". The asset ref is unknown to the local state.");
}

void AssetRefListListener::OnAssetLoaded(AssetPtr asset)
{
    if (!asset)
        return;

    bool signaled = false;
    String completedRef = asset->Name();
    for(uint i = 0; i < current_.Size(); ++i)
    {
        const AssetReference &ref = current_[i];
        if (ref.ref.Compare(completedRef, false) == 0)
        {
            // Don't break/return. Same asset might be in multiple indexes!
            Loaded.Emit(i, asset);
            signaled = true;
        }
    }
    if (!signaled)
        LogWarning("AssetRefListListener: Failed to signal completion of " + completedRef + ". The asset ref is unknown to the local state.");
}

}
