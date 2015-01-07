// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "AssetCache.h"
#include "AssetAPI.h"
#include "IAsset.h"

#include "CoreDefines.h"
#include "Framework.h"
#include "LoggingFunctions.h"

#include <Urho3D/IO/FileSystem.h>

namespace Tundra
{

AssetCache::AssetCache(AssetAPI *owner, String assetCacheDirectory) : 
    Object(owner->GetContext()),
    assetAPI(owner),
    cacheDirectory(GuaranteeTrailingSlash(Urho3D::GetInternalPath(assetCacheDirectory)))
{
    Urho3D::FileSystem* fileSystem = GetSubsystem<Urho3D::FileSystem>();
    if (!Urho3D::IsAbsolutePath(cacheDirectory))
        cacheDirectory = fileSystem->GetCurrentDir() + cacheDirectory;

    // Check that the main directory exists
    if (!fileSystem->DirExists(cacheDirectory))
        fileSystem->CreateDir(cacheDirectory);

    // Check --clearAssetCache start param
    if (owner->GetFramework()->HasCommandLineParameter("--clearAssetCache") ||
        owner->GetFramework()->HasCommandLineParameter("--clear-asset-cache")) /**< @todo Remove support for the deprecated parameter version at some point. */
    {
        LogInfo("AssetCache: Removing all data and metadata files from cache, found 'clearAssetCache' from the startup params!");
        ClearAssetCache();
    }
}

String AssetCache::FindInCache(const String &assetRef)
{
    Urho3D::FileSystem* fileSystem = GetSubsystem<Urho3D::FileSystem>();
    String absolutePath = DiskSourceByRef(assetRef);
    if (fileSystem->FileExists(absolutePath))
        return absolutePath;
    else // The file is not in cache, return an empty string to denote that.
        return "";
}

String AssetCache::DiskSourceByRef(const String &assetRef)
{
    // Return the path where the given asset ref would be stored, if it was saved in the cache
    // (regardless of whether it now exists in the cache).
    return cacheDirectory + AssetAPI::SanitateAssetRef(assetRef);
}

String AssetCache::CacheDirectory() const
{
    return GuaranteeTrailingSlash(cacheDirectory);
}

String AssetCache::StoreAsset(AssetPtr asset)
{
    Vector<u8> data;
    asset->SerializeTo(data);
    return StoreAsset(&data[0], data.Size(), asset->Name());
}

String AssetCache::StoreAsset(const u8 *data, uint numBytes, const String &assetName)
{
    String absolutePath = DiskSourceByRef(assetName);
    bool success = SaveAssetFromMemoryToFile(data, numBytes, absolutePath);
    if (success)
        return absolutePath;
    return "";
}

unsigned AssetCache::LastModified(const String &assetRef)
{
    Urho3D::FileSystem* fileSystem = GetSubsystem<Urho3D::FileSystem>();
    String absolutePath = FindInCache(assetRef);
    if (absolutePath.Empty())
        return 0;
    return fileSystem->GetLastModifiedTime(absolutePath);
}

bool AssetCache::SetLastModified(const String & assetRef, unsigned dateTime)
{
    Urho3D::FileSystem* fileSystem = GetSubsystem<Urho3D::FileSystem>();
    String absolutePath = FindInCache(assetRef);
    if (absolutePath.Empty())
        return false;
    return fileSystem->SetLastModifiedTime(absolutePath, dateTime);
}

void AssetCache::DeleteAsset(const String &assetRef)
{
    String absolutePath = DiskSourceByRef(assetRef);
    Urho3D::FileSystem* fileSystem = GetSubsystem<Urho3D::FileSystem>();
    if (fileSystem->FileExists(absolutePath))
        fileSystem->Delete(absolutePath);
}

void AssetCache::ClearAssetCache()
{
    Urho3D::FileSystem* fileSystem = GetSubsystem<Urho3D::FileSystem>();
    StringVector filenames;
    fileSystem->ScanDir(filenames, cacheDirectory, "*.*", Urho3D::SCAN_FILES, true);
    foreach(String file, filenames)
        fileSystem->Delete(cacheDirectory + file);
}

}

