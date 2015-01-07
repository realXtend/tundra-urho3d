// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreTypes.h"

#include <Urho3D/Container/Str.h>
#include <Urho3D/Container/Ptr.h>

namespace Tundra
{
    class ZipBundleFactory;
    class ZipAssetBundle;
    class ZipWorker;
    
    /// @cond PRIVATE
    struct ZipArchiveFile
    {
        String relativePath;
        String cachePath;
        uint compressedSize;
        uint uncompressedSize;
        uint lastModified;
        bool doExtract;
    };
    typedef Vector<ZipArchiveFile> ZipFileVector;
    /// @endcond
}
