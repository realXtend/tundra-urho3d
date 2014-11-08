// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "Math/MathNamespace.h"
#include "IAsset.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"

namespace Tundra
{

/// Represents a mesh loaded to the GPU.
/** Base class for varying format implementations (Urho mesh, Ogre mesh, Assimp mesh etc.) */
class URHO_MODULE_API IMeshAsset : public IAsset
{
    OBJECT(IMeshAsset);

public:
    IMeshAsset(AssetAPI *owner, const String &type_, const String &name_);
    ~IMeshAsset();

    /// Returns the Urho model resource
    Urho3D::Model* UrhoModel() const;

    /// IAsset override.
    bool IsLoaded() const override;

    /// Returns submesh count.
    uint NumSubmeshes() const;

protected:
    /// Unload mesh. IAsset override.
    void DoUnload() override;

    /// Urho model resource. Filled by the loading implementations in subclasses.
    SharedPtr<Urho3D::Model> model;
};

}
