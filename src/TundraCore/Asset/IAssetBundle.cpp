// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "IAssetBundle.h"
#include "IAssetTransfer.h"
#include "AssetAPI.h"
#include "IAssetProvider.h"
#include "LoggingFunctions.h"

namespace Tundra
{

IAssetBundle::IAssetBundle(AssetAPI *assetAPI, const String &type, const String &name) :
    Object(assetAPI->GetContext()),
    assetAPI_(assetAPI),
    type_(type),
    name_(name)
{
}

bool IAssetBundle::IsEmpty() const
{
    return !IsLoaded() && diskSource_.Empty();
}

String IAssetBundle::Type() const
{
    return type_;
}

String IAssetBundle::Name() const
{
    return name_;
}

AssetStoragePtr IAssetBundle::AssetStorage() const
{
    return storage.Lock();
}

AssetProviderPtr IAssetBundle::AssetProvider() const
{
    return provider.Lock();
}

String IAssetBundle::DiskSource() const
{
    return diskSource_;
}

void IAssetBundle::SetAssetProvider(AssetProviderPtr provider_)
{
    provider = provider_;
}

void IAssetBundle::SetAssetStorage(AssetStoragePtr storage_)
{
    storage = storage_;
}

void IAssetBundle::SetDiskSource(String diskSource)
{
    diskSource_ = diskSource;
}

void IAssetBundle::Unload()
{
    DoUnload();
    Unloaded.Emit(this);
}

// AssetBundleMonitor

AssetBundleMonitor::AssetBundleMonitor(AssetAPI *owner, AssetTransferPtr bundleTransfer) :
    assetAPI_(owner),
    bundleTransfer_(bundleTransfer),
    bundleRef_(bundleTransfer->source.ref)
{
    if (bundleTransfer_)
    bundleTransfer_->Failed.Connect(this, &AssetBundleMonitor::BundleFailed);
}

AssetBundleMonitor::~AssetBundleMonitor()
{
    childTransfers_.Clear();
    bundleTransfer_.Reset();
}

void AssetBundleMonitor::AddSubAssetTransfer(AssetTransferPtr transfer)
{
    childTransfers_.Push(transfer);
}

AssetTransferPtr AssetBundleMonitor::SubAssetTransfer(const String &fullSubAssetRef)
{
    for(Vector<AssetTransferPtr>::Iterator subTransferIter=childTransfers_.Begin(); subTransferIter!=childTransfers_.End(); ++subTransferIter)
        if ((*subTransferIter)->source.ref == fullSubAssetRef)
            return (*subTransferIter);
    return AssetTransferPtr();
}

Vector<AssetTransferPtr> AssetBundleMonitor::SubAssetTransfers()
{
    return childTransfers_;
}

AssetTransferPtr AssetBundleMonitor::BundleTransfer()
{
    return bundleTransfer_;
}

String AssetBundleMonitor::BundleAssetRef()
{
    return bundleRef_;
}

void AssetBundleMonitor::BundleFailed(IAssetTransfer* /*transfer*/, String /*reason*/)
{
    for(Vector<AssetTransferPtr>::Iterator subTransferIter=childTransfers_.Begin(); subTransferIter!=childTransfers_.End(); ++subTransferIter)
    {
        AssetTransferPtr transfer = (*subTransferIter);
        if (transfer.Get())
        {
            String error = "Failed to load parent asset bundle \"" + bundleRef_ + "\" for sub asset \"" + transfer->source.ref + "\".";
            transfer->EmitAssetFailed(error);
            LogError("AssetBundleMonitor: " + error);
        }
    }
}

}
