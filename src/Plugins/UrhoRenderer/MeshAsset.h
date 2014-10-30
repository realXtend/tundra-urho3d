// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "Math/MathNamespace.h"
#include "IAsset.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"

namespace Tundra
{

/// Represents a mesh loaded to the GPU.
class URHO_MODULE_API MeshAsset : public IAsset
{
    OBJECT(MeshAsset);

public:
    MeshAsset(AssetAPI *owner, const String &type_, const String &name_);
    ~MeshAsset();

    /// Load mesh from memory. IAsset override.
    virtual bool DeserializeFromData(const u8 *data_, size_t numBytes, bool allowAsynchronous);

    /// IAsset override.
    virtual bool IsLoaded() const;

    /// Returns submesh count.
    size_t NumSubmeshes() const;

    /// Returns the Urho model resource
    Urho3D::Model* UrhoModel() const;

private:
    /// Unload mesh. IAsset override.
    virtual void DoUnload();

    /// Urho model resource.
    SharedPtr<Urho3D::Model> model;
};

}
