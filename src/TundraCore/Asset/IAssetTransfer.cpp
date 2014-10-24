// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "IAssetTransfer.h"
#include "IAssetProvider.h"
#include "IAsset.h"

#include "LoggingFunctions.h"

namespace Tundra
{

IAssetTransfer::IAssetTransfer() : 
    cachingAllowed(true),
    diskSourceType(IAsset::Original)
{
}

IAssetTransfer::~IAssetTransfer()
{
}

void IAssetTransfer::EmitAssetDownloaded()
{
    Downloaded.Emit(this);
}

void IAssetTransfer::EmitAssetDownloaded(IAssetTransfer* transfer)
{
    Downloaded.Emit(transfer);
}

void IAssetTransfer::EmitTransferSucceeded()
{
    Succeeded.Emit(this->asset);
}

void IAssetTransfer::EmitTransferSucceeded(AssetPtr asset)
{
    Succeeded.Emit(asset);
}

void IAssetTransfer::EmitAssetFailed(String reason)
{
    Failed.Emit(this, reason);
}

void IAssetTransfer::EmitAssetFailed(IAssetTransfer* transfer, String reason)
{
    Failed.Emit(transfer, reason);
}

bool IAssetTransfer::Abort()
{
    if (provider.Lock().Get())
        return provider.Lock()->AbortTransfer(this);
    else
    {    
        LogWarning("IAssetTransfer::Abort() Provider is null, cannot call IAssetProvider::AbortTransfer()."); 
        return false;
    }
}

void IAssetTransfer::SetCachingBehavior(bool cachingAllowed, String diskSource)
{
    this->cachingAllowed = cachingAllowed; 
    this->diskSource = diskSource;
}

String IAssetTransfer::DiskSource() const
{
    return diskSource;
}

IAsset::SourceType IAssetTransfer::DiskSourceType() const
{
    return diskSourceType;
}

bool IAssetTransfer::CachingAllowed() const
{
    return cachingAllowed;
}

String IAssetTransfer::SourceUrl() const
{
    return source.ref;
}

String IAssetTransfer::AssetType() const
{
    return assetType;
}

AssetPtr IAssetTransfer::Asset() const
{
    return asset;
}

}

