// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreTypes.h"

namespace Urho3D
{
class FileWatcher;
}

namespace Tundra
{

class Framework;
class AssetAPI;
class AssetCache;

class IAsset;
typedef SharedPtr<IAsset> AssetPtr;
typedef WeakPtr<IAsset> AssetWeakPtr;

class IAssetBundle;
typedef SharedPtr<IAssetBundle> AssetBundlePtr;
typedef WeakPtr<IAssetBundle> AssetBundleWeakPtr;

class IAssetTransfer;
typedef SharedPtr<IAssetTransfer> AssetTransferPtr;
typedef WeakPtr<IAssetTransfer> AssetTransferWeakPtr;

class AssetBundleMonitor;
typedef SharedPtr<AssetBundleMonitor> AssetBundleMonitorPtr;
typedef WeakPtr<AssetBundleMonitor> AssetBundleMonitorWeakPtr;
struct SubAssetLoader;

class IAssetProvider;
typedef SharedPtr<IAssetProvider> AssetProviderPtr;
typedef WeakPtr<IAssetProvider> AssetProviderWeakPtr;

class IAssetStorage;
typedef SharedPtr<IAssetStorage> AssetStoragePtr;
typedef WeakPtr<IAssetStorage> AssetStorageWeakPtr;

class IAssetUploadTransfer;
typedef SharedPtr<IAssetUploadTransfer> AssetUploadTransferPtr;

struct AssetReference;
struct AssetReferenceList;

class IAssetTypeFactory;
typedef SharedPtr<IAssetTypeFactory> AssetTypeFactoryPtr;

class IAssetBundleTypeFactory;
typedef SharedPtr<IAssetBundleTypeFactory> AssetBundleTypeFactoryPtr;

class AssetRefListener;
typedef SharedPtr<AssetRefListener> AssetRefListenerPtr;

class Framework;
class BinaryAsset;
typedef SharedPtr<BinaryAsset> BinaryAssetPtr;

class LocalAssetProvider;
typedef SharedPtr<LocalAssetProvider> LocalAssetProviderPtr;
class LocalAssetStorage;
typedef SharedPtr<LocalAssetStorage> LocalAssetStoragePtr;

}
