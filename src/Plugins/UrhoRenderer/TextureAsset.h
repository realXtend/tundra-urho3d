// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "Math/MathNamespace.h"
#include "IAsset.h"
#include "UrhoRendererApi.h"
#include "UrhoRendererFwd.h"

namespace Tundra
{

/// Represents a texture asset loaded to the GPU.
class URHORENDERER_API TextureAsset : public IAsset
{
    URHO3D_OBJECT(TextureAsset, IAsset);

public:
    TextureAsset(AssetAPI *owner, const String &type_, const String &name_);
    ~TextureAsset();

    /// Load asset from memory. IAsset override.
    bool DeserializeFromData(const u8 *data_, uint numBytes, bool allowAsynchronous) override;

    /// IAsset override.
    bool IsLoaded() const override;

    /// Returns Urho3D texture
    Urho3D::Texture2D* UrhoTexture() const;

    /// Get width of the texture. Returns 0 if not loaded.
    size_t Width() const;

    /// Get height of the texture. Returns 0 if not loaded.
    size_t Height() const;

protected:
    /// Unload asset. IAsset override.
    void DoUnload() override;

    /// Urho asset resource.
    SharedPtr<Urho3D::Texture2D> texture;

private:
    void HandleDeviceReset(StringHash eventType, VariantMap& eventData);

    bool DecompressCRNtoDDS(const u8 *crnData, uint crnNumBytes, Vector<u8> &ddsData) const;

    int MaxTextureSize() const;
    void DetermineMipsToSkip(Urho3D::Image* image, Urho3D::Texture2D* texture) const;
};

}
