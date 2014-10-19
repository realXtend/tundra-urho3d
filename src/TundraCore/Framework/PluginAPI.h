// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "FrameworkFwd.h"
#include "CoreTypes.h"

#include <List.h>
#include <Object.h>

namespace Tundra
{

/// Implements plugin loading functionality.
class TUNDRACORE_API PluginAPI : public Object
{
    OBJECT(PluginAPI);

public:
    /// Returns list of plugin configuration files that were used to load the plugins at startup.
    Vector<String> ConfigurationFiles() const;

    /// Loads and executes the given shared library plugin.
    void LoadPlugin(const String &filename);

    /// Parses the specified .xml file and loads and executes all plugins specified in that file.
    void LoadPluginsFromXML(String pluginListFilename);

    /// Loads plugins specified on command line with --plugin
    void LoadPluginsFromCommandLine();

    void UnloadPlugins();

    /// Prints the list of loaded plugins to the console.
    void ListPlugins();

private:
    friend class Framework;

    /// @note Framework takes ownership of the object.
    explicit PluginAPI(Framework *framework);

    struct Plugin
    {
        void *handle;
        String name;
        String filename;
    };
    Vector<Plugin> plugins;

    Framework *owner;
};

}

