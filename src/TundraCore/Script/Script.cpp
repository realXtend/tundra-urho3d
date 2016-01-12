// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "Script.h"
#include "IScriptInstance.h"
#include "ScriptAsset.h"
#include "AssetAPI.h"
#include "Framework.h"
#include "IAttribute.h"
#include "AttributeMetadata.h"
#include "IAssetTransfer.h"
#include "Entity.h"
#include "AssetRefListener.h"
#include "LoggingFunctions.h"

namespace Tundra
{

Script::~Script()
{
    // If we have a classname, empty it to trigger deletion of the script object
    if (!className.Get().Trimmed().Empty())
        className.Set("", AttributeChange::LocalOnly);
    
    if (scriptInstance_)
        scriptInstance_->Unload();
    scriptInstance_.Reset();
}

void Script::SetScriptInstance(IScriptInstance *instance)
{
    // If we already have a script instance, unload and delete it.
    if (scriptInstance_)
    {
        scriptInstance_->Unload();
        scriptInstance_.Reset();
    }
    scriptInstance_ = instance;
}

void Script::SetScriptApplication(Script* app)
{
    if (app)
        scriptApplication_ = app;
    else
        scriptApplication_.Reset();
}

Script* Script::ScriptApplication() const
{
    return dynamic_cast<Script*>(scriptApplication_.Get());
}

bool Script::ShouldRun() const
{
    int mode = runMode.Get();
    if (mode == RunOnBoth)
        return true;
    if (mode == RunOnClient && isClient_)
        return true;
    if (mode == RunOnServer && isServer_)
        return true;
    return false;
}

void Script::SetIsClientIsServer(bool isClient, bool isServer)
{
    isClient_ = isClient;
    isServer_ = isServer;
}

void Script::Run(const String &name)
{
    if (!ShouldRun())
    {
        LogWarning("Run explicitly called, but RunMode does not match");
        return;
    }
    
    // This function (Script::Run) is invoked on the Entity Action RunScript(scriptName). To
    // allow the user to differentiate between multiple instances of Script in the same entity, the first
    // parameter of RunScript allows the user to specify which Script to run. So, first check
    // if this Run message is meant for us.
    if (!name.Empty() && name != Name())
        return; // Not our RunScript invocation - ignore it.

    if (!scriptInstance_)
    {
        LogError("Run: No script instance set");
        return;
    }

    scriptInstance_->Run();
}

/// Invoked on the Entity Action UnloadScript(scriptName).
void Script::Unload(const String &name)
{
    if (!name.Empty() && name != Name())
        return; // Not our RunScript invocation - ignore it.

    if (!scriptInstance_)
    {
        LogError("Unload: Cannot perform, no script instance set");
        return;
    }

    scriptInstance_->Unload();
}

Script::Script(Urho3D::Context* context, Scene* scene):
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(scriptRef, "Script ref", AssetReferenceList("Script")),
    INIT_ATTRIBUTE_VALUE(runOnLoad, "Run on load", false),
    INIT_ATTRIBUTE_VALUE(runMode, "Run mode", RunOnBoth),
    INIT_ATTRIBUTE(applicationName, "Script application name"),
    INIT_ATTRIBUTE(className, "Script class name"),
    scriptInstance_(0),
    isClient_(false),
    isServer_(false)
{
    static AttributeMetadata scriptRefData;
    static AttributeMetadata runModeData;
    static bool metadataInitialized = false;
    if (!metadataInitialized)
    {
        AttributeMetadata::ButtonInfoList scriptRefButtons;
        scriptRefData.buttons = scriptRefButtons;
        scriptRefData.elementType = "AssetReference";
        runModeData.enums[RunOnBoth] = "Both";
        runModeData.enums[RunOnClient] = "Client";
        runModeData.enums[RunOnServer] = "Server";
        metadataInitialized = true;
    }
    scriptRef.SetMetadata(&scriptRefData);
    runMode.SetMetadata(&runModeData);

    AttributeChanged.Connect(this, &Script::HandleAttributeChanged);
    ParentEntitySet.Connect(this, &Script::RegisterActions);
}

void Script::HandleAttributeChanged(IAttribute* attribute, AttributeChange::Type /*change*/)
{
    if (!framework)
        return;

    if (attribute == &scriptRef)
    {
        // Do not even fetch the assets if we should not run
        if (!ShouldRun())
            return;
        
        AssetReferenceList scripts = scriptRef.Get();

        // Purge empty script refs
        scripts.RemoveEmpty();

        if (scripts.Size())
            scriptAssets->HandleChange(scripts);
        else // If there are no non-empty script refs, we unload the script instance.
            SetScriptInstance(0);
    }
    else if (attribute == &applicationName)
    {
        ApplicationNameChanged.Emit(applicationName.Get());
    }
    else if (attribute == &className)
    {
        ClassNameChanged.Emit(className.Get());
    }
    else if (attribute == &runMode)
    {
        // If we had not loaded script assets previously because of runmode not allowing, load them now
        if (ShouldRun())
        {
            if (scriptAssets->Assets().Empty())
                HandleAttributeChanged(&scriptRef, AttributeChange::Default);
        }
        else // If runmode is changed and shouldn't run, unload script assets and script instance 
        {
            scriptAssets->HandleChange(AssetReferenceList());
            SetScriptInstance(0);
        }
    }
    else if (attribute == &runOnLoad)
    {
        // If RunOnLoad changes, is true, and we don't have a script instance yet, emit ScriptAssetsChanged to start up the script.
        if (runOnLoad.Get() && scriptAssets->Assets().Size() && (!scriptInstance_ || !scriptInstance_->IsEvaluated()))
            OnScriptAssetLoaded(0, AssetPtr()); // The asset ptr can be null, it is not used.
    }
}

void Script::OnScriptAssetLoaded(uint /*index*/, AssetPtr /*asset_*/)
{
    // If all asset ref listeners have valid, loaded script assets, it's time to fire up the script engine
    Vector<ScriptAssetPtr> loadedScriptAssets;
    Vector<AssetPtr> allAssets = scriptAssets->Assets();

    for (uint i = 0; i < allAssets.Size(); ++i)
    {
        if (allAssets[i])
        {
            ScriptAssetPtr asset = Urho3D::DynamicCast<ScriptAsset>(allAssets[i]);
            if (!asset)
            {
                LogError("Script::ScriptAssetLoaded: Loaded asset of type other than ScriptAsset!");
                continue;
            }
            if (asset->IsLoaded())
                loadedScriptAssets.Push(asset);
        }
    }
    
    if (loadedScriptAssets.Size() == allAssets.Size())
        ScriptAssetsChanged.Emit(this, loadedScriptAssets);
}

void Script::RegisterActions()
{
    Entity *entity = ParentEntity();
    assert(entity);
    if (entity)
    {
        entity->Action("RunScript")->Triggered.Connect(this, &Script::RunTriggered);
        entity->Action("UnloadScript")->Triggered.Connect(this, &Script::UnloadTriggered);
    }
    scriptAssets = new AssetRefListListener(framework->Asset());
    scriptAssets->Loaded.Connect(this, &Script::OnScriptAssetLoaded);
}

void Script::RunTriggered(const StringVector& params)
{
    if (params.Size())
        Run(params[0]);
}

void Script::UnloadTriggered(const StringVector& params)
{
    if (params.Size())
        Unload(params[0]);
}

}
