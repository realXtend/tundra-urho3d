// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "HttpAssetStorage.h"

#include "AssetAPI.h"

#include <Engine/Core/StringUtils.h>

namespace Tundra
{

HttpAssetStorage::HttpAssetStorage(Urho3D::Context* context, const String &name, const String &baseUrl, const String &localDir) :
    IAssetStorage(context),
    // HttpAssetStorage
    name_(name),
    baseUrl_(baseUrl),
    localDir_(localDir.Empty() ? localDir : GuaranteeTrailingSlash(localDir))
{
    // IAssetStorage
    writable = false;
    liveUpdate = false;
    autoDiscoverable = false;
    isReplicated = true;
}

String HttpAssetStorage::Type() const
{
    return "HttpAssetStorage";
}

String HttpAssetStorage::Name() const
{
    return name_;
}

String HttpAssetStorage::BaseURL() const
{
    return baseUrl_;
}

String HttpAssetStorage::GetFullAssetURL(const String &localName)
{
    if (localName.StartsWith("/", true))
        return baseUrl_ + localName.Substring(1);
    return baseUrl_ + localName;
}

String HttpAssetStorage::SerializeToString(bool networkTransfer) const
{
    String serialized =
        "type=" + Type() + ";name=" + name_ +  ";src=" + baseUrl_ +
        ";readonly=" + String(!writable) +
        ";liveupdate=" + String(liveUpdate) + 
        ";liveupload=" + String(liveUpload) + 
        ";autodiscoverable=" + String(autoDiscoverable) +
        ";replicated=" + String(isReplicated) +
        ";trusted=" + TrustStateToString(trustState);
    if (!networkTransfer && !localDir_.Empty())
        serialized += ";localdir=" + localDir_;
    serialized += ";";
    return serialized;
}

}
