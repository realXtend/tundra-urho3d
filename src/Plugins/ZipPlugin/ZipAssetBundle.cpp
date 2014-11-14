// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "ZipAssetBundle.h"
#include "ZipHelpers.h"
#include "ZipWorker.h"

#include "CoreDefines.h"
#include "Framework.h"
#include "FrameAPI.h"
#include "AssetAPI.h"
#include "AssetCache.h"
#include "LoggingFunctions.h"

#include <FileSystem.h>
#include <zzip/zzip.h>

namespace Tundra
{

ZipAssetBundle::ZipAssetBundle(AssetAPI *owner, const String &type, const String &name) :
    IAssetBundle(owner, type, name),
    worker_(0),
    archive_(0),
    fileCount_(-1),
    done_(false),
    success_(false)
{
}

ZipAssetBundle::~ZipAssetBundle()
{
    Unload();
}

void ZipAssetBundle::DoUnload()
{
    Close();
    StopThread();

    fileCount_ = -1;
}

void ZipAssetBundle::Close()
{
    if (archive_)
    {
        zzip_dir_close(archive_);
        archive_ = 0;
    }
}

bool ZipAssetBundle::DeserializeFromDiskSource()
{
    if (!assetAPI_->Cache())
    {
        LogError("ZipAssetBundle::DeserializeFromDiskSource: Cannot process archive, AssetAPI cache is null.");
        return false;
    }
    else if (DiskSource().Empty())
    {
        LogError("ZipAssetBundle::DeserializeFromDiskSource: Cannot process archive, no disk source for " + Name());
        return false;
    }

    /* We want to detect if the extracted files are already up to date to save time.
       If the last modified date for the sub asset is the same as the parent zip file, 
       we don't extract it. If the zip is re-downloaded from source everything will get unpacked even
       if only one file would have changed inside it. We could do uncompressed size comparisons
       but that is not a absolute guarantee that the file has not changed. We'll be on the safe side
       to unpack the whole zip file. Zip files are meant for deploying the scene and should be touched
       rather rarely. Note that local:// refs are unpacked to cache but the zips disk source is not in the
       cache. Meaning that local:// zip files will always be extracted fully even if the disk source
       was not changed, we don't have a mechanism to get the last modified date properly except from
       the asset cache. For local scenes this should be fine as there is no real need to
       zip the scene up as you already have the disk sources right there in the storage.
       The last modified query will fail if the file is open with zziplib, do it first. */
    uint zipLastModified = assetAPI_->Cache()->LastModified(Name());

    const String diskSourceInternal = Urho3D::GetInternalPath(DiskSource());

    zzip_error_t error = ZZIP_NO_ERROR;
    archive_ = zzip_dir_open(diskSourceInternal.CString(), &error);
    if (CheckAndLogZzipError(error) || CheckAndLogArchiveError(archive_) || !archive_)
    {
        archive_ = 0;
        return false;
    }
    
    int uncompressing = 0;
    
    ZZIP_DIRENT archiveEntry;
    while(zzip_dir_read(archive_, &archiveEntry))
    {
        String relativePath = Urho3D::GetInternalPath(archiveEntry.d_name);
        if (!relativePath.EndsWith("/"))
        {
            String subAssetRef = GetFullAssetReference(relativePath);
            
            ZipArchiveFile file;
            file.relativePath = relativePath;
            file.cachePath = Urho3D::GetInternalPath(assetAPI_->Cache()->DiskSourceByRef(subAssetRef));
            file.lastModified = assetAPI_->Cache()->LastModified(subAssetRef);
            file.compressedSize = archiveEntry.d_csize;
            file.uncompressedSize = archiveEntry.st_size;
            
            /* Mark this file for extraction. If both cache files have valid dates
               and they differ extract. If they have the same date stamp skip extraction.
               Note that file.lastModified will be non-valid for non cached files so we 
               will cover also missing files. */
            file.doExtract = (zipLastModified > 0 && file.lastModified > 0) ? (zipLastModified != file.lastModified) : true;
            if (file.doExtract)
                uncompressing++;

            files_.Push(file);
            fileCount_++;
        }
    }
    
    // Close the zzip directory ptr
    Close();
    
    // If the zip file was empty we don't want IsLoaded to fail on the files_ check.
    // The bundle loaded fine but there was no content, log a warning.
    if (files_.Empty())
    {
        LogWarning("ZipAssetBundle: Bundle loaded but does not contain any files " + Name());
        files_.Push(ZipArchiveFile());
        Loaded.Emit(this);
        return true;
    }
    
    // Don't spin the worker if all sub assets are up to date in cache.
    if (uncompressing > 0)
    {   
        // Now that the file info has been read, continue in a worker thread.
        LogDebug("ZipAssetBundle: File information read for " + Name() + ". File count: " + String(files_.Size()) + ". Starting worker thread to uncompress " + String(uncompressing) + " files.");
        
        // ZipWorker is a QRunnable we can pass to QThreadPool, it will handle scheduling it and deletes it when done.
        worker_ = new ZipWorker(this, zipLastModified, diskSourceInternal, files_);   
        if (!worker_->Run())
        {
            LogError("ZipAssetBundle: Failed to start worker thread for " + Name());
            files_.Clear();
            return false;
        }

        assetAPI_->GetFramework()->Frame()->Updated.Connect(this, &ZipAssetBundle::CheckDone);
    }
    else
        Loaded.Emit(this);
        
    return true;
}

bool ZipAssetBundle::DeserializeFromData(const u8* /*data*/, uint /*numBytes*/)
{
    /** @note At this point it seems zzip needs a disk source to do processing
        so we require disk source for the archive. This might change in the future by changing the lib. */
    return false;
}

Vector<u8> ZipAssetBundle::GetSubAssetData(const String &subAssetName)
{
    /* Makes no sense to keep the whole zip file contents in memory as only
       few files could be wanted from a 100mb bundle. Additionally all asset would take 2x the memory.
       We could make this function also open the zip file and uncompress the data for every sub asset request. 
       But that would be rather pointless, not to mention slower, as we already have the unpacked individual 
       assets on disk. If the unpacking to disk changes we might need to rethink this. */

    String filePath = GetSubAssetDiskSource(subAssetName);
    if (filePath.Empty())
        return Vector<u8>();

    Vector<u8> data;
    return LoadFileToVector(filePath, data) ? data : Vector<u8>();
}

String ZipAssetBundle::GetSubAssetDiskSource(const String &subAssetName)
{
    return assetAPI_->Cache()->FindInCache(GetFullAssetReference(subAssetName));
}

String ZipAssetBundle::GetFullAssetReference(const String &subAssetName)
{
    return Name() + "#" + subAssetName;
}

bool ZipAssetBundle::IsLoaded() const
{
    return (archive_ != 0 || !files_.Empty());
}

void ZipAssetBundle::CheckDone(float /*frametime*/)
{
    // Invoked in main thread context
    {
        Urho3D::MutexLock m(mutexDone_);
        if (!done_)
            return;

        if (success_)
            Loaded.Emit(this);
        else
            Failed.Emit(this);
    }
    StopThread();

    assetAPI_->GetFramework()->Frame()->Updated.Disconnect(this, &ZipAssetBundle::CheckDone);
}

void ZipAssetBundle::WorkerDone(bool successful)
{
    // Invoked in worker thread context
    
    Urho3D::MutexLock m(mutexDone_);
    done_ = true;
    success_ = successful;
}

void ZipAssetBundle::StopThread()
{
    if (worker_)
        worker_->Stop();
    SAFE_DELETE(worker_);
}

Urho3D::Context *ZipAssetBundle::Context() const
{
    return assetAPI_->GetContext();
}

Urho3D::FileSystem *ZipAssetBundle::FileSystem() const
{
    return assetAPI_->GetSubsystem<Urho3D::FileSystem>();
}

}
