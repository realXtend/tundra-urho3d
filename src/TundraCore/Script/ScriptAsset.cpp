// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "ScriptAsset.h"
#include "AssetAPI.h"
#include "LoggingFunctions.h"

#include <MemoryBuffer.h>
#include <cstring>

namespace Tundra
{

ScriptAsset::~ScriptAsset()
{
    Unload();
}

void ScriptAsset::DoUnload()
{
    scriptContent = "";
    references.Clear();
}

bool ScriptAsset::DeserializeFromData(const u8 *data, uint numBytes, bool /*allowAsynchronous*/)
{
    Urho3D::MemoryBuffer buffer(data, numBytes);
    scriptContent = buffer.ReadString();

    ParseReferences();
    assetAPI->AssetLoadCompleted(Name());
    return true;
}

bool ScriptAsset::SerializeTo(Vector<u8> &dst, const String &/*serializationParameters*/) const
{
    dst.Resize(scriptContent.Length());
    if (scriptContent.Length())
        memcpy(&dst[0], &scriptContent[0], scriptContent.Length());
    
    return true;
}

void ScriptAsset::ParseReferences()
{
    references.Clear();
    /// \todo Implement
}

bool ScriptAsset::IsLoaded() const
{
    return !scriptContent.Empty();
}

}
