// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Framework.h"
#include "JSON.h"
#include "PluginAPI.h"
#include "ConfigAPI.h"

#include <Context.h>
#include <Engine.h>
#include <FileSystem.h>
#include <IModule.h>
#include <Log.h>
#include <File.h>
#include <XMLFile.h>
#include <ProcessUtils.h>

using namespace Urho3D;

namespace Tundra
{

static int argc;
static char** argv;

int run(int argc_, char** argv_)
{
    argc = argc_;
    argv = argv_;

    Context ctx;
    Framework fw(&ctx);
    fw.Go();
    return 0;
}

Framework::Framework(Context* ctx) :
    Object(ctx),
    organizationName("realXtend"),
    applicationName("tundra-urho3d"),
    exitSignal(false),
    headless(false)
{
    plugin = new PluginAPI(this);
    config = new ConfigAPI(this);

    // Create the Urho3D engine, which creates various other subsystems, but does not initialize them yet
    engine = new Urho3D::Engine(GetContext());
    // Timestamps clutter the log. Disable for now
    GetSubsystem<Log>()->SetTimeStamp(false);

    ProcessStartupOptions();
    // In headless mode, no main UI/rendering window is initialized.
    headless = HasCommandLineParameter("--headless");

    // Open console window if necessary
    if (headless)
        OpenConsoleWindow();
}

Framework::~Framework()
{
    plugin.Reset();
    config.Reset();
}

void Framework::Go()
{
    // Initialization prints
    LOGINFO("Starting up");
    LOGINFO("* Installation directory : " + InstallationDirectory());
    LOGINFO("* Working directory      : " + CurrentWorkingDirectory());
    LOGINFO("* User data directory    : " + UserDataDirectory());

    // Prepare ConfigAPI data folder
    Vector<String> configDirs = CommandLineParameters("--configDir");
    String configDir = "$(USERDATA)/configuration"; // The default configuration goes to "C:\Users\username\AppData\Roaming\Tundra\configuration"
    if (configDirs.Size() >= 1)
        configDir = configDirs.Back();
    if (configDirs.Size() > 1)
        LOGWARNING("Multiple --configDir parameters specified! Using \"" + configDir + "\" as the configuration directory.");
    config->PrepareDataFolder(configDir);

    // Set target FPS limits, if specified.
    ConfigData targetFpsConfigData(ConfigAPI::FILE_FRAMEWORK, ConfigAPI::SECTION_RENDERING);
    if (config->HasKey(targetFpsConfigData, "fps target limit"))
    {
        int targetFps = config->Read(targetFpsConfigData, "fps target limit").GetInt();
        if (targetFps >= 0)
            engine->SetMaxFps(targetFps);
        else
            LOGWARNING("Invalid target FPS value " + String(targetFps) + " read from config. Ignoring.");
    }

    Vector<String> fpsLimitParam = CommandLineParameters("--fpsLimit");
    if (fpsLimitParam.Size() > 1)
        LOGWARNING("Multiple --fpslimit parameters specified! Using " + fpsLimitParam.Front() + " as the value.");
    if (fpsLimitParam.Size() > 0)
    {
        int targetFpsLimit = ToInt(fpsLimitParam.Front());
        if (targetFpsLimit >= 0)
            engine->SetMaxFps(targetFpsLimit);
        else
            LOGWARNING("Erroneous FPS limit given with --fpsLimit: " + fpsLimitParam.Front() + ". Ignoring.");
    }

    Vector<String> fpsLimitInactiveParam = CommandLineParameters("--fpsLimitWhenInactive");
    if (fpsLimitInactiveParam.Size() > 1)
        LOGWARNING("Multiple --fpslimit parameters specified! Using " + fpsLimitInactiveParam.Front() + " as the value.");
    if (fpsLimitInactiveParam.Size() > 0)
    {
        int targetFpsLimit = ToInt(fpsLimitParam.Front());
        if (targetFpsLimit >= 0)
            engine->SetMaxInactiveFps(targetFpsLimit);
        else
            LOGWARNING("Erroneous FPS limit given with --fpsLimit: " + fpsLimitInactiveParam.Front() + ". Ignoring.");
    }
    
    PrintStartupOptions();

    // Load and initialize plugins
    plugin->LoadPluginsFromCommandLine();

    for(size_t i = 0; i < modules.Size(); ++i)
    {
        LOGDEBUG("Initializing module " + modules[i]->Name());
        modules[i]->Initialize();
    }

    // Initialize the Urho3D engine
    VariantMap engineInitMap;
    engineInitMap["ResourcePaths"] = GetSubsystem<FileSystem>()->GetProgramDir() + "Data";
    engineInitMap["AutoloadPaths"] = "";
    engineInitMap["FullScreen"] = false;
    engineInitMap["Headless"] = headless;
    engineInitMap["WindowTitle"] = "Tundra";
    engineInitMap["LogName"] = "Tundra.log";
    engine->Initialize(engineInitMap);

    // Run mainloop
    if (!exitSignal)
    {
        while (!engine->IsExiting())
            ProcessOneFrame();
    }

    for(size_t i = 0; i < modules.Size(); ++i)
    {
        LOGDEBUG("Uninitializing module " + modules[i]->Name());
        modules[i]->Uninitialize();
    }

    for(size_t i = 0; i < modules.Size(); ++i)
    {
        LOGDEBUG("Unloading module " + modules[i]->Name());
        modules[i]->Unload();
    }

    // Delete all modules.
    modules.Clear();

    // Actually unload all DLL plugins from memory.
    plugin->UnloadPlugins();
}

void Framework::Exit()
{
    exitSignal = true;
    exitRequested.Emit();
}

void Framework::ForceExit()
{
    exitSignal = true;
    engine->Exit(); // Can not be canceled
}

void Framework::CancelExit()
{
    exitSignal = false;
}

void Framework::ProcessOneFrame()
{
    float dt = engine->GetNextTimeStep();

    Time* time = GetSubsystem<Time>();
    time->BeginFrame(dt);

    for(unsigned i = 0; i < modules.Size(); ++i)
        modules[i]->Update(dt);

    // Perform Urho engine update/render/measure next timestep
    engine->Update();
    engine->Render();
    engine->ApplyFrameLimit();

    time->EndFrame();

    if (exitSignal)
        engine->Exit();
}

void Framework::RegisterModule(IModule *module)
{
    modules.Push(SharedPtr<IModule>(module));
    module->Load();
}

IModule *Framework::ModuleByName(const String &name) const
{
    for(unsigned i = 0; i < modules.Size(); ++i)
        if (modules[i]->Name() == name)
            return modules[i].Get();
    return 0;
}

Engine* Framework::Engine() const
{
    return engine;
}

void Framework::SetCurrentWorkingDirectory(const String& newCwd)
{
    GetSubsystem<FileSystem>()->SetCurrentDir(newCwd);
}

String Framework::CurrentWorkingDirectory() const
{
    return GetSubsystem<FileSystem>()->GetCurrentDir();
}

String Framework::InstallationDirectory() const
{
    return GetSubsystem<FileSystem>()->GetProgramDir();
}

String Framework::UserDataDirectory() const
{
    return GetInternalPath(GetSubsystem<FileSystem>()->GetAppPreferencesDir(OrganizationName(), ApplicationName()));
}

String Framework::UserDocumentsDirectory() const
{
    return GetSubsystem<FileSystem>()->GetUserDocumentsDir();
}

String Framework::ParseWildCardFilename(const String& input) const
{
    // Parse all the special symbols from the log filename.
    String filename = input.Trimmed().Replaced("$(CWD)/", CurrentWorkingDirectory());
    filename = filename.Replaced("$(INSTDIR)/", InstallationDirectory());
    filename = filename.Replaced("$(USERDATA)/", UserDataDirectory());
    filename = filename.Replaced("$(USERDOCS)/", UserDocumentsDirectory());
    return filename;
}

String Framework::LookupRelativePath(String path) const
{
    FileSystem* fs = GetSubsystem<FileSystem>();

    // If a relative path was specified, lookup from cwd first, then from application installation directory.
    if (IsAbsolutePath(path))
    {
        String cwdPath = CurrentWorkingDirectory() + path;
        if (fs->FileExists(cwdPath))
            return cwdPath;
        else
            return InstallationDirectory() + path;
    }
    else
        return path;
}

void Framework::AddCommandLineParameter(const String &command, const String &parameter)
{
    String commandLowercase = command.ToLower();
    startupOptions[commandLowercase].first_ = command;
    startupOptions[commandLowercase].second_.Push(parameter);
}

bool Framework::HasCommandLineParameter(const String &value) const
{
    String valueLowercase = value.ToLower();
    if (value == "--config")
        return !configFiles.Empty();

    return startupOptions.Find(valueLowercase) != startupOptions.End();
}

Vector<String> Framework::CommandLineParameters(const String &key) const
{
    String keyLowercase = key.ToLower();
    if (key == "--config")
        return ConfigFiles();
    
    OptionsMap::ConstIterator i = startupOptions.Find(keyLowercase);
    if (i != startupOptions.End())
        return i->second_.second_;
    else
        return Vector<String>();
}

void Framework::ProcessStartupOptions()
{
    for(int i = 1; i < argc; ++i)
    {
        String option(argv[i]);
        String peekOption = (i+1 < argc ? String(argv[i+1]) : "");
        if (option.StartsWith("--") && !peekOption.Empty())
        {
#ifdef WIN32
            // --key "value
            if (peekOption.StartsWith("\""))
            {
                // --key "value"
                if (peekOption.EndsWith("\""))
                {
                    // Remove quotes and append to the return list.
                    peekOption = peekOption.Substring(1, peekOption.Length() - 1);
                }
                // --key "val u e"
                else
                {
                    for(int pi=i+2; pi+1 < argc; ++pi)
                    {
                        // If a new -- key is found before an end quote we have a error.
                        // Report and don't add anything to the return list as the param is malformed.
                        String param = argv[pi];
                        if (param.StartsWith("--"))
                        {
                            LOGERROR("Could not find an end quote for '" + option + "' parameter: " + peekOption);
                            i = pi - 1; // Step one back so the main for loop will inspect this element next.
                            break;
                        }
                        
                        peekOption += " " + param;
                        if (param.EndsWith("\""))
                        {                            
                            if (peekOption.StartsWith("\""))
                                peekOption = peekOption.Substring(1);
                            if (peekOption.EndsWith("\""))
                                peekOption.Resize(peekOption.Length() - 1);

                            // Set the main for loops index so it will skip the 
                            // parts that included in this quoted param.
                            i = pi; 
                            break;
                        }
                    }
                }
            }
#endif
        }
        else if (option.StartsWith("--") && peekOption.Empty())
            AddCommandLineParameter(option);
        else
        {
            LOGWARNING("Orphaned startup option parameter value specified: " + String(argv[i]));
            continue;
        }

        if (option.Trimmed().Empty())
            continue;

        // --config
        if (option.Compare("--config", false) == 0)
        {
            LoadStartupOptionsFromFile(peekOption);
            ++i;
            continue;
        }

        // --key value
        if (!peekOption.StartsWith("--"))
        {
            AddCommandLineParameter(option, peekOption);
            ++i;
        }
        // --key
        else
            AddCommandLineParameter(option);
    }

    if (!HasCommandLineParameter("--config"))
        LoadStartupOptionsFromFile("tundra.json");
}

void Framework::PrintStartupOptions()
{
    LOGINFO("Startup options:");
    for (OptionsMap::ConstIterator i = startupOptions.Begin(); i != startupOptions.End(); ++i)
    {
        String option = i->second_.first_;
        LOGINFO("  " + option);
        for (unsigned j = 0; j < i->second_.second_.Size(); ++j)
        {
            if (!i->second_.second_[j].Empty())
                LOGINFO("    '" + i->second_.second_[j] + "'");
        }
    }
}

bool Framework::LoadStartupOptionsFromFile(const String &configurationFile)
{
    String suffix = GetExtension(configurationFile);
    bool read = false;
    if (suffix == ".xml")
        read = LoadStartupOptionsFromXML(configurationFile);
    else if (suffix == ".json")
        read = LoadStartupOptionsFromJSON(configurationFile);
    else
        LOGERROR("Invalid config file format. Only .xml and .json are supported: " + configurationFile);
    if (read)
        configFiles.Push(configurationFile);
    return read;
}

bool Framework::LoadStartupOptionsFromXML(String configurationFile)
{
    configurationFile = LookupRelativePath(configurationFile);

    XMLFile doc(GetContext());
    File file(GetContext(), configurationFile, FILE_READ);
    if (!doc.Load(file))
    {
        LOGERROR("Failed to open config file \"" + configurationFile + "\"!");
        return false;
    }

    XMLElement root = doc.GetRoot();

    XMLElement e = root.GetChild("option");
    while(e)
    {
        if (e.HasAttribute("name"))
        {
            /// \todo Support build exclusion

            /// If we have another config XML specified with --config inside this config XML, load those settings also
            if (e.GetAttribute("name").Compare("--config", false) == 0)
            {
                if (!e.GetAttribute("value").Empty())
                    LoadStartupOptionsFromFile(e.GetAttribute("value"));
                e = e.GetNext("option");
                continue;
            }

            AddCommandLineParameter(e.GetAttribute("name"), e.GetAttribute("value"));
        }
        e = e.GetNext("option");
    }
    return true;
}

bool Framework::LoadStartupOptionsFromJSON(String configurationFile)
{
    configurationFile = LookupRelativePath(configurationFile);
    
    File file(GetContext(), configurationFile, FILE_READ);
    if (!file.IsOpen())
    {
        LOGERROR("Failed to open config file \"" + configurationFile + "\"!");
        return false;
    }
    JSONValue root;
    if (!root.FromString(file.ReadString()))
    {
        LOGERROR("Failed to parse config file \"" + configurationFile + "\"!");
        return false;
    }

    if (root.IsArray())
        LoadStartupOptionArray(root);
    else if (root.IsObject())
        LoadStartupOptionMap(root);
    else if (root.IsString())
        AddCommandLineParameter(root.GetString());
    else
        LOGERROR("JSON config file " + configurationFile + " was not an object, array or string");
    
    return true;
}

void Framework::LoadStartupOptionArray(const JSONValue& value)
{
    const JSONArray& arr = value.GetArray();
    for (JSONArray::ConstIterator i = arr.Begin(); i != arr.End(); ++i)
    {
        const JSONValue& innerValue = *i;
        if (innerValue.IsArray())
            LoadStartupOptionArray(innerValue);
        else if (innerValue.IsObject())
            LoadStartupOptionMap(innerValue);
        else if (innerValue.IsString())
            AddCommandLineParameter(innerValue.GetString());
    }
}

void Framework::LoadStartupOptionMap(const JSONValue& value)
{
    const JSONObject& obj = value.GetObject();
    for (JSONObject::ConstIterator i = obj.Begin(); i != obj.End(); ++i)
    {
        /// \todo Support build and platform exclusion

        String option = i->first_;
        if (i->second_.IsString())
        {
            if (option.Compare("--config", false) == 0)
                LoadStartupOptionsFromFile(i->second_.GetString());
            else
                AddCommandLineParameter(option, i->second_.GetString());
        }
        else if (i->second_.IsArray())
        {
            const JSONArray& innerArr = i->second_.GetArray();
            for (JSONArray::ConstIterator j = innerArr.Begin(); j != innerArr.End(); ++j)
            {
                if (j->IsString())
                {
                    if (option.Compare("--config", false) == 0)
                        LoadStartupOptionsFromFile(j->GetString());
                    else
                        AddCommandLineParameter(option, j->GetString());
                }
            }
        }
    }
}

}