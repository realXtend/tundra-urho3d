// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "AssetFwd.h"
#include "IAssetUploadTransfer.h"
#include "CoreTypes.h"
#include "CoreDefines.h"

#include <Urho3D/Core/Object.h>

namespace Tundra
{

/// A common interface for all classes that implement downloading and uploading assets via different protocols.
/** Asset providers receive asset download requests through the RequestAsset() function. It should
    return true if the asset id was of such format that the request can be handled, false if it could not be handled.

    Additionally, the asset provider can post events of the progress of an asset download. */
class TUNDRACORE_API IAssetProvider : public Object
{
public:
    IAssetProvider(Urho3D::Context* context) :
        Object(context)
    {}

    /// @cond PRIVATE
    friend class AssetAPI;
    /// @endcond

    virtual ~IAssetProvider() {}

    /// Returns name of asset provider for identification purposes
    virtual String Name() const = 0;

    /// Queries this asset provider whether the assetRef is a valid assetRef this provider can handle.
    /** @param assetType The type of the asset. This field is optional, and the ref itself can specify the type,
               or if the provider in question does not need the type information, this can be left blank. */
    virtual bool IsValidRef(String assetRef, String assetType) const = 0;

    /// Create asset transfer for @c assetRef and @c assetType.
    /** @return Initiated asset transfer. Note that this can be null. */
    virtual AssetTransferPtr CreateTransfer(String assetRef, String assetType) = 0;

    /// Execute transfer that this provider has created and prepared in CreateTransfer.
    virtual void ExecuteTransfer(AssetTransferPtr transfer) = 0;

    /// Aborts the ongoing transfer, returns true if successful and false otherwise.
    /** Override this function in a provider implementation if it supports aborting. */
    virtual bool AbortTransfer(IAssetTransfer * UNUSED_PARAM(transfer)) { return false; }

    /// Performs time-based update of asset provider, to for example handle timeouts.
    /** The system will call this periodically for all registered asset providers, so
        it does not need to be called manually.
        @param frametime Seconds since last frame */
    virtual void Update(float UNUSED_PARAM(frametime)) {}

    /// Issues an asset deletion request to the asset storage and provider this asset resides in.
    /** If the asset provider supports this feature, it will delete the asset from the source. */
    virtual void DeleteAssetFromStorage(String assetRef) = 0;

    /// Removes the storage with the given name from this provider, or returns false if it doesn't exist.
    virtual bool RemoveAssetStorage(String UNUSED_PARAM(storageName)) { return false; }

    /// Returns the list of all asset storages registered into this asset provider.
    virtual Vector<AssetStoragePtr> Storages() const = 0;

    virtual AssetStoragePtr StorageByName(const String &name) const = 0;

    virtual AssetStoragePtr StorageForAssetRef(const String &assetRef) const = 0;

    /// Starts an asset upload from the given file in memory to the given storage.
    /** The default implementation fails all upload attempts and returns 0 immediately. */
    virtual AssetUploadTransferPtr UploadAssetFromFileInMemory(const u8 * UNUSED_PARAM(data), uint UNUSED_PARAM(numBytes),
        AssetStoragePtr UNUSED_PARAM(destination), const String & UNUSED_PARAM(assetName))
    {
        return AssetUploadTransferPtr();
    }

private:
    /// Reads the given storage string and tries to deserialize it to an asset storage in this provider.
    /** Returns a pointer to the newly created storage, or 0 if the storage string is not of the type of this asset provider.
        @note If you want to add storages from code use AssetAPI::DeserializeAssetStorageFromString. */
    virtual AssetStoragePtr TryCreateStorage(HashMap<String, String> &storageParams, bool fromNetwork) = 0;
};

}
