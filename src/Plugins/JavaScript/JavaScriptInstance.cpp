// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "JavaScript.h"
#include "JavaScriptInstance.h"
#include "ScriptAsset.h"
#include "Framework.h"
#include "LoggingFunctions.h"
#include "AssetAPI.h"

#include <Urho3D/Core/Profiler.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/File.h>

namespace Tundra
{

#define JS_PROFILE(name) Urho3D::AutoProfileBlock profile_ ## name (module_->GetSubsystem<Urho3D::Profiler>(), #name)

JavaScriptInstance::JavaScriptInstance(const String &fileName, JavaScript *module) :
    ctx_(0),
    sourceFile(fileName),
    module_(module),
    evaluated(false)
{
    assert(module);
    CreateEngine();
    Load();
}

JavaScriptInstance::JavaScriptInstance(ScriptAssetPtr scriptRef, JavaScript *module) :
    ctx_(0),
    module_(module),
    evaluated(false)
{
    assert(module);
    // Make sure we do not push null or empty script assets as sources
    if (scriptRef && !scriptRef->scriptContent.Empty())
        scriptRefs_.Push(scriptRef);

    CreateEngine();
    Load();
}

JavaScriptInstance::JavaScriptInstance(const Vector<ScriptAssetPtr>& scriptRefs, JavaScript *module) :
    ctx_(0),
    module_(module),
    evaluated(false)
{
    assert(module);
    // Make sure we do not push null or empty script assets as sources
    for (unsigned i = 0; i < scriptRefs.Size(); ++i)
        if (scriptRefs[i] && !scriptRefs[i]->scriptContent.Empty()) scriptRefs_.Push(scriptRefs[i]);

    CreateEngine();
    Load();
}

void JavaScriptInstance::CreateEngine()
{
    ctx_ = duk_create_heap_default();
    module_->ScriptInstanceCreated.Emit(this);
}

void JavaScriptInstance::DeleteEngine()
{
    if (ctx_)
    {
        program_ = "";
        ScriptUnloading.Emit();

        duk_destroy_heap(ctx_);
        ctx_ = 0;
    }
}

JavaScriptInstance::~JavaScriptInstance()
{
    DeleteEngine();
}

HashMap<String, uint> JavaScriptInstance::DumpEngineInformation()
{
    /// \todo Implement
    return HashMap<String, uint>();
}

void JavaScriptInstance::Load()
{
    JS_PROFILE(JSInstance_Load);
    if (!ctx_)
        CreateEngine();

    if (sourceFile.Empty() && scriptRefs_.Empty())
    {
        LogError("JavascriptInstance::Load: No script content to load!");
        return;
    }
    // Can't specify both a file source and an Asset API source.
    if (!sourceFile.Empty() && !scriptRefs_.Empty())
    {
        LogError("JavascriptInstance::Load: Cannot specify both an local input source file and a list of script refs to load!");
        return;
    }

    bool useAssetAPI = !scriptRefs_.Empty();
    size_t numScripts = useAssetAPI ? scriptRefs_.Size() : 1;

    // Determine based on code origin whether it can be trusted with system access or not
    if (useAssetAPI)
    {
        trusted_ = true;
        for (unsigned i = 0; i < scriptRefs_.Size(); ++i)
            trusted_ = trusted_ && scriptRefs_[i]->IsTrusted();
    }
    else // Local file: always trusted.
    {
        program_ = LoadScript(sourceFile);
        trusted_ = true; // This is a file on the local filesystem. We are making an assumption nobody can inject untrusted code here.
        // Actually, we are assuming the attacker does not know the absolute location of the asset cache locally here, since if he makes
        // the client to load a script into local cache, he could use this code path to automatically load that unsafe script from cache, and make it trusted. -jj.
    }

    /// \todo Check validity at this point
}

String JavaScriptInstance::LoadScript(const String &fileName)
{
    JS_PROFILE(JSInstance_LoadScript);
    String filename = fileName.Trimmed();

    // First check if the include was supposed to go through the Asset API.
    ScriptAssetPtr asset = Urho3D::DynamicCast<ScriptAsset>(module_->GetFramework()->Asset()->FindAsset(fileName));
    if (asset)
        return asset->scriptContent;

    /// @bug When including other scripts from startup scripts the only way to include is with relative paths.
    /// As you cannot use !rel: ref in startup scripts (loaded without EC_Script) so you cannot use local:// refs either. 
    /// You have to do engine.IncludeFile("lib/class.js") etc. and this below code needs to find the file whatever the working dir is

    // Otherwise, treat fileName as a local file to load up.
    // Check install dir and the clean rel path.
    String installRelativePath = AddTrailingSlash(module_->Fw()->InstallationDirectory() + "jsmodules") + filename;
    String pathToFile;
    Urho3D::FileSystem* fs = module_->GetSubsystem<Urho3D::FileSystem>();
    if (fs->FileExists(installRelativePath))
        pathToFile = installRelativePath;
    else if (fs->FileExists(filename))
        pathToFile = filename;
    if (pathToFile.Empty())
    {
        LogError("JavascriptInstance::LoadScript: Failed to load script from file " + filename + "!");
        return "";
    }

    Urho3D::File scriptFile(module_->GetContext(), pathToFile, Urho3D::FILE_READ);
    if (!scriptFile.IsOpen())
    {
        LogError("JavascriptInstance::LoadScript: Failed to load script from file " + filename + "!");
        return "";
    }

    String result;
    result.Resize(scriptFile.GetSize());
    scriptFile.Read(&result[0], result.Length());
    scriptFile.Close();

    String trimmedResult = result.Trimmed();
    if (trimmedResult.Empty())
    {
        LogWarning("JavascriptInstance::LoadScript: Warning Loaded script from file " + filename + ", but the content was empty.");
        return "";
    }
    return result;
}

void JavaScriptInstance::Unload()
{
    DeleteEngine();
}

void JavaScriptInstance::Run()
{
    JS_PROFILE(JSInstance_Run);
    // Need to have either absolute file path source or an Asset API source.
    if (scriptRefs_.Empty() && program_.Empty())
    {
        LogError("JavascriptInstance::Run: Cannot run, no script reference loaded.");
        return;
    }

    // Can't specify both a file source and an Asset API source.
    assert(sourceFile.Empty() || scriptRefs_.Empty());

    // If we've already evaluated this script once before, create a new script engine to run it again, or otherwise
    // the effects would stack (we'd possibly register into signals twice, or other odd side effects).
    // We never allow a script to be run twice in this kind of "stacking" manner.
    if (evaluated)
    {
        Unload();
        Load();
    }

    if (!ctx_)
    {
        LogError("JavascriptInstance::Run: Cannot run, script engine not loaded.");
        return;
    }

    // If no script specified at all, we'll have to abort.
    if (program_.Empty() && scriptRefs_.Empty())
        return;

    bool useAssets = !scriptRefs_.Empty();
    size_t numScripts = useAssets ? scriptRefs_.Size() : 1;
    includedFiles.Clear();

    for (size_t i = 0; i < numScripts; ++i)
    {
        JS_PROFILE(JSInstance_Evaluate);
        String scriptSourceFilename = (useAssets ? scriptRefs_[i]->Name() : sourceFile);
        const String &scriptContent = (useAssets ? scriptRefs_[i]->scriptContent : program_);

        if (!Evaluate(scriptContent))
            break;
    }

    evaluated = true;
    ScriptEvaluated.Emit();
}

bool JavaScriptInstance::Evaluate(const String& script)
{
    duk_push_string(ctx_, script.CString());
    bool success = duk_peval(ctx_) == 0;
    if (!success)
        LogError("[JavaScript] Evaluate error: " + String(duk_safe_to_string(ctx_, -1)));

    duk_pop(ctx_); // Pop result/error
    return success;
}

bool JavaScriptInstance::Execute(const String& functionName)
{
    duk_push_global_object(ctx_);
    duk_get_prop_string(ctx_, -1, functionName.CString());
    bool success = duk_pcall(ctx_, 0) == 0;
    if (duk_pcall(ctx_, 0) != 0)
        LogError("[JavaScript] Execute error: " + String(duk_safe_to_string(ctx_, -1)));

    duk_pop(ctx_); // Pop result/error
    duk_pop(ctx_); // Pop global object
    return success;
}

}
