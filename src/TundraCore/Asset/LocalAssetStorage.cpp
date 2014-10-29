// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "LocalAssetStorage.h"
#include "LocalAssetProvider.h"
#include "AssetAPI.h"
#include "LoggingFunctions.h"

#include <Profiler.h>
#include <File.h>
#include <FileSystem.h>
#include <FileWatcher.h>
#include <StringUtils.h>

namespace Tundra
{

LocalAssetStorage::LocalAssetStorage(Urho3D::Context* context, bool writable_, bool liveUpdate_, bool autoDiscoverable_) :
    IAssetStorage(context),
    recursive(true),
    changeWatcher(0)
{
    // Override the parameters for the base class.
    writable = writable_;
    liveUpdate = liveUpdate_;
    autoDiscoverable = autoDiscoverable_;
}

LocalAssetStorage::~LocalAssetStorage()
{
    RemoveWatcher();
}

void LocalAssetStorage::LoadAllAssetsOfType(AssetAPI *assetAPI, const String &suffix, const String &assetType)
{
    Urho3D::FileSystem* fileSystem = GetSubsystem<Urho3D::FileSystem>();
    StringVector filenames;
    fileSystem->ScanDir(filenames, directory, "*.*", Urho3D::SCAN_FILES, recursive);
    foreach(String str, filenames)
    {
        if ((suffix == "" || str.EndsWith(suffix)) && !(str.Contains(".git") || str.Contains(".svn") || str.Contains(".hg")))
        {
            uint lastSlash = str.FindLast('/');
            if (lastSlash != String::NPOS)
                str = str.Substring(lastSlash + 1);
            assetAPI->RequestAsset("local://" + str, assetType);
        }
    }
}

void LocalAssetStorage::RefreshAssetRefs()
{
    Urho3D::FileSystem* fileSystem = GetSubsystem<Urho3D::FileSystem>();
    StringVector filenames;
    fileSystem->ScanDir(filenames, directory, "*.*", Urho3D::SCAN_FILES, recursive);

    foreach(String str, filenames)
    {
        if (!(str.Contains(".git") || str.Contains(".svn") || str.Contains(".hg")))
        {
            String diskSource = directory + str;
            uint lastSlash = str.FindLast('/');
            if (lastSlash != String::NPOS)
                str = str.Substring(lastSlash + 1);
            String localName = str;
            str = "local://" + str;
            if (!assetRefs.Contains(str))
            {
                assetRefs.Push(str);
                AssetChanged.Emit(this, localName, diskSource, IAssetStorage::AssetCreate);
            }
        }
    }
}

void LocalAssetStorage::CacheStorageContents()
{
    cachedFiles.clear();
    Urho3D::FileSystem* fileSystem = GetSubsystem<Urho3D::FileSystem>();
    StringVector filenames;
    fileSystem->ScanDir(filenames, directory, "*.*", Urho3D::SCAN_FILES, recursive);

    foreach(String str, filenames)
    {
        if (!(str.Contains(".git") || str.Contains(".svn") || str.Contains(".hg")))
        {
            String diskSource = directory + str;
            uint lastSlash = str.FindLast('/');
            if (lastSlash != String::NPOS)
                str = str.Substring(lastSlash + 1);
            String localName = str;

///\todo This is an often-received error condition if the user is not aware, but also occurs naturally in built-in Ogre Media storages.
/// Fix this check to occur somehow nicer (without additional constraints to asset load time) without a hardcoded check
/// against the storage name.
            if (cachedFiles.find(localName) != cachedFiles.end())
                LogWarning("Warning: Asset Storage \"" + Name() + "\" contains ambiguous assets \"" + cachedFiles[localName] + "\" and \"" + diskSource + "\" in two different subdirectories!");

            cachedFiles[localName] = diskSource;
        }
    }
}

String LocalAssetStorage::GetFullPathForAsset(const String &assetname, bool recursiveLookup)
{
    Urho3D::FileSystem* fileSystem = GetSubsystem<Urho3D::FileSystem>();
    if (fileSystem->FileExists(directory + assetname))
        return directory;

    std::map<String, String, StringCompareCaseInsensitive>::iterator iter = cachedFiles.find(assetname);
    if (iter == cachedFiles.end())
    {
        if (!recursive || !recursiveLookup)
            return "";
        else
            CacheStorageContents();
    }

    iter = cachedFiles.find(assetname);
    if (iter != cachedFiles.end())
    {
        if (fileSystem->FileExists(iter->second))
            return directory;
    }

    return "";
}

/// @todo Make this function handle arbitrary asset refs.
String LocalAssetStorage::GetFullAssetURL(const String &localName)
{
    String filename;
    String subAssetName;
    AssetAPI::ParseAssetRef(localName, 0, 0, 0, 0, &filename, 0, 0, &subAssetName);
    return BaseURL() + filename + (subAssetName.Empty() ? "" : ("#" + subAssetName));
}

String LocalAssetStorage::Type() const
{
    return "LocalAssetStorage";
}

String LocalAssetStorage::SerializeToString(bool networkTransfer) const
{
    if (networkTransfer)
        return ""; // Cannot transfer a LocalAssetStorage through network to another computer, since it is local to this system!
    else
        return "type=" + Type() + ";name=" + name + ";src=" + directory + ";recursive=" + String(recursive) + ";readonly=" + String(!writable) +
            ";liveupdate=" + String(liveUpdate) + ";autodiscoverable=" + String(autoDiscoverable) + ";replicated=" + String(isReplicated)
            + ";trusted=" + TrustStateToString(GetTrustState());
}

void LocalAssetStorage::EmitAssetChanged(String absoluteFilename, IAssetStorage::ChangeType change)
{
    String localName;
    uint lastSlash = absoluteFilename.FindLast('/');
    if (lastSlash != String::NPOS)
        localName = absoluteFilename.Substring(lastSlash + 1);
    String assetRef = "local://" + localName;
    if (assetRefs.Contains(assetRef) && change == IAssetStorage::AssetCreate)
        LogDebug("LocalAssetStorage::EmitAssetChanged: Emitting AssetCreate signal for already existing asset " + assetRef +
            ", file " + absoluteFilename + ". Asset was probably removed and then added back.");
    AssetChanged.Emit(this, localName, absoluteFilename, change);
}

void LocalAssetStorage::SetupWatcher()
{
    if (changeWatcher) // Remove the old watcher if one exists.
        RemoveWatcher();

    changeWatcher = new Urho3D::FileWatcher(GetContext());
    changeWatcher->StartWatching(directory, recursive);
    LogInfo("LocalAssetStorage: started watching " + directory);
}

void LocalAssetStorage::RemoveWatcher()
{
    changeWatcher.Reset();
}

}
