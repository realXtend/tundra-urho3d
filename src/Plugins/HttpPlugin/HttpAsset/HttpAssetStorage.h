// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpPluginApi.h"
#include "IAssetStorage.h"

namespace Tundra
{

/// Represents a single (possibly recursive) directory on the local file system.
class TUNDRA_HTTP_API HttpAssetStorage : public IAssetStorage
{
    OBJECT(HttpAssetStorage);

public:
    HttpAssetStorage(Urho3D::Context* context, const String &name, const String &baseUrl, const String &localDir);

    /// IAssetStorage override.
    String Type() const override;
    /// IAssetStorage override.
    String Name() const override;
    /// IAssetStorage override.
    String BaseURL() const override;
    /// IAssetStorage override.
    String SerializeToString(bool networkTransfer = false) const override;

private:
    String name_;
    String baseUrl_;
    String localDir_;
};

}
