// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "JavaScript.h"
#include "Framework.h"
#include "LoggingFunctions.h"
#include "AssetAPI.h"
#include "FrameAPI.h"
#include "ConfigAPI.h"
#include "SceneAPI.h"
#include "Console/ConsoleAPI.h"
#include "Input/InputAPI.h"
#include "GenericAssetFactory.h"
#include "Script.h"
#include "ScriptAsset.h"
#include "JavaScriptInstance.h"
#include "MathBindings/MathBindings.h"
#include "CoreBindings/CoreBindings.h"
#include "JavaScriptBindings/JavaScriptBindings.h"

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
        script->ClassNameChanged.Connect(this, &JavaScript::OnScriptClassNameChanged);

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
        script->ClassNameChanged.Disconnect(this, &JavaScript::OnScriptClassNameChanged);
        RemoveScriptObject(script);
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
        if (!scriptComp->applicationName.Get().Trimmed().Empty())
        {
            jsInstance->ScriptEvaluated.Connect(this, &JavaScript::OnScriptEvaluated);
            jsInstance->ScriptUnloading.Connect(this, &JavaScript::OnScriptUnloading);
        }

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
    {
        URHO3D_PROFILE(ExposeJavaScriptClasses);
        ExposeJavaScriptClasses(ctx);
    }

    instance->RegisterService("framework", Fw());
    instance->RegisterService("frame", Fw()->Frame());
    instance->RegisterService("input", Fw()->Input());
    instance->RegisterService("console", Fw()->Console());
    instance->RegisterService("asset", Fw()->Asset());
    instance->RegisterService("config", Fw()->Config());

    instance->RegisterService("engine", instance);

    if (scriptComp)
    {
        instance->RegisterService("me", scriptComp->ParentEntity());
        instance->RegisterService("scene", scriptComp->ParentScene());
    }
}

void JavaScript::OnScriptClassNameChanged(Script* scriptComp, const String& /*newClassName*/)
{
    // Check runmode for the object
    if (!scriptComp->ShouldRun())
        return;
    
    // It is possible that we do not find the script application yet. In that case, the object will be created once the app loads.
    String appName, className;
    ParseAppAndClassName(scriptComp, appName, className);
    Script* app = FindScriptApplication(scriptComp, appName);
    if (app)
        CreateScriptObject(app, scriptComp, className);
    else
        // If we did not find the class yet, delete the existing object in any case
        RemoveScriptObject(scriptComp);
}

void JavaScript::OnScriptEvaluated(JavaScriptInstance* instance)
{
    Script* app = dynamic_cast<Script*>(instance->Owner().Get());
    if (app)
        CreateScriptObjects(app);
}

void JavaScript::OnScriptUnloading(JavaScriptInstance* instance)
{
    RemoveScriptObjects(instance);
}

Script* JavaScript::FindScriptApplication(Script* instance, const String& appName)
{
    if (!instance || appName.Empty())
        return 0;
    Entity* entity = instance->ParentEntity();
    if (!entity)
        return 0;
    Scene* scene = entity->ParentScene();
    if (!scene)
        return 0;
    // Get all script components that possibly refer to this application
    Vector<SharedPtr<Script> > scripts = scene->Components<Script>();
    for (unsigned i = 0; i < scripts.Size(); ++i)
    {
        const String& name = scripts[i]->applicationName.Get();
        if (!name.Empty() && name.Trimmed().Compare(appName, false) == 0)
            return scripts[i].Get();
    }

    return 0;
}

void JavaScript::ParseAppAndClassName(Script* instance, String& appName, String& className)
{
    if (!instance)
    {
        appName.Clear();
        className.Clear();
    }
    else
    {
        StringList strings = instance->className.Get().Split('.');
        if (strings.Size() == 2)
        {
            appName = strings[0].Trimmed();
            className = strings[1].Trimmed();
        }
        else
        {
            appName.Clear();
            className.Clear();
        }
    }
}

void JavaScript::CreateScriptObject(Script* app, Script* instance, const String& className)
{
    // Delete any existing object instance, possibly in another script application
    RemoveScriptObject(instance);
    
    // Make sure the runmode allows object creation
    if (!instance->ShouldRun())
        return;
    
    JavaScriptInstance* jsInstance = dynamic_cast<JavaScriptInstance*>(app->ScriptInstance());
    if (!jsInstance || !jsInstance->IsEvaluated())
        return;
    
    const String& appAndClassName = instance->className.Get();

    duk_context* ctx = jsInstance->Context();
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, className.CString());
    duk_remove(ctx, -2);
    if (duk_is_function(ctx, -1))
    {
        // Push containing entity and the instance script component as parameters
        PushWeakObject(ctx, instance->ParentEntity());
        PushWeakObject(ctx, instance);
        bool success = duk_pnew(ctx, 2) == 0;
        if (success && duk_is_object(ctx, -1))
        {
            duk_push_global_object(ctx);
            duk_get_prop_string(ctx, -1, "_StoreScriptObject");
            duk_remove(ctx, -2);
            // Use pointer of the object instance script component as the key
            duk_push_number(ctx, (size_t)instance);
            duk_dup(ctx, -3);
            duk_remove(ctx, -4);
            success = duk_pcall(ctx, 2) == 0;
            if (!success)
                LogError("[JavaScript] CreateScriptObject: failed to store script object for " + appAndClassName + ": " + String(duk_safe_to_string(ctx, -1)));
            duk_pop(ctx);
        }
        else
        {
            LogError("[JavaScript] CreateScriptObject: failed to run constructor for " + appAndClassName + ": " + String(duk_safe_to_string(ctx, -1)));
            duk_pop(ctx);
        }
    }
    else
    {
        LogError("[JavaScript] CreateScriptObject: constructor not found for " + appAndClassName);
        duk_pop(ctx);
    }
}

void JavaScript::RemoveScriptObject(Script* instance)
{
    Script* app = instance->ScriptApplication();
    if (!app)
        return;
    
    JavaScriptInstance* jsInstance = dynamic_cast<JavaScriptInstance*>(app->ScriptInstance());
    if (jsInstance)
    {
        duk_context* ctx = jsInstance->Context();
        duk_push_global_object(ctx);
        duk_get_prop_string(ctx, -1, "_RemoveScriptObject");
        duk_remove(ctx, -2);
        // Use pointer of the object instance script component as the key
        duk_push_number(ctx, (size_t)instance);
        bool success = duk_pcall(ctx, 1) == 0;
        if (!success) LogError("[JavaScript] RemoveScriptObject: " + String(duk_safe_to_string(ctx, -1)));
        duk_pop(ctx);
    }
}

void JavaScript::CreateScriptObjects(Script* app)
{
    // If application has an empty name, we can not create script objects out of it. Skip the expensive
    // entity/scriptcomponent scan in that case.
    const String& thisAppName = app->applicationName.Get().Trimmed();
    if (thisAppName.Empty())
        return;

    JavaScriptInstance* jsInstance = dynamic_cast<JavaScriptInstance*>(app->ScriptInstance());
    if (!jsInstance || !jsInstance->IsEvaluated())
    {
        LogError("CreateScriptObjects: the application Script component does not have a script engine that has already evaluated its code");
        return;
    }
    
    Entity* appEntity = app->ParentEntity();
    if (!appEntity)
        return;
    Scene* scene = appEntity->ParentScene();
    if (!scene)
        return;
    String appName, className;
    // Get all script components that possibly refer to this application
    Vector<SharedPtr<Script> > scripts = scene->Components<Script>();
    for(unsigned i = 0; i < scripts.Size(); ++i)
        if (scripts[i]->ShouldRun())
        {
            ParseAppAndClassName(scripts[i].Get(), appName, className);
            if (appName == thisAppName)
                CreateScriptObject(app, scripts[i].Get(), className);
        }
}

void JavaScript::RemoveScriptObjects(JavaScriptInstance* jsInstance)
{
    if (jsInstance)
        jsInstance->Execute("_RemoveScriptObjects");
}

}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::JavaScript(fw));
}

}
