// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Renderer.h"
#include "AssetAPI.h"
#include "AssetCache.h"
#include "Profiler.h"
#include "LoggingFunctions.h"
#include "TextureAsset.h"

#include "Crunch/crn_decomp.h"
#include "Crunch/dds_defs.h"

#include <MemoryBuffer.h>
#include <Texture2D.h>
#include <Material.h>

namespace Tundra
{

TextureAsset::TextureAsset(AssetAPI *owner, const String &type_, const String &name_) :
    IAsset(owner, type_, name_)
{
}

TextureAsset::~TextureAsset()
{
    Unload();
}

bool TextureAsset::DeserializeFromData(const u8 *data_, uint numBytes, bool /*allowAsynchronous*/)
{
    PROFILE(TextureAsset_LoadFromFileInMemory);

    bool success = false;

    /// Force an unload of previous data first.
    Unload();

    if (!Name().EndsWith(".crn", false))
    {
        Urho3D::MemoryBuffer imageBuffer(data_, numBytes);
        texture = new Urho3D::Texture2D(GetContext());
        success = texture->Load(imageBuffer);
    }
    else
    {
        Vector<u8> ddsData;
        success = DecompressCRNtoDDS(data_, numBytes, ddsData);
        if (success)
        {
            Urho3D::MemoryBuffer imageBuffer(&ddsData[0], ddsData.Size());
            texture = new Urho3D::Texture2D(GetContext());
            success = texture->Load(imageBuffer);
        }
    }

    if (success)
        assetAPI->AssetLoadCompleted(Name());
    else
    {
        LogError("TextureAsset::DeserializeFromData: Failed to load texture asset " + Name());
        texture.Reset();
    }

    return success;
}

bool TextureAsset::DecompressCRNtoDDS(const u8 *crnData, uint crnNumBytes, Vector<u8> &ddsData) const
{
    PROFILE(TextureAsset_DecompressCRNtoDDS);

    // Texture data
    crnd::crn_texture_info textureInfo;
    if (!crnd::crnd_get_texture_info((void*)crnData, (crnd::uint32)crnNumBytes, &textureInfo))
    {
        LogError("CRN texture info parsing failed, invalid input data.");
        return false;
    }
    // Begin unpack
    crnd::crnd_unpack_context crnContext = crnd::crnd_unpack_begin((void*)crnData, (crnd::uint32)crnNumBytes);
    if (!crnContext)
    {
        LogError("CRN texture data unpacking failed, invalid input data.");
        return false;
    }

    // DDS header
    crnlib::DDSURFACEDESC2 header;
    memset(&header, 0, sizeof(header));
    header.dwSize = sizeof(header);
    // - Size and flags
    header.dwFlags = crnlib::DDSD_CAPS | crnlib::DDSD_HEIGHT | crnlib::DDSD_WIDTH | crnlib::DDSD_PIXELFORMAT | ((textureInfo.m_levels > 1) ? crnlib::DDSD_MIPMAPCOUNT : 0);
    header.ddsCaps.dwCaps = crnlib::DDSCAPS_TEXTURE;
    header.dwWidth = textureInfo.m_width;
    header.dwHeight = textureInfo.m_height;
    // - Pixelformat
    header.ddpfPixelFormat.dwSize = sizeof(crnlib::DDPIXELFORMAT);
    header.ddpfPixelFormat.dwFlags = crnlib::DDPF_FOURCC;
    crn_format fundamentalFormat = crnd::crnd_get_fundamental_dxt_format(textureInfo.m_format);
    header.ddpfPixelFormat.dwFourCC = crnd::crnd_crn_format_to_fourcc(fundamentalFormat);
    if (fundamentalFormat != textureInfo.m_format)
        header.ddpfPixelFormat.dwRGBBitCount = crnd::crnd_crn_format_to_fourcc(textureInfo.m_format);
    // - Mipmaps
    header.dwMipMapCount = (textureInfo.m_levels > 1) ? textureInfo.m_levels : 0;
    if (textureInfo.m_levels > 1)
        header.ddsCaps.dwCaps |= (crnlib::DDSCAPS_COMPLEX | crnlib::DDSCAPS_MIPMAP);
    // - Cubemap with 6 faces
    if (textureInfo.m_faces == 6)
    {
        header.ddsCaps.dwCaps2 = crnlib::DDSCAPS2_CUBEMAP |
            crnlib::DDSCAPS2_CUBEMAP_POSITIVEX | crnlib::DDSCAPS2_CUBEMAP_NEGATIVEX | crnlib::DDSCAPS2_CUBEMAP_POSITIVEY |
            crnlib::DDSCAPS2_CUBEMAP_NEGATIVEY | crnlib::DDSCAPS2_CUBEMAP_POSITIVEZ | crnlib::DDSCAPS2_CUBEMAP_NEGATIVEZ;
    }

    // Set pitch/linear size field (some DDS readers require this field to be non-zero).
    int bits_per_pixel = crnd::crnd_get_crn_format_bits_per_texel(textureInfo.m_format);
    header.lPitch = (((header.dwWidth + 3) & ~3) * ((header.dwHeight + 3) & ~3) * bits_per_pixel) >> 3;
    header.dwFlags |= crnlib::DDSD_LINEARSIZE;

    // Prepare output data
    uint totalSize = sizeof(crnlib::cDDSFileSignature) + header.dwSize;
    uint writePos = 0;
    ddsData.Resize(totalSize);

    // Write signature. Note: Not endian safe.
    memcpy(&ddsData[0] + writePos, &crnlib::cDDSFileSignature, sizeof(crnlib::cDDSFileSignature));
    writePos += sizeof(crnlib::cDDSFileSignature);

    // Write header
    memcpy(&ddsData[0] + writePos, &header, header.dwSize);
    writePos += header.dwSize;

    // Now transcode all face and mipmap levels into memory, one mip level at a time.
    for (crn_uint32 iLevel = 0; iLevel < textureInfo.m_levels; iLevel++)
    {
        // Compute the face's width, height, number of DXT blocks per row/col, etc.
        const crn_uint32 width = std::max(1U, textureInfo.m_width >> iLevel);
        const crn_uint32 height = std::max(1U, textureInfo.m_height >> iLevel);
        const crn_uint32 blocksX = std::max(1U, (width + 3) >> 2);
        const crn_uint32 blocksY = std::max(1U, (height + 3) >> 2);
        const crn_uint32 rowPitch = blocksX * crnd::crnd_get_bytes_per_dxt_block(textureInfo.m_format);
        const crn_uint32 faceSize = rowPitch * blocksY;

        totalSize += faceSize;
        if (ddsData.Size() < totalSize)
            ddsData.Resize(totalSize);

        // Now transcode the level to raw DXTn
        void *dest = (void*)(&ddsData[0] + writePos);
        if (!crnd::crnd_unpack_level(crnContext, &dest, faceSize, rowPitch, iLevel))
        {
            ddsData.Clear();
            break;
        }
        writePos += faceSize;
    }
    crnd::crnd_unpack_end(crnContext);

    if (ddsData.Empty())
    {
        LogError("CRN uncompression failed!");
        return false;
    }
    return true;
}

void TextureAsset::DoUnload()
{
    texture.Reset();
}

bool TextureAsset::IsLoaded() const
{
    return texture != nullptr;
}

Urho3D::Texture2D* TextureAsset::UrhoTexture() const
{
    return texture;
}

size_t TextureAsset::Height() const
{
    if (!IsLoaded())
        return 0;

    return texture->GetHeight();
}

size_t TextureAsset::Width() const
{
    if (!IsLoaded())
        return 0;

    return texture->GetWidth();
}

}
