// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpPluginApi.h"
#include "HttpPluginFwd.h"

#include "IAssetTransfer.h"

namespace Tundra
{

/// HTTP asset transfer
class TUNDRA_HTTP_API HttpAssetTransfer : public IAssetTransfer
{
    URHO3D_OBJECT(HttpAssetTransfer, IAssetTransfer);

public:
    HttpAssetTransfer(HttpAssetProvider *provider, HttpRequestPtr &request, const String &assetRef_, const String &assetType_);
    ~HttpAssetTransfer();

    HttpRequestPtr Request() const { return request_; }

private:
    void OnFinished(HttpRequestPtr &request, int status, const String &error);

    HttpAssetProvider *provider_;
    HttpRequestPtr request_;
};

}
