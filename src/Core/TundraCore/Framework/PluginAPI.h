// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "FrameworkFwd.h"

#include <List.h>
#include <Object.h>

namespace Tundra
{

/// Implements plugin loading functionality.
class TUNDRACORE_API PluginAPI : public Urho3D::Object
{
    OBJECT(PluginAPI);

public:
    /// Returns list of plugin configuration files that were used to load the plugins at startup.
    Urho3D::Vector<Urho3D::String> ConfigurationFiles() const;

    /// Loads and executes the given shared library plugin.
    void LoadPlugin(const Urho3D::String &filename);

    /// Parses the specified .xml file and loads and executes all plugins specified in that file.
    void LoadPluginsFromXML(Urho3D::String pluginListFilename);

    /// Loads plugins specified on command line with --plugin
    void LoadPluginsFromCommandLine();

    void UnloadPlugins();

    /// Prints the list of loaded plugins to the console.
    void ListPlugins() const;

private:
    friend class Framework;

    /// @note Framework takes ownership of the object.
    explicit PluginAPI(Framework *framework);

    struct Plugin
    {
        void *handle;
        Urho3D::String name;
        Urho3D::String filename;
    };
    Urho3D::Vector<Plugin> plugins;

    Framework *owner;
};

}

