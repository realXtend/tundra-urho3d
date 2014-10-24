// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "AssetAPI.h"
#include "IAsset.h"

namespace Tundra
{

/// A generic data container for assets of unknown type.
class TUNDRACORE_API BinaryAsset : public IAsset
{
    OBJECT(BinaryAsset);

public:
    BinaryAsset(AssetAPI *owner, const String &type_, const String &name_) :
        IAsset(owner, type_, name_)
    {
    }

    ~BinaryAsset()
    {
        Unload();
    }

    virtual void DoUnload()
    {
        data.Clear();
    }

    virtual bool DeserializeFromData(const u8 *data_, size_t numBytes, bool /*allowAsynchronous*/)
    {
        data.Resize(numBytes);
        if (numBytes && data_)
            memcpy(&data[0], data_, numBytes);
        assetAPI->AssetLoadCompleted(Name());
        return true;
    }

    virtual bool SerializeTo(Vector<u8> &dst, const String &/*serializationParameters*/) const
    {
        dst = data;
        return true;
    }

    virtual Vector<AssetReference> FindReferences() const
    {
        return Vector<AssetReference>();
    }

    bool IsLoaded() const
    {
        return data.Size() > 0;
    }

    Vector<u8> data;
};

}
