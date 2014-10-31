// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "Math/MathNamespace.h"
#include "IMeshAsset.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"

namespace Tundra
{

/// Represents a mesh asset in Urho native format.
class URHO_MODULE_API UrhoMeshAsset : public IMeshAsset
{
    OBJECT(UrhoMeshAsset);

public:
    UrhoMeshAsset(AssetAPI *owner, const String &type_, const String &name_);

    /// Load mesh from memory. IAsset override.
    bool DeserializeFromData(const u8 *data_, uint numBytes, bool allowAsynchronous) override;
};

}
