// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "AssetFwd.h"
#include "IAsset.h"

namespace Tundra
{

/// Contains data of a script file loaded to the system.
class TUNDRACORE_API ScriptAsset : public IAsset
{
    URHO3D_OBJECT(ScriptAsset, IAsset);

public:
    ScriptAsset(AssetAPI *owner, const String &type_, const String &name_) :
        IAsset(owner, type_, name_)
    {
    }

    ~ScriptAsset();
    
    /// Load script asset from memory
    bool DeserializeFromData(const u8 *data, uint numBytes, bool allowAsynchronous) override;

    /// Load script asset into memory
    virtual bool SerializeTo(Vector<u8> &dst, const String &serializationParameters) const;

    /// Return found asset references inside the script
    virtual Vector<AssetReference> FindReferences() const { return references; }

    /// The asset references specified by this asset are specified in the above scriptContent data,
    /// but we cache them here on loading to quicken the access if they're needed several times.
    /// This also performs validation-on-load.
    Vector<AssetReference> references;

    String scriptContent;

    bool IsLoaded() const;

private:
    /// Unload script asset
    virtual void DoUnload();

    /// Parse internal references from script
    void ParseReferences();
};

}

