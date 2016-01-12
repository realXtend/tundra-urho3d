// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "ScriptAsset.h"
#include "AssetAPI.h"
#include "LoggingFunctions.h"

#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/FileSystem.h>
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
    StringVector addedRefs;

    // In headless mode we dont want to mark certain asset types as
    // dependencies for the script, as they will fail Load() anyways
    StringVector ignoredAssetTypes;
    if (assetAPI->IsHeadless())
    {
        ignoredAssetTypes.Push("Texture");
        ignoredAssetTypes.Push("OgreMaterial");
        ignoredAssetTypes.Push("OgreParticle");
        ignoredAssetTypes.Push("Audio");
    }

    // Script asset dependencies are expressed in code comments using lines like "// !ref: http://myserver.com/myasset.png".
    // The asset type can be specified using a comma: "// !ref: http://myserver.com/avatarasset.xml, Avatar".
    StringVector lines = scriptContent.Split('\n');
    for (StringVector::ConstIterator it = lines.Begin(); it != lines.End(); ++it)
    {
        String line = it->Trimmed();
        uint pos = line.Find("!ref:");
        if (pos != String::NPOS)
        {
            String name = line.Substring(pos + 5);
            String type;
            pos = name.Find(',');
            if (pos != String::NPOS)
            {
                name = name.Substring(pos + 1).Trimmed();
                name = name.Substring(0, pos);
            }
            
            AssetReference ref;
            ref.ref = assetAPI->ResolveAssetRef(Name(), name.Trimmed());
            if (ignoredAssetTypes.Contains(assetAPI->ResourceTypeForAssetRef(ref.ref)))
                continue;
            for (uint i = 0; i < addedRefs.Size(); ++i)
            {
                if (addedRefs[i].Compare(ref.ref, false) == 0)
                    continue;
            }
            references.Push(ref);
            addedRefs.Push(ref.ref);
        }
        else
        {
            pos = line.Find("engine.IncludeFile");
            if (pos != String::NPOS)
            {
                uint startPos = line.Find('(', pos+18);
                uint endPos = line.Find(')', pos+18);
                if (startPos != String::NPOS && endPos != String::NPOS && endPos > startPos)
                {
                    String fileName = line.Substring(startPos + 1, endPos-startPos-1).Trimmed();
                    // First check if this is a relative ref directly to jsmodules
                    // We don't want to add these to the references list as it will request them via asset api
                    // with a relative path and it will always fail (as we dont have working file:// schema etc.)
                    // The IncludeFile function will take care of relative refs when the script is ran.
                    if (!IsAbsolutePath(fileName) && (fileName.StartsWith("jsmodules") ||
                        fileName.StartsWith("/jsmodules") || fileName.StartsWith("./jsmodules")))
                        continue;

                    // Ask AssetAPI to resolve the ref
                    AssetReference ref;
                    ref.ref = assetAPI->ResolveAssetRef(Name(), fileName);
                    for (uint i = 0; i < addedRefs.Size(); ++i)
                    {
                        if (addedRefs[i].Compare(ref.ref, false) == 0)
                            continue;
                    }
                    references.Push(ref);
                    addedRefs.Push(ref.ref);
                }
            }
        }
    }
}

bool ScriptAsset::IsLoaded() const
{
    return !scriptContent.Empty();
}

}
