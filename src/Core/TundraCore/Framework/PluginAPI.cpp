// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "PluginAPI.h"
#include "Framework.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

#include <File.h>
#include <FileSystem.h>
#include <Log.h>
#include <ForEach.h>
#include <XMLFile.h>

using namespace Urho3D;

namespace Tundra
{

/// @todo Move to SystemInfo?
static String GetErrorString(int error)
{
#ifdef WIN32
    void *lpMsgBuf = 0;

    HRESULT hresult = HRESULT_FROM_WIN32(error);
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        0, hresult, 0 /*Default language*/, (LPTSTR) &lpMsgBuf, 0, 0);

    // Copy message to C++ -style string, since the data need to be freed before return.
    String ret;
    ret += String(WString((wchar_t*)lpMsgBuf));
    ret += "(";
    ret += error;
    ret += ")";
    LocalFree(lpMsgBuf);
    return ret;
#else
    return String(strerror(error)) + "(" + String(error) + ")";
#endif
}

/// Signature for Tundra plugins
typedef void (*TundraPluginMainSignature)(Framework *owner);

PluginAPI::PluginAPI(Framework *framework) :
    Object(framework->GetContext()),
    owner(framework)
{
}

void PluginAPI::LoadPlugin(const String &filename)
{
#ifdef WIN32
  #ifdef _DEBUG
    const String pluginSuffix = "_d.dll";
  #else
    const String pluginSuffix = ".dll";
  #endif
#elif defined(__APPLE__)
    const String pluginSuffix = ".dylib";
#else
    const String pluginSuffix = ".so";
#endif

    FileSystem* fs = GetSubsystem<FileSystem>();

    String path = GetNativePath(owner->InstallationDirectory() + "plugins/" + filename.Trimmed() + pluginSuffix);
    if (!fs->FileExists(path))
    {
        LOGWARNINGF("Cannot load plugin \"%s\" as the file does not exist.", path.CString());
        return;
    }
    LOGINFO("Loading plugin " + filename);
    //owner->App()->SetSplashMessage("Loading plugin " + filename);

#ifdef WIN32
    HMODULE module = LoadLibraryW(WString(path).CString());
    if (module == NULL)
    {
        DWORD errorCode = GetLastError();
        LOGERRORF("Failed to load plugin from \"%s\": %s (Missing dependencies?)", path.CString(), GetErrorString(errorCode).CString());
        return;
    }
    TundraPluginMainSignature mainEntryPoint = (TundraPluginMainSignature)GetProcAddress(module, "TundraPluginMain");
    if (mainEntryPoint == NULL)
    {
        DWORD errorCode = GetLastError();
        LOGERRORF("Failed to find plugin startup function 'TundraPluginMain' from plugin file \"%s\": %s", path.CString(), GetErrorString(errorCode).CString());
        return;
    }
#else
    const char *dlerrstr;
    dlerror();
    void *module = dlopen(path.toStdString().c_str(), RTLD_GLOBAL|RTLD_LAZY);
    if ((dlerrstr=dlerror()) != 0)
    {
        LOGERROR("Failed to load plugin from file \"" + path + "\": Error " + String(dlerrstr) + "!");
        return;
    }

    dlerror();
    TundraPluginMainSignature mainEntryPoint = (TundraPluginMainSignature)dlsym(module, "TundraPluginMain");
    if ((dlerrstr=dlerror()) != 0)
    {
        LOGERROR("Failed to find plugin startup function 'TundraPluginMain' from plugin file \"" + path + "\": Error " + String(dlerrstr) + "!");
        return;
    }
#endif
    Plugin p = { module, filename, path };
    plugins.Push(p);
    mainEntryPoint(owner);
}

void PluginAPI::UnloadPlugins()
{
    foreach(const Plugin &plugin, plugins)
    {
#ifdef WIN32
        FreeLibrary((HMODULE)plugin.handle);
#else
    /// \bug caused memory errors in destructors in the dlclose call chain
    //        dlclose(iter->handle);
#endif
    }
    plugins.Clear();
}

void PluginAPI::ListPlugins() const
{
    LOGINFO("Loaded plugins:");
    foreach(const Plugin &plugin, plugins)
        LOGINFO(plugin.name);
}

Vector<String> PluginAPI::ConfigurationFiles() const
{
    Vector<String> configs;
    Vector<String> cmdLineParams = owner->CommandLineParameters("--config");
    if (cmdLineParams.Size() > 0)
        foreach(const String &config, cmdLineParams)
            configs.Push(owner->LookupRelativePath(config));
    return configs;
}

void PluginAPI::LoadPluginsFromXML(String pluginConfigurationFile)
{
    bool showDeprecationWarning = true;
    pluginConfigurationFile = owner->LookupRelativePath(pluginConfigurationFile);

    XMLFile doc(GetContext());
    File file(GetContext(), pluginConfigurationFile, FILE_READ);
    if (!doc.Load(file))
    {
        LOGERROR("PluginAPI::LoadPluginsFromXML: Failed to open file \"" + pluginConfigurationFile + "\"!");
        return;
    }
    file.Close();

    XMLElement root = doc.GetRoot();
    XMLElement e = root.GetChild("plugin");
    while(e)
    {
        if (e.HasAttribute("path"))
        {
            String pluginPath = e.GetAttribute("path");
            LoadPlugin(pluginPath);
            if (showDeprecationWarning)
            {
                LOGWARNING("PluginAPI::LoadPluginsFromXML: In file " + pluginConfigurationFile + ", using XML tag <plugin path=\"PluginNameHere\"/> will be deprecated. Consider replacing it with --plugin command line argument instead");
                showDeprecationWarning = false;
            }
        }
        e = e.GetNext("plugin");
    }
}

void PluginAPI::LoadPluginsFromCommandLine()
{
    if (!owner->HasCommandLineParameter("--plugin"))
        return;

    Vector<String> plugins = owner->CommandLineParameters("--plugin");
    foreach(String plugin, plugins)
    {
        plugin = plugin.Trimmed();
        if (!plugin.Contains(";"))
            LoadPlugin(plugin);
        else
        {
            Vector<String> entries = plugin.Split(';');
            foreach(const String& entry, entries)
                LoadPlugin(entry);
        }
    }
}

}
