/// Class copied from the Urho3D project. Modified for Tundra use.

#include "StableHeaders.h"
#include "CoreDebugHuds.h"
#include "CoreStringUtils.h"

#include "Framework.h"
#include "AssetAPI.h"
#include "IAsset.h"
#include "Scene.h"
#include "IRenderer.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/Container/Sort.h>

/// @cond PRIVATE

using namespace Urho3D;

namespace Tundra
{

String PadDouble(double value, uint pad, int digits = 4)
{
    String temp = Urho3D::ToString("%f", value);
    if (digits > -1)
    {
        uint i = temp.Find('.');
        if (i != String::NPOS && i + 1 + digits < temp.Length())
            temp = temp.Substring(0, i + 1 + digits);
    }
    String v = PadString(temp, pad);
    if (v.Length() >= pad) // cut digits
        v = v.Substring(0, pad);
    return v; 
}

void InsertAlpha(StringVector &container, const String &value)
{
    if (container.Find(value) != container.End())
        return;
    for(auto iter = container.Begin(); iter != container.End(); ++iter)
    {
        if (value.Compare((*iter), false) < 0)
        {
            container.Insert(iter, value);
            return;
        }
    }
    container.Push(value);
}

// Urho Profiler

ProfilerHudPanel::ProfilerHudPanel(Framework *framework) :
    DebugHudPanel(framework),
    profilerMaxDepth(M_MAX_UNSIGNED),
    profilerInterval(1000)
{
}

SharedPtr<Urho3D::UIElement> ProfilerHudPanel::CreateImpl()
{
    return SharedPtr<Urho3D::UIElement>(new Text(framework_->GetContext()));
}

void ProfilerHudPanel::UpdatePanel(float /*frametime*/, const SharedPtr<Urho3D::UIElement> &widget)
{
    if (profilerTimer_.GetMSec(false) < profilerInterval)
        return;

    Profiler* profiler = framework_->GetSubsystem<Profiler>();
    Text *profilerText = dynamic_cast<Text*>(widget.Get());
    if (!profiler || !profilerText)
        return;

    profilerTimer_.Reset();

    String profilerOutput = profiler->GetData(false, false, profilerMaxDepth);
    profilerText->SetText(profilerOutput);

    profiler->BeginInterval();
}

// Scene

SceneHudPanel::SceneHudPanel(Framework *framework) :
    DebugHudPanel(framework),
    limiter_(1.f)
{
}

SharedPtr<Urho3D::UIElement> SceneHudPanel::CreateImpl()
{
    return SharedPtr<Urho3D::UIElement>(new Text(framework_->GetContext()));
}

struct SceneInfo
{
    uint ents;
    uint entsRoot;
    uint entsParented;
    uint entsLocal;
    uint entsReplicated;
    uint entsTemporary;
    uint entsEmpty;

    StringVector groups;
    HashMap<String, uint> entGroups;

    uint comps;
    uint compsLocal;
    uint compsReplicated;
    uint compsTemporary;

    StringVector types;
    HashMap<String, uint> compTypes;

    int pad;

    SceneInfo() : ents(0), entsRoot(0), entsParented(0), entsLocal(0), entsReplicated(0), entsTemporary(0), entsEmpty(0),
        comps(0), compsLocal(0), compsReplicated(0), compsTemporary(0), pad(14) {}
};

void SceneHudPanel::UpdatePanel(float frametime, const SharedPtr<Urho3D::UIElement> &widget)
{
    if (!limiter_.ShouldUpdate(frametime))
        return;

    Text *sceneText = dynamic_cast<Text*>(widget.Get());
    if (!sceneText || !framework_->Renderer())
        return;

    Scene *scene = framework_->Renderer()->MainCameraScene();
    if (!scene)
        return;
    
    String str;

    SceneInfo info;
    auto entities = scene->Entities();
    info.ents = entities.Size();
    for(auto entIter = entities.Begin(); entIter != entities.End(); ++entIter)
    {
        const EntityPtr ent = entIter->second_;
        if (ent->Parent())
            info.entsParented++;
        else
            info.entsRoot++;

        if (ent->IsLocal())
            info.entsLocal++;
        if (ent->IsReplicated())
            info.entsReplicated++;
        if (ent->IsTemporary())
            info.entsTemporary++;

        String group = ent->Group().Trimmed();
        if (!group.Empty())
        {
            info.entGroups[group]++;
            InsertAlpha(info.groups, group);

            if ((int)group.Length() > info.pad)
                info.pad = group.Length() + 2;
        }

        auto components = ent->Components();
        info.comps += components.Size();
        for(auto compIter = components.Begin(); compIter != components.End(); ++compIter)
        {
            auto comp = compIter->second_;
            String type = comp->TypeName();
            info.compTypes[type]++;
            InsertAlpha(info.types, type);

            if (comp->IsLocal())
                info.compsLocal++;
            if (comp->IsReplicated())
                info.compsReplicated++;
            if (comp->IsTemporary())
                info.compsTemporary++;

            if ((int)type.Length() > info.pad)
                info.pad = type.Length() + 2;
        }
        if (components.Empty())
            info.entsEmpty++;
    }

    str.AppendWithFormat("%s   %u\n", PadString("Entities", info.pad).CString(), info.ents);
    str.AppendWithFormat("  %s %u\n", PadString("Root", info.pad).CString(), info.entsRoot);
    str.AppendWithFormat("  %s %u\n", PadString("Parented", info.pad).CString(), info.entsParented);
    str.AppendWithFormat("  %s %u\n", PadString("Empty", info.pad).CString(), info.entsEmpty);
    str.AppendWithFormat("  %s %u\n", PadString("Replicated", info.pad).CString(), info.entsReplicated);
    str.AppendWithFormat("  %s %u\n", PadString("Local", info.pad).CString(), info.entsLocal);
    str.AppendWithFormat("  %s %u\n\n", PadString("Temporary", info.pad).CString(), info.entsTemporary);

    if (!info.groups.Empty())
    {
        str.AppendWithFormat("%s   %u\n", PadString("Entity Groups", info.pad).CString(), info.groups.Size());
        foreach(auto &group, info.groups)
        {
            uint num = info.entGroups[group];
            str.AppendWithFormat("  %s %u\n", PadString(group, info.pad).CString(), num);
        }
        str.Append("\n");
    }

    str.AppendWithFormat("%s   %u\n", PadString("Components", info.pad).CString(), info.comps);
    str.AppendWithFormat("  %s %u\n", PadString("Replicated", info.pad).CString(), info.compsReplicated);
    str.AppendWithFormat("  %s %u\n", PadString("Local", info.pad).CString(), info.compsLocal);
    str.AppendWithFormat("  %s %u\n\n", PadString("Temporary", info.pad).CString(), info.compsTemporary);

    foreach(auto &type, info.types)
    {
        uint num = info.compTypes[type];
        str.AppendWithFormat("  %s %u\n", PadString(type, info.pad).CString(), num);
    }
    str.Append("\n");

    sceneText->SetText(str);
}

// AssetAPI

AssetHudPanel::AssetHudPanel(Framework *framework) :
    DebugHudPanel(framework),
    limiter_(1/10.f)
{
}

SharedPtr<Urho3D::UIElement> AssetHudPanel::CreateImpl()
{
    return SharedPtr<Urho3D::UIElement>(new Text(framework_->GetContext()));
}

String Extension(const String &value)
{
    uint index = value.FindLast(".");
    if (index != String::NPOS)
        return value.Substring(index, value.Length()-index);
    return "no-ext";
}

String PadNonZero(uint value, int pad)
{
    return PadString((value > 0 ? String(value) : "-"), pad);
}

struct AssetTypeInfo
{
    // IAsset
    uint loaded;
    uint unloaded;
    uint original;
    uint cached;
    uint programmatic;
    uint bundle;
    float spentLoading;
    float spentDiskRead;

    // IAssetBundle
    uint subassets;

    // Binary
    HashMap<String, uint> numExts;

    AssetTypeInfo() : loaded(0), unloaded(0), original(0),
        cached(0), programmatic(0), subassets(0), bundle(0),
        spentLoading(0.f), spentDiskRead(0.f) {}
};

void AssetHudPanel::UpdatePanel(float frametime, const SharedPtr<Urho3D::UIElement> &widget)
{
    if (!limiter_.ShouldUpdate(frametime))
        return;

    Text *assetText = dynamic_cast<Text*>(widget.Get());
    if (!assetText)
        return;

    String str;

    // Assets
    AssetTypeInfo totals;
    StringVector types;
    HashMap<String, AssetTypeInfo> typeInfos;

    auto assets = framework_->Asset()->Assets();
    for(auto iter = assets.begin(); iter != assets.end(); ++iter)
    {
        String type = iter->second->Type();
        InsertAlpha(types, type);

        float spentLoading = iter->second->profile.SpentSeconds(AssetProfile::Load);
        float spentDiskRead = iter->second->profile.SpentSeconds(AssetProfile::DiskRead);

        AssetTypeInfo &info = typeInfos[type];
        if (iter->second->IsLoaded())
        {
            info.loaded++;
            totals.loaded++;
            info.spentLoading += spentLoading;
            totals.spentLoading += spentLoading;
            info.spentDiskRead += spentDiskRead;
            totals.spentDiskRead += spentDiskRead;
        }
        else
        {
            info.unloaded++;
            totals.unloaded++;
        }
        switch(iter->second->DiskSourceType())
        {
            case IAsset::Original:
            {
                info.original++;
                totals.original++;
                break;
            }
            case IAsset::Cached:
            {
                info.cached++;
                totals.cached++;
                break;
            }
            case IAsset::Programmatic:
            {
                info.programmatic++;
                totals.programmatic++;
                break;
            }
            case IAsset::Bundle:
            {
                info.bundle++;
                break;
            }
        }
        if (type == "Binary")
            info.numExts[Extension(iter->first.ToLower())]++;
    }
    auto assetBundles = framework_->Asset()->AssetBundles();
    for(auto iter = assetBundles.begin(); iter != assetBundles.end(); ++iter)
    {
        String type = iter->second->Type();
        InsertAlpha(types, type);

        AssetTypeInfo &info = typeInfos[type];
        if (iter->second->IsLoaded())
        {
            info.loaded++;
            totals.loaded;
        }
        else
        {
            info.unloaded++;
            totals.unloaded;
        }
        int subAssets = iter->second->SubAssetCount();
        if (subAssets > 0)
            info.subassets += subAssets;
    }
    str.AppendWithFormat("%s %s %s %s %s %s %s %s\n\n", PadString("", 13).CString(),
        PadString("Loaded", 7).CString(), PadString("Unloaded", 9).CString(),
        PadString("Cached", 7).CString(), PadString("Original", 8).CString(),
        PadString("Bundle", 6).CString(), PadString("Load sec", 8).CString(),
        PadString("Disk Read sec", 13).CString()
    );
    str.AppendWithFormat("%s %s %s %s %s %s %s %s\n", PadString("Total", 13).CString(),
        PadNonZero(totals.loaded, 7).CString(), PadNonZero(totals.unloaded, 9).CString(),
        PadNonZero(totals.cached, 7).CString(), PadNonZero(totals.original, 8).CString(),
        PadNonZero(totals.bundle, 6).CString(), PadDouble(totals.spentLoading, 8, 4).CString(),
        PadDouble(totals.spentDiskRead, 13, 4).CString()
    );
    String binaryExts;
    foreach(const String &type, types)
    {
        AssetTypeInfo &info = typeInfos[type];
        str.AppendWithFormat("%s %s %s %s %s %s %s %s", PadString(type, 13).CString(),
            PadNonZero(info.loaded, 7).CString(), PadNonZero(info.unloaded, 9).CString(),
            PadNonZero(info.cached, 7).CString(), PadNonZero(info.original, 8).CString(),
            PadNonZero(info.bundle, 6).CString(), PadDouble(info.spentLoading, 8, 4).CString(),
            PadDouble(info.spentDiskRead, 13, 4).CString()
        );
        if (info.subassets > 0)
            str.AppendWithFormat(" %u sub assets", info.subassets);
        if (!info.numExts.Empty())
        {
            for(auto iter = info.numExts.Begin(); iter != info.numExts.End(); ++iter)
                binaryExts.AppendWithFormat("%d %s, ", iter->second_, iter->first_.CString());
        }
        str.Append("\n");
    }
    str.Append("\n");

    if (!binaryExts.Empty())
        str.AppendWithFormat("Binary Types          %s\n\n", binaryExts.Substring(0, binaryExts.Length()-2).CString());

    // todo Transfers
    auto transfers = framework_->Asset()->PendingTransfers();
    str.AppendWithFormat("Asset Transfers       %u\n\n", transfers.Size());
    uint printed = 0;
    foreach(const AssetTransferPtr &transfer, transfers)
    {
        if (printed >= 30)
            break;
        String ref = transfer->SourceUrl();
        if (ref.StartsWith("http://"))
            ref = ref.Substring(7);
        else if (ref.StartsWith("https://"))
            ref = ref.Substring(8);
        if (ref.Length() > 80)
            ref = ref.Substring(0, 27) + "..." + ref.Substring(ref.Length()-50);
        str.Append(ref + "\n");
        printed++;
    }
    str.Append("\n");

    assetText->SetText(str);
}

}

/// @endcond
