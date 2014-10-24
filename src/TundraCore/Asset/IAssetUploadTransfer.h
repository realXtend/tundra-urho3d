// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreTypes.h"
#include "AssetFwd.h"
#include "IAssetStorage.h"
#include "Signals.h"

namespace Tundra
{

/// Represents a currently ongoing asset upload operation.
class TUNDRACORE_API IAssetUploadTransfer : public Object
{
    OBJECT(IAssetUploadTransfer);

public:
    IAssetUploadTransfer(Urho3D::Context* context);

    virtual ~IAssetUploadTransfer();

    /// Returns the current transfer progress in the range [0, 1].
    virtual float Progress() const { return 0.f; }

    /// Specifies the source file of the upload transfer, or none if this upload does not originate from a file in the system.
    String sourceFilename;

    /// Contains the raw asset data to upload. If sourceFilename=="", the data is taken from this array instead.
    Vector<u8> assetData;

    /// Contains the reply from the storage if one was provided. Eg. HTTP PUT/POST may give response data in the body.
    Vector<u8> replyData;

    /// Headers returned by the upload.
    HashMap<String, String> replyHeaders;

    /// Specifies the destination name for the asset.
    String destinationName;

    /// Destination storage.
    WeakPtr<IAssetStorage> destinationStorage;

    /// Destination provider.
    AssetProviderWeakPtr destinationProvider;

    /// Emits Completed signal.
    void EmitTransferCompleted();

    /// Emits Failed signal.
    void EmitTransferFailed();

    /// Returns the full assetRef address this asset will have when the upload is complete.
    String AssetRef();

    /// Returns a copy of the raw asset data in this upload.
    Vector<u8> RawData() const { return assetData; }

    /// Returns the raw reply data returned by this upload.
    Vector<u8> RawReplyData() const { return replyData; }

    /// Returns a header value for the passed in header name. The name check is case insensitive.
    /** @note Headers can be empty or can have values for certain providers like HTTP. 
        @param Header name.
        @return Header value or empty string if not found. */
    String ReplyHeader(const String &header) const;

    /// @copydoc sourceFilename
    String SourceFilename() const;

    /// @copydoc destinationName
    String DestinationName() const;

    /// Emitted when upload completes successfully.
    Signal1<IAssetUploadTransfer*> Completed;

    /// Emitted when upload fails.
    Signal1<IAssetUploadTransfer*> Failed;
};

}
