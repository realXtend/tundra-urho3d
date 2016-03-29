// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "FrameworkFwd.h"
#include "CoreTypes.h"
#include "Signals.h"

#include <Urho3D/Core/Object.h>

namespace Tundra
{

class JSONValue;

typedef HashMap<String, Pair<String, Vector<String> > > OptionsMap;

/// The system root access object.
class TUNDRACORE_API Framework : public Object 
{
    URHO3D_OBJECT(Framework, Object);

public:
    explicit Framework(Urho3D::Context* ctx);
    ~Framework();

    /// Reads cmd line parameters, loads modules and setups the Urho engine. [noscript]
    /** @note You must call this function before Go() or pumping your own main loop with ProcessOneFrame. */
    void Initialize();

    /// Clears Framework and unloads all plugins. Call this before destructing Framework. [noscript]
    void Uninitialize();

    /// Run the main loop until exit requested. [noscript]
    void Go();

    /// Alternative to Go(). This function will process once frame and return. [noscript]
    /** @return False if Framework or Engine is exiting and no processing was done, othewise true. */
    bool Pump();

    /// Runs through a single frame of logic update and rendering. [noscript]
    void ProcessOneFrame();

    /// Returns module by class T.
    /** @param T class type of the module.
        @return The module, or null if the module doesn't exist. Always remember to check for null pointer. */
    template <class T>
    T *Module() const;

    /// Returns module by name.
    /** @param name Name of the module. */
    IModule *ModuleByName(const String& name) const;

    /// Registers a new module into the Framework.
    /** Framework will take ownership of the module pointer, so it is safe to pass in a raw pointer. */
    void RegisterModule(IModule *module);

    /// Sets the current working directory. Use with caution. [noscript]
    void SetCurrentWorkingDirectory(const String& newCwd);

    /// Returns the cwd of the current environment. [noscript]
    /** This directory should not be relied, since it might change due to external code running.
        Always prefer to use InstallationDirectory, UserDataDirectory and UserDocumentsDirectory instead.
        The returned path contains a trailing slash. */
    String CurrentWorkingDirectory() const;

    /// Returns the directory where Tundra was installed to. [noscript]
    /** This is *always* the directory Tundra.exe resides in.
        E.g. on Windows 7 this is usually of form "C:\Program Files (x86)\Tundra 1.0.5\".
        The returned path contains a trailing slash. */
    String InstallationDirectory() const;

    /// Returns the directory that is used for Tundra data storage on the current user. [noscript]
    /** E.g. on Windows 7 this is usually of form "C:\Users\username\AppData\Roaming\Tundra\".
        The returned path contains a trailing slash. */
    String UserDataDirectory() const;

    /// Returns the directory where the documents (for Tundra) of the current user are located in. [noscript]
    /** E.g. on Windows 7 this is usually of form "C:\Users\username\Documents\Tundra\".
        The returned path contains a trailing slash. */
    String UserDocumentsDirectory() const;

    /// Return organization of the application, e.g. "realXtend".
    static const String& OrganizationName();

    /// Returns name of the application, "Tundra-urho3d" by default.
    static const String& ApplicationName();

    /// Return current execution platform. E.g. "Windows", "Linux", "Mac OS X", "Android", "iOS" or "Raspberry Pi". [property]
    String Platform() const;

#ifdef ANDROID
    /// Returns Android package name.
    static const String& PackageName();
#endif

    /// Returns application version string.
    static const String& VersionString();

    /// Parse a filename for specific wildcard modifiers, and return as parsed
    /** $(CWD) is expanded to the current working directory.
        $(INSTDIR) is expanded to the Tundra installation directory (Application::InstallationDirectory)
        $(USERDATA) is expanded to Application::UserDataDirectory.
        $(USERDOCS) is expanded to Application::UserDocumentsDirectory. */
    String ParseWildCardFilename(const String& input) const;

    /// Returns whether or not the command line arguments contain a specific value.
    /** @param value Key or value with possible prefixes, case-insensitive. */
    bool HasCommandLineParameter(const String &value) const;

    /// Returns list of command line parameter values for a specific @c key, f.ex. "--file".
    /** Value is considered to be the command line argument following the @c key.
        If the argument following @c key is another key-type argument (--something), it's not appended to the return list.
        @param key Key with possible prefixes, case-insensitive */
    Vector<String> CommandLineParameters(const String &key) const;

    /// Returns list of all the config XML filenames specified on command line or within another config XML
    Vector<String> ConfigFiles() const { return configFiles; }

    /// Lookup a filename relative to either the installation or current working directory. [noscript]
    String LookupRelativePath(String path) const;

    /// Request application exit. Exit request signal will be sent and the exit can be canceled by calling CancelExit();
    void Exit();

    /// Forcibly exit application, can not be canceled.
    void ForceExit();

    /// Cancel exit request.
    void CancelExit();

    /// Return whether is headless (no rendering)
    bool IsHeadless() const { return headless; }

    /// Returns core API Plugin object.
    PluginAPI* Plugin() const;

    /// Returns core API Config object.
    ConfigAPI* Config() const;

    /// Returns core API Frame object.
    FrameAPI* Frame() const;

    /// Returns core API Scene object.
    SceneAPI* Scene() const;

    /// Return core API Console object.
    ConsoleAPI* Console() const;

    /// Returns core API Asset object.
    AssetAPI* Asset() const;

    /// Returns core API Debug object.
    DebugAPI *Debug() const;

    /// Returns core API Input object.
    InputAPI* Input() const;

    /// Returns core API UI object.
    UiAPI* Ui() const;

    /// Return the Urho3D Engine object.
    Urho3D::Engine* Engine() const;

    /// @cond PRIVATE
    /// Registers the system Renderer object.
    /** @note Please don't use this function. Called only by the UrhoRenderer which implements the rendering subsystem. */
    void RegisterRenderer(IRenderer *renderer);
    /// @endcond

    /// Returns the system Renderer object.
    /** @note Please don't use this function. It exists for dependency inversion purposes only.
        Instead, call framework->Module<UrhoRenderer>()->Renderer(); to directly obtain the renderer,
        as that will make the dependency explicit. The IRenderer interface is not continuously updated to match the real Renderer implementation. */
    IRenderer *Renderer() const;

    /// Exit request signal.
    Signal0<void> ExitRequested;

    /// Return the static Framework instance. Only to be used internally.
    static Framework* Instance();

private:
    /// Processes command line options and stores them into a multimap
    void ProcessStartupOptions();

    /// Read and apply startup options relevant for Framework/Urho engine.
    void ApplyStartupOptions(VariantMap &engineInitMap);

    /// Prints to console all the used startup options.
    void PrintStartupOptions();

    /// Setup asset storages specifed on the command line.
    void SetupAssetStorages();

    /// Adds new command line parameter (option | value pair)
    void AddCommandLineParameter(const String &command, const String &parameter = "");

    /// Directs to XML of JSON parsing function depending on file suffix.
    bool LoadStartupOptionsFromFile(const String &configurationFile);
    
    /// Appends all found startup options from the given file to the startupOptions member.
    bool LoadStartupOptionsFromXML(String configurationFile);
    
    /// Appends all found startup options from the given file to the startupOptions member.
    bool LoadStartupOptionsFromJSON(String configurationFile);

    /// Load a JSON array to startup options.
    void LoadStartupOptionArray(const JSONValue& value);

    /// Load a JSON map to startup options.
    void LoadStartupOptionMap(const JSONValue& value);

    /// Save relevant TundraCore state to config.
    void SaveConfig();

    /// Load TundraCore state from config to @c engineInitMap.
    void LoadConfig(VariantMap &engineInitMap);

    /// Urho3D engine
    SharedPtr<Urho3D::Engine> engine;
    /// Framework owns the memory of all the modules in the system. These are freed when Framework is exiting.
    Vector<SharedPtr<IModule> > modules;

    /// FrameAPI
    SharedPtr<FrameAPI> frame;
    /// PluginAPI
    SharedPtr<PluginAPI> plugin;
    /// ConfigAPI
    SharedPtr<ConfigAPI> config;
    /// SceneAPI
    SharedPtr<SceneAPI> scene;
    /// ConsoleAPI
    SharedPtr<ConsoleAPI> console;
    /// AssetAPI
    SharedPtr<AssetAPI> asset;
    /// DebugAPI
    SharedPtr<DebugAPI> debug;
    /// InputAPI
    SharedPtr<InputAPI> input;
    // UiAPI
    SharedPtr<UiAPI> ui;

    /// Stores all command line parameters and expanded options specified in the Config XML files, except for the config file(s) themselves.
    OptionsMap startupOptions;
    /// Stores config XML filenames
    Vector<String> configFiles;

    /// Exiting flag. When raised, the Framework will exit on the next main loop iteration
    bool exitSignal;
    /// Headless flag. When headless, no rendering window is created
    bool headless;
    /// Renderer object
    IRenderer* renderer;
};

template <class T>
T *Framework::Module() const
{
    for(unsigned i = 0; i < modules.Size(); ++i)
    {
        T *module = dynamic_cast<T*>(modules[i].Get());
        if (module)
            return module;
    }

    return nullptr;
}

/// Instantiate the Framework and run until exited.
TUNDRACORE_API int run(int argc, char** argv);
TUNDRACORE_API void set_run_args(int argc, char** argv);

}
