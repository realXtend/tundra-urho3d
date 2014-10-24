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
        IAssetTransfer* current = currentTransfer.Lock().Get();
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
        return;
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
    AssetPtr assetData = asset.Lock();
    if (assetData)
        assetData->Loaded.Disconnect(this, &AssetRefListener::OnAssetLoaded);
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

}
