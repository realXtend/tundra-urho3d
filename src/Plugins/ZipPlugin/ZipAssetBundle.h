// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "ZipPluginApi.h"
#include "ZipPluginFwd.h"

#include "AssetAPI.h"
#include "IAssetBundle.h"

#include <Urho3D/Core/Mutex.h>

namespace Urho3D
{
    class Context;
    class FileSystem;
}

/// @cond PRIVATE
struct zzip_dir;
/// @endcond

namespace Tundra
{

/// Provides zip packed asset bundles.
class TUNDRA_ZIP_API ZipAssetBundle : public IAssetBundle
{
    URHO3D_OBJECT(ZipAssetBundle, IAssetBundle);

public:
    ZipAssetBundle(AssetAPI *owner, const String &type, const String &name);
    ~ZipAssetBundle();

    /// IAssetBundle override.
    bool IsLoaded() const override;

    /// IAssetBundle override.
    /** Our current zziplib implementation requires disk source for processing. */
    bool RequiresDiskSource() override { return true; }

    /// IAssetBundle override.
    /** Our current zziplib implementation requires disk source for processing.
        So we fail DeserializeFromData and try our best here to.
        This function unpacks the archive content to asset cache to normal cache files
        and provides the sub asset data via GetSubAssetData and GetSubAssetDiskSource. */
    bool DeserializeFromDiskSource() override;

    /// IAssetBundle override.
    /** @todo If we must support this in memory method with zzip 
        we could store the data to disk and open it. Be sure to change RequiresDiskSource to false.
        @return Currently not applicable, so false always. */
    bool DeserializeFromData(const u8 *data, uint numBytes) override;

    /// IAssetBundle override.
    /** This does not include sub folders inside the zip file to this count, files in sub folders will be counted. */
    int SubAssetCount() const override { return fileCount_; }

    /// IAssetBundle override.
    Vector<u8> GetSubAssetData(const String &subAssetName) override;

    /// IAssetBundle override.
    String GetSubAssetDiskSource(const String &subAssetName) override;
    
private:
    friend class ZipWorker;

    /// Returns full asset reference for a sub asset.
    String GetFullAssetReference(const String &subAssetName);
    
    /// IAssetBundle override.
    void DoUnload() override;

    /// Closes zip file.
    void Close();

    /// Check if worker has completed.
    void CheckDone(float frametime);

    /// Handler for asynch loading completion.
    /** Invoked in worker thread context. */
    void WorkerDone(bool successful);

    /// Stops and destroys thread.
    /** @note Only call in main thread context. */
    void StopThread();

    Urho3D::Context *Context() const;
    Urho3D::FileSystem *FileSystem() const;

    /// Zziplib ptr to the zip file.
    zzip_dir *archive_;

    /// Zip sub assets.
    ZipFileVector files_;

    /// Worker
    ZipWorker *worker_;

    /// Count of files inside this zip.
    int fileCount_;

    /// Mutex for polling completion of worker thread.
    Urho3D::Mutex mutexDone_;
    bool done_;
    bool success_;
};
typedef SharedPtr<ZipAssetBundle> ZipAssetBundlePtr;

}
