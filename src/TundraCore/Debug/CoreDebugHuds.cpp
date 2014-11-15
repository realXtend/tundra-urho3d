/// Class copied from the Urho3D project. Modified for Tundra use.

#include "StableHeaders.h"
#include "CoreDebugHuds.h"
#include "CoreStringUtils.h"

#include "Framework.h"
#include "AssetAPI.h"
#include "IAsset.h"

#include <StringUtils.h>
#include <Profiler.h>
#include <Text.h>
#include <Container/Sort.h>

/// @cond PRIVATE

using namespace Urho3D;

namespace Tundra
{

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

    // IAssetBundle
    uint subassets;

    // Binary
    HashMap<String, uint> numExts;

    AssetTypeInfo() : loaded(0), unloaded(0), original(0),
        cached(0), programmatic(0), subassets(0), bundle(0) {}
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

        AssetTypeInfo &info = typeInfos[type];
        if (iter->second->IsLoaded())
        {
            info.loaded++;
            totals.loaded++;
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
                info.programmatic++;
                totals.programmatic++;
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
    str.AppendWithFormat("%s %s %s %s %s %s\n\n", PadString("", 13).CString(),
        PadString("Loaded", 7).CString(), PadString("Unloaded", 9).CString(),
        PadString("Cached", 7).CString(), PadString("Original", 8).CString(),
        PadString("Bundle", 6).CString()
    );
    str.AppendWithFormat("%s %s %s %s %s %s\n", PadString("Total", 13).CString(),
        PadNonZero(totals.loaded, 7).CString(), PadNonZero(totals.unloaded, 9).CString(),
        PadNonZero(totals.cached, 7).CString(), PadNonZero(totals.original, 8).CString(),
        PadNonZero(totals.bundle, 6).CString()
    );
    foreach(const String &type, types)
    {
        AssetTypeInfo &info = typeInfos[type];
        str.AppendWithFormat("%s %s %s %s %s %s", PadString(type, 13).CString(),
            PadNonZero(info.loaded, 7).CString(), PadNonZero(info.unloaded, 9).CString(),
            PadNonZero(info.cached, 7).CString(), PadNonZero(info.original, 8).CString(),
            PadNonZero(info.bundle, 6).CString()
        );
        if (info.subassets > 0)
            str.AppendWithFormat(" %u sub assets", info.subassets);
        if (!info.numExts.Empty())
        {
            String exts;
            for(auto iter = info.numExts.Begin(); iter != info.numExts.End(); ++iter)
                exts.AppendWithFormat("%d %s, ", iter->second_, iter->first_.CString());
            str.AppendWithFormat(" %s", exts.Substring(0, exts.Length()-2).CString());
        }
        str.Append("\n");
    }

    // todo Transfers

    assetText->SetText(str);
}

}

/// @endcond
