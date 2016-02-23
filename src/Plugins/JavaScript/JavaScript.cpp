// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "JavaScript.h"
#include "Framework.h"
#include "LoggingFunctions.h"
#include "AssetAPI.h"
#include "SceneAPI.h"
#include "GenericAssetFactory.h"
#include "Script.h"
#include "ScriptAsset.h"
#include "JavaScriptInstance.h"
#include "MathBindings/MathBindings.h"
#include "CoreBindings/CoreBindings.h"

#include <Urho3D/Core/Profiler.h>

using namespace JSBindings;

namespace Tundra
{

JavaScript::JavaScript(Framework* owner) :
    IModule("JavaScript", owner)
{
}

JavaScript::~JavaScript()
{
}

void JavaScript::Load()
{
    //SceneAPI* scene = framework->Scene();
    framework->Asset()->RegisterAssetTypeFactory(AssetTypeFactoryPtr(new GenericAssetFactory<ScriptAsset>("ScriptAsset", ".js")));
}

void JavaScript::Initialize()
{
    framework->Scene()->SceneCreated.Connect(this, &JavaScript::OnSceneCreated);
}

void JavaScript::Uninitialize()
{
}

void JavaScript::OnSceneCreated(Scene *scene, AttributeChange::Type /*change*/)
{
    scene->ComponentAdded.Connect(this, &JavaScript::OnComponentAdded);
    scene->ComponentRemoved.Connect(this, &JavaScript::OnComponentRemoved);
}

void JavaScript::OnComponentAdded(Entity* entity, IComponent* comp, AttributeChange::Type /*change*/)
{
    if (comp->TypeName() == Script::TypeNameStatic())
    {
        Script* script = static_cast<Script*>(comp);
        script->ScriptAssetsChanged.Connect(this, &JavaScript::OnScriptAssetsChanged);
        /// \todo Handle application / class mechanic

        // Set the script component's isClient & isServer flags to determine run mode
        /// \hack Can not depend on TundraLogicModule so that other modules can depend on us, so rather check the server commandline flag
        bool isServer = framework->HasCommandLineParameter("--server");
        bool isClient = !isServer;
        /// \hack Currently we use the scene's name to recognize the single-user scene: it will have both client and server flags set
        if (!isServer)
        {
            Scene* scene = entity->ParentScene();
            if (scene)
                isServer = scene->Name().Compare("TundraServer", false) == 0;
        }
        script->SetIsClientIsServer(isClient, isServer);
    }
}

void JavaScript::OnComponentRemoved(Entity* /*entity*/, IComponent* comp, AttributeChange::Type /*change*/)
{
    if (comp->TypeName() == Script::TypeNameStatic())
    {
        Script* script = static_cast<Script*>(comp);
        script->ScriptAssetsChanged.Disconnect(this, &JavaScript::OnScriptAssetsChanged);
    }
}

void JavaScript::OnScriptAssetsChanged(Script* scriptComp, const Vector<ScriptAssetPtr>& newScripts)
{
    URHO3D_PROFILE(JavaScript_ScriptAssetsChanged);

    if (newScripts.Empty())
    {
        LogError("Script asset vector was empty");
        return;
    }

    // First clean up any previous running script from, if any exists.
    scriptComp->SetScriptInstance(0);

    if (newScripts[0]->Name().EndsWith(".js")) // We're positively using QtScript.
    {
        JavaScriptInstance *jsInstance = new JavaScriptInstance(newScripts, this, scriptComp);

        scriptComp->SetScriptInstance(jsInstance);

        // If this component is a script application, connect to the evaluate / unload signals so that we can create or delete script objects as needed
        /// \todo Implement, skipped for now
        /*
        if (!sender->applicationName.Get().trimmed().isEmpty())
        {
            jsInstance->ScriptEvaluated.Connect(this, &JavaScript::OnScriptEvaluated);
            jsInstance->ScriptUnloading.Connect(this, &JavaScript::OnScriptUnloading);
        }
        */

        bool isApplication = !scriptComp->applicationName.Get().Trimmed().Empty();
        if (scriptComp->runOnLoad.Get() && scriptComp->ShouldRun())
        {
            if (isApplication && framework->HasCommandLineParameter("--disablerunonload"))
                return;
            jsInstance->Run();
        }
    }
}

void JavaScript::PrepareScriptInstance(JavaScriptInstance* instance, Script* scriptComp)
{
    URHO3D_PROFILE(PrepareScriptInstance);

    duk_context* ctx = instance->Context();

    {
        URHO3D_PROFILE(ExposeMathClasses);
        ExposeMathClasses(ctx);
    }
    {
        URHO3D_PROFILE(ExposeCoreClasses);
        ExposeCoreClasses(ctx);
    }

    /// \todo Register engine and other services
    instance->RegisterService("framework", Fw());

    if (scriptComp)
    {
        instance->RegisterService("me", scriptComp->ParentEntity());
        instance->RegisterService("scene", scriptComp->ParentScene());
    }
}

}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::JavaScript(fw));
}

}
