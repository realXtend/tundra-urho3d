// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "IAssetUploadTransfer.h"

namespace Tundra
{

IAssetUploadTransfer::~IAssetUploadTransfer()
{
}

void IAssetUploadTransfer::EmitTransferCompleted()
{
    Completed.Emit(this);
}

void IAssetUploadTransfer::EmitTransferFailed()
{
    Failed.Emit(this);
}

String IAssetUploadTransfer::AssetRef()
{
    SharedPtr<IAssetStorage> storage = destinationStorage.Lock();
    if (!storage)
        return "";
    return storage->GetFullAssetURL(destinationName);
}

String IAssetUploadTransfer::ReplyHeader(const String &header) const
{
    StringVector keys = replyHeaders.Keys();
    foreach(const String &name, keys)
    {
        if (name.Compare(header, false) == 0)
            return replyHeaders.Find(name)->second_;
    }
    return "";
}

String IAssetUploadTransfer::SourceFilename() const
{
    return sourceFilename;
}

String IAssetUploadTransfer::DestinationName() const
{
    return destinationName;
}

}
