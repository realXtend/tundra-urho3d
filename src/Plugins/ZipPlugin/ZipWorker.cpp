// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "ZipWorker.h"
#include "ZipHelpers.h"
#include "ZipAssetBundle.h"

#include "LoggingFunctions.h"

#include <IO/File.h>
#include <IO/FileSystem.h>
#include <Container/Sort.h>

#include <zzip/zzip.h>

namespace Tundra
{

bool ArchiveFileSizeCompare(const ZipArchiveFile &f1, const ZipArchiveFile &f2)
{
    return (f1.uncompressedSize < f2.uncompressedSize);
}

ZipWorker::ZipWorker(ZipAssetBundle *owner, uint zipLastModified, const String &diskSource, const ZipFileVector &files) :
    owner_(owner),
    zipLastModified_(zipLastModified),
    diskSource_(diskSource),
    files_(files),
    archive_(0)
{
}

ZipWorker::~ZipWorker()
{
    Close();
}

void ZipWorker::ThreadFunction()
{
    zzip_error_t error = ZZIP_NO_ERROR;
    archive_ = zzip_dir_open(diskSource_.CString(), &error);
    if (CheckAndLogZzipError(error) || CheckAndLogArchiveError(archive_) || !archive_)
    {
        archive_ = 0;
        owner_->WorkerDone(false);
        return;
    }

    Urho3D::FileSystem *fs = owner_->FileSystem();
    
    // Sort by size so we can resize the buffer less often.
    Urho3D::Sort(files_.Begin(), files_.End(), ArchiveFileSizeCompare);

    // Read file contents
    bool success = true;
    zzip_ssize_t chunkLen = 0;
    zzip_ssize_t chunkRead = 0;
    Vector<u8> buffer;

    foreach(const ZipArchiveFile &file, files_)
    {
        if (!file.doExtract)
            continue;

        // Open file from zip
        ZZIP_FILE *zzipFile = zzip_file_open(archive_, file.relativePath.CString(), ZZIP_ONLYZIP | ZZIP_CASELESS);
        if (!zzipFile || CheckAndLogArchiveError(archive_))
        {
            success = false;
            break;
        }

        // Create cache file
        Urho3D::File cacheFile(owner_->Context(), file.cachePath, Urho3D::FILE_WRITE);
        if (!cacheFile.IsOpen())
        {
            LogError("ZipWorker: Failed to open cache file: " + file.cachePath + ". Cannot unzip " + file.relativePath);
            continue;
        }

        // Detect file size and adjust buffer (quite naive atm but is a slight speed improvement)
        if (file.uncompressedSize > 1000*1024)
            chunkLen = 500*1024;
        else if (file.uncompressedSize > 500*1024)
            chunkLen = 250*1024;
        else if (file.uncompressedSize > 100*1024)
            chunkLen = 50*1024;
        else if (file.uncompressedSize > 20*1024)
            chunkLen = 10*1024;
        else
            chunkLen = 5*1024;

        if (buffer.Size() < (uint)chunkLen)
            buffer.Resize(chunkLen);

        // Read zip file content to cache file
        while (0 < (chunkRead = zzip_read(zzipFile, &buffer[0], chunkLen)))
        {
            if (cacheFile.Write((void*)&buffer[0], (uint)chunkRead) != (uint)chunkRead)
            {
                LogError("Failed to write cache file + " + file.cachePath);
                success = false;
                break;
            }
        }

        // Close zip and cache file.
        zzip_file_close(zzipFile);
        cacheFile.Close();

        if (!success)
            break;

        // Update last modified same to the parent zip file.
        if (success && zipLastModified_ > 0)
            fs->SetLastModifiedTime(file.cachePath, zipLastModified_);
    }

    // Close the zzip directory ptr
    Close();

    owner_->WorkerDone(success);
}

void ZipWorker::Close()
{
    if (archive_)
    {
        zzip_dir_close(archive_);
        archive_ = 0;
    }
}

}
