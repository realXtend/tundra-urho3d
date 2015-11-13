// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "IAsset.h"
#include "AssetAPI.h"
#include "IAssetStorage.h"
#include "IAssetProvider.h"
#include "LoggingFunctions.h"

#include <Urho3D/Core/Profiler.h>
#include <Urho3D/Container/HashSet.h>

namespace Tundra
{

IAsset::IAsset(AssetAPI *owner, const String &type_, const String &name_) :
Object(owner->GetContext()), assetAPI(owner), type(type_), name(name_), diskSourceType(Programmatic), modified(false)
{
    assert(assetAPI);
}

void IAsset::SetDiskSource(const String &diskSource_)
{
    diskSource = diskSource_.Trimmed();
    PropertyStatusChanged.Emit(this);
}

void IAsset::SetDiskSourceType(SourceType type)
{
    if (diskSourceType != type)
    {
        diskSourceType = type;
        PropertyStatusChanged.Emit(this);
    }
}

bool IAsset::LoadFromCache()
{
    // If asset did not have dependencies, this causes Loaded() to be emitted
    bool success = LoadFromFile(DiskSource());
    if (!success)
        return false;

    AssetPtr thisAsset(this);

    if (assetAPI->HasPendingDependencies(thisAsset))
        assetAPI->RequestAssetDependencies(thisAsset);

    return success;
}

void IAsset::Unload()
{
//    LogDebug("IAsset::Unload called for asset \"" + name.toStdString() + "\".");
    DoUnload();
    Unloaded.Emit(this);
}

bool IAsset::IsEmpty() const
{
    return !IsLoaded() && diskSource.Empty();
}

bool IAsset::IsTrusted() const
{
    AssetStoragePtr storage = AssetStorage();
    if (!storage)
    {
        AssetAPI::AssetRefType type = AssetAPI::ParseAssetRef(Name());
        return type == AssetAPI::AssetRefLocalPath || type == AssetAPI::AssetRefLocalUrl;
    }
    return storage->Trusted();
}

void IAsset::MarkModified()
{
    if (!modified)
    {
        modified = true;
        PropertyStatusChanged.Emit(this);
    }
}

void IAsset::ClearModified()
{
    if (modified)
    {
        modified = false;
        PropertyStatusChanged.Emit(this);
    }
}

AssetPtr IAsset::Clone(String newAssetName) const
{
    assert(assetAPI);
    if (!IsLoaded())
        return AssetPtr();

    AssetPtr existing = assetAPI->FindAsset(newAssetName);
    if (existing)
    {
        LogError("Cannot Clone() asset \"" + Name() + "\" to a new asset \"" + newAssetName + "\": An asset with that name already exists!");
        return AssetPtr();
    }

    Vector<u8> data;
    bool success = SerializeTo(data);
    if (!success)
    {
        LogError("Cannot Clone() asset \"" + Name() + "\" to a new asset \"" + newAssetName + "\": Serializing the asset failed!");
        return AssetPtr();
    }
    if (data.Size() == 0)
    {
        LogError("Cannot Clone() asset \"" + Name() + "\" to a new asset \"" + newAssetName + "\": Asset serialization succeeded with zero size!");
        return AssetPtr();
    }

    AssetPtr newAsset = assetAPI->CreateNewAsset(this->Type(), newAssetName);
    if (!newAsset)
    {
        LogError("Cannot Clone() asset \"" + Name() + "\" to a new asset \"" + newAssetName + "\": AssetAPI::CreateNewAsset failed!");
        return AssetPtr();
    }

    // Do not allow asynchronous loading due the caller of this 
    // expects the asset to be usable when this function returns.
    success = newAsset->LoadFromFileInMemory(&data[0], data.Size(), false);
    if (!success)
    {
        LogError("Cannot Clone() asset \"" + Name() + "\" to a new asset \"" + newAssetName + "\": Deserializing the new asset from bytes failed!");
        assetAPI->ForgetAsset(newAsset, false);
        return AssetPtr();
    }

    return newAsset;
}

bool IAsset::LoadFromFile(String filename)
{
    URHO3D_PROFILE(IAsset_LoadFromFile);

    filename = filename.Trimmed(); ///\todo Sanitate.
    if (filename.Empty())
    {
        LogDebug("LoadFromFile failed for asset \"" + Name() + "\", given file path is empty!");
        return false;
    }

    Vector<u8> fileData;
    profile.Start(AssetProfile::DiskRead);
    bool success = LoadFileToVector(filename, fileData);
    profile.Done(AssetProfile::DiskRead);
    if (!success)
    {
        LogDebug("LoadFromFile failed for file \"" + filename + "\", could not read file!");
        return false;
    }
    if (fileData.Size() == 0)
    {
        LogDebug("LoadFromFile failed for file \"" + filename + "\", file size was 0!");
        return false;
    }

    // Invoke the actual virtual function to load the asset.
    // Do not allow asynchronous loading due the caller of this 
    // expects the asset to be usable when this function returns.
    return LoadFromFileInMemory(&fileData[0], fileData.Size(), false);
}

bool IAsset::LoadFromFileInMemory(const u8 *data, uint numBytes, bool allowAsynchronous)
{
    URHO3D_PROFILE(IAsset_LoadFromFileInMemory);
    if (!data || numBytes == 0)
    {
        LogDebug("LoadFromFileInMemory failed for asset \"" + ToString() + "\"! No data present!");
        return false;
    }

    profile.Start(AssetProfile::Load);
    return DeserializeFromData(data, numBytes, allowAsynchronous);
}

void IAsset::DependencyLoaded(AssetPtr dependee)
{
    // If we are loaded, and this was the last dependency, emit Loaded().
    // No need to have exact duplicate code here even if LoadCompleted is not the most 
    // informative name in the world for the situation.
    LoadCompleted();
}

void IAsset::LoadCompleted()
{
    URHO3D_PROFILE(IAsset_LoadCompleted);
    profile.Done(AssetProfile::Load);

    // If asset was loaded successfully, and there are no pending dependencies, emit Loaded() now.
    AssetPtr thisAsset(this);
    if (IsLoaded() && !assetAPI->HasPendingDependencies(thisAsset))
        Loaded.Emit(thisAsset);
}

Vector<AssetReference> IAsset::FindReferencesRecursive() const
{
    HashSet<AssetReference> refs;

    Vector<AssetReference> unwalkedRefs = FindReferences();
    while(unwalkedRefs.Size() > 0)
    {
        AssetReference ref = unwalkedRefs.Back();
        unwalkedRefs.Pop();
        if (refs.Find(ref) == refs.End())
        {
            refs.Insert(ref);
            AssetPtr asset = assetAPI->FindAsset(ref.ref);
            if (asset)
            {
                Vector<AssetReference> newRefs = asset->FindReferences();
                unwalkedRefs.Insert(unwalkedRefs.End(), newRefs.Begin(), newRefs.End());
            }
        }
    }

    refs.Sort();
    Vector<AssetReference> finalRefs;
    for (HashSet<AssetReference>::ConstIterator i = refs.Begin(); i != refs.End(); ++i)
        finalRefs.Push(*i);
    
    return finalRefs;
}

bool IAsset::SerializeTo(Vector<u8> &/*data*/, const String &/*serializationParameters*/) const
{
    LogError("IAsset::SerializeTo: Asset serialization not implemented for asset \"" + ToString() + "\"!");
    return false;
}

bool IAsset::SaveToFile(const String &filename, const String &serializationParameters) const
{
    Vector<u8> data;
    bool success = SerializeTo(data, serializationParameters);
    if (!success || data.Size() == 0)
    {
        LogError("IAsset::SaveToFile: SerializeTo returned no data for asset \"" + ToString() + "\"!");
        return false;
    }

    return SaveAssetFromMemoryToFile(&data[0], data.Size(), filename);
}

bool IAsset::SaveCachedCopyToFile(const String &filename)
{
    return CopyAssetFile(DiskSource(), filename);
}

void IAsset::SetAssetProvider(AssetProviderPtr provider_)
{
    provider = provider_;
}

void IAsset::SetAssetStorage(AssetStoragePtr storage_) 
{
    storage = storage_;
}

AssetStoragePtr IAsset::AssetStorage() const
{
    return storage.Lock();
}

AssetProviderPtr IAsset::AssetProvider() const
{
    return provider.Lock();
}

String IAsset::ToString() const
{ 
    return (Name().Empty() ? "(noname)" : Name()) + " (" + (Type().Empty() ? "notype" : Type()) + ")";
}

Vector<u8> IAsset::RawData(const String &serializationParameters) const
{ 
    Vector<u8> data;
    if (SerializeTo(data, serializationParameters) && data.Size() > 0)
        return data;
    else
        return Vector<u8>();
}

/// @cond PRIVATE

void AssetProfile::Start(Phase p)
{
    timers[p].Reset();
}

void AssetProfile::Done(Phase p)
{
    HashMap<int, Urho3D::Timer>::Iterator iter = timers.Find(p);
    if (iter != timers.End())
    {
        spent[p] = iter->second_.GetMSec(false) / 1000.f;
        timers.Erase(iter);
    }
}

float AssetProfile::SpentSeconds(Phase p) const
{
    HashMap<int, float>::ConstIterator iter = spent.Find(p);
    if (iter != spent.End())
        return iter->second_;
    return 0.f;
}

/// @endcond

}
