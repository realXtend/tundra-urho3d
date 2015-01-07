// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "ZipPluginApi.h"
#include "ZipPluginFwd.h"

#include <Urho3D/Core/Thread.h>

/// @cond PRIVATE
struct zzip_dir;
/// @endcond

namespace Tundra
{

/// Worker thread that unpacks zip file contents.
class TUNDRA_ZIP_API ZipWorker : public Urho3D::Thread
{
public:
    ZipWorker(ZipAssetBundle *owner, uint zipLastModified, const String &diskSource, const ZipFileVector &files);
    ~ZipWorker();

    /// Urho3D::Thread override
    void ThreadFunction() override;
    
private:
    void Close();

    ZipAssetBundle *owner_;
    
    String diskSource_;
    ZipFileVector files_;
    zzip_dir *archive_;
    uint zipLastModified_;
};

}
