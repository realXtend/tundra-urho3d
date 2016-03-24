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
#include <Urho3D/IO/FileSystem.h>

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
    framework->Console()->RegisterCommand(
        "jsExec", "Execute given code in the embedded Javascript interpreter. Usage: jsExec(mycodestring)")->ExecutedWith.Connect(
        this, &JavaScript::RunStringCommand);

    framework->Console()->RegisterCommand(
        "jsLoad", "Execute a javascript file. jsLoad(myJsFile.js)")->ExecutedWith.Connect(
        this, &JavaScript::RunScriptCommand);

    framework->Console()->RegisterCommand(
        "jsReloadScripts", "Reloads and re-executes startup scripts.",
        this, &JavaScript::LoadStartupScripts);
        
    framework->Scene()->SceneCreated.Connect(this, &JavaScript::OnSceneCreated);

    // Initialize startup scripts
    LoadStartupScripts();

    StringVector runScripts = framework->CommandLineParameters("--run");
    foreach(const String &script, runScripts)
    {
        SharedPtr<JavaScriptInstance> jsInstance(new JavaScriptInstance(script, this));
        startupScripts_.Push(jsInstance);
        jsInstance->Run();
    }
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

    instance->RegisterService("framework", framework);
    instance->RegisterService("frame", framework->Frame());
    instance->RegisterService("input", framework->Input());
    instance->RegisterService("console", framework->Console());
    instance->RegisterService("asset", framework->Asset());
    instance->RegisterService("config", framework->Config());

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

StringList JavaScript::StartupScripts()
{
    StringList scripts;
    if (framework->HasCommandLineParameter("--jsplugin"))
        scripts.Push(framework->CommandLineParameters("--jsplugin"));
    return scripts;
}

void JavaScript::UnloadStartupScripts()
{
    startupScripts_.Clear();
}

void JavaScript::LoadStartupScripts()
{
    UnloadStartupScripts();

    StringVector startupScripts = StartupScripts();
    StringVector startupAutoScripts;

    String path = framework->InstallationDirectory() + "jsmodules/startup";
    Urho3D::FileSystem* fileSystem = GetSubsystem<Urho3D::FileSystem>();
    fileSystem->ScanDir(startupAutoScripts, path, "*.js", Urho3D::SCAN_FILES, true);

    // 1. Run any existing StartupScripts that reside in /jsmodules/startup first
    if (!startupAutoScripts.Empty())
    {
        LogInfo(Name() + ": Loading startup scripts from /jsmodules/startup");
        foreach (const String &script, startupAutoScripts)
        {
            String fullPath = path + "/" + script;
            LogInfo(Name() + ": ** " + script);
            SharedPtr<JavaScriptInstance> jsInstance(new JavaScriptInstance(fullPath, this));
            startupScripts_.Push(jsInstance);
            jsInstance->Run();
            startupScripts.Remove(fullPath);
            startupScripts.Remove(script);
        }
    }

    // 2. Load the rest of the references from the config files
    foreach (const String &script, startupScripts)
    {
        // Allow relative paths from '/<install_dir>/jsmodules' to start also
        String jsPluginsDir = framework->InstallationDirectory() + "jsmodules";

        // Only allow relative paths, maybe allow absolute paths as well, maybe even URLs at some point?
        if (Urho3D::IsAbsolutePath(script))
            continue;

        String pathToFile;
        if (fileSystem->FileExists(jsPluginsDir + "/" + script))
            pathToFile = jsPluginsDir + "/" + script;
        // Absolute path (above already ignored?)
        else if (fileSystem->FileExists(script))
            pathToFile = script;
        else
        {
            // Try relative to the startup config.
            LogWarning(Name() + "** Could not find startup file for: " + script);
            continue;
        }

        LogInfo(Name() + ": ** " + script);
        SharedPtr<JavaScriptInstance> jsInstance(new JavaScriptInstance(pathToFile, this));
        startupScripts_.Push(jsInstance);
        jsInstance->Run();
    }
}

void JavaScript::RunString(const String& codeString)
{
    if (!defaultInstance_)
        defaultInstance_ = new JavaScriptInstance(this);
    defaultInstance_->Evaluate(codeString);
}

void JavaScript::RunScript(const String& scriptFile)
{
    if (!defaultInstance_)
        defaultInstance_ = new JavaScriptInstance(this);
    String script = defaultInstance_->LoadScript(scriptFile);
    if (script.Length() > 0)
        defaultInstance_->Evaluate(script);
}

void JavaScript::RunStringCommand(const StringVector& params)
{
    String result; 
    result.Join(params, " ");
    RunString(result);
}

void JavaScript::RunScriptCommand(const StringVector& params)
{
    if (params.Size())
        RunScript(params[0]);
}

}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::JavaScript(fw));
}

}
