// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "FrameworkFwd.h"

#include <Object.h>
#include <StringUtils.h>

namespace Tundra
{

/// Convenience structure for dealing constantly with same config file/sections.
struct TUNDRACORE_API ConfigData
{
    ConfigData() {}

    ConfigData(const Urho3D::String &cfgFile, const Urho3D::String &cfgSection, const Urho3D::String &cfgKey = Urho3D::String(),
        const Urho3D::Variant &cfgValue = Urho3D::Variant(), const Urho3D::Variant &cfgDefaultValue = Urho3D::Variant()) :
        file(cfgFile),
        section(cfgSection),
        key(cfgKey),
        value(cfgValue),
        defaultValue(cfgDefaultValue)
    {
    }

    Urho3D::String file;
    Urho3D::String section;
    Urho3D::String key;
    Urho3D::Variant value;
    Urho3D::Variant defaultValue;

    /// Returns string presentation of the contained data.
    Urho3D::String ToString() const
    {
        return Urho3D::ToString("ConfigData(file:%s section:%s key:%s value:%s defaultValue:%s)", file.CString(), section.CString(), value.ToString().CString(), defaultValue.ToString().CString());
    }

    /// @cond PRIVATE
    /// Same as ToString, exists for QtScript-compatibility.
    Urho3D::String toString() const { return ToString(); }
    /// @endcond
};

/// Structure for a config file section.
struct ConfigSection
{
    Urho3D::HashMap<Urho3D::String, Urho3D::Variant> keys;
};

/// Structure for sections contained within a config file.
struct ConfigFile
{
    ConfigFile() :
        loaded(false)
    {
    }

    Urho3D::HashMap<Urho3D::String, ConfigSection> sections;
    bool loaded;

    void Load(Urho3D::Context* ctx, const Urho3D::String& fileName);
    void Save(Urho3D::Context* ctx, const Urho3D::String& fileName);    
};

/// Configuration API for accessing config files.
/** The Configuration API utilizes Variants extensively for script-compatibility.
    In C++ code use the Variant::Get*() functions to convert the values to the correct type.
    The Config API supports ini sections but you may also write to the root of the ini document without a section.

    @note All file, key and section parameters are case-insensitive. This means all of them are transformed to 
    lower case before any accessing files. "MyKey" will get and set you same value as "mykey". */
class TUNDRACORE_API ConfigAPI : public Urho3D::Object
{
    OBJECT(ConfigAPI);

public:
    ///\todo Make these properties so that can be obtained to scripts too.
    static Urho3D::String FILE_FRAMEWORK;
    static Urho3D::String SECTION_FRAMEWORK;
    static Urho3D::String SECTION_SERVER;
    static Urho3D::String SECTION_CLIENT;
    static Urho3D::String SECTION_RENDERING;
    static Urho3D::String SECTION_UI;
    static Urho3D::String SECTION_SOUND;

    /// Returns if a key exists in the config.
    /** @param file Name of the file. For example: "foundation" or "foundation.ini" you can omit the .ini extension.
        @param section The section in the config where key is. For example: "login".
        @param key Key to look for in the file under section. */
    bool HasKey(Urho3D::String file, Urho3D::String section, Urho3D::String key) const;
    bool HasKey(const ConfigData &data) const; /**< @overload @param data Filled ConfigData object */
    bool HasKey(const ConfigData &data, Urho3D::String key) const; /**< @overload */

    /// @todo Add DeleteKey

    /// Returns value for a key in a config file
    /** @param file Name of the file. For example: "foundation" or "foundation.ini" you can omit the .ini extension.
        @param section The section in the config where key is. For example: "login".
        @param key Key that value gets returned. For example: "username".
        @param defaultValue What you expect to get back if the file/section/key combination was not found.
        @return The value of key/section in file. */
    Urho3D::Variant Read(Urho3D::String file, Urho3D::String section, Urho3D::String key, const Urho3D::Variant &defaultValue = Urho3D::Variant()) const;
    Urho3D::Variant Read(const ConfigData &data) const; /**< @overload @param data Filled ConfigData object. */
    /// @overload
    /** @param data ConfigData object that has file and section filled, also may have defaultValue and it will be used if input defaultValue is null. */
    Urho3D::Variant Read(const ConfigData &data, Urho3D::String key, const Urho3D::Variant &defaultValue = Urho3D::Variant()) const;

    /** Sets the value of key in a config file.
        @param file Name of the file. For example: "foundation" or "foundation.ini" you can omit the .ini extension.
        @param section The section in the config where key is. For example: "login".
        @param key Key that value gets set. For example: "username".
        @param value New value for the key.
        @note If setting value of type float, convert to double if you want the value to be human-readable in the file. */
    void Write(Urho3D::String file, Urho3D::String section, Urho3D::String key, const Urho3D::Variant &value);
    void Write(const ConfigData &data, Urho3D::String key, const Urho3D::Variant &value); /**< @overload @param data ConfigData object that has file and section filled. */
    void Write(const ConfigData &data, const Urho3D::Variant &value); /**< @overload @param data ConfigData object that has file, section and key filled. */
    void Write(const ConfigData &data); /**< @overload @param data Filled ConfigData object.*/

    /// Returns the absolute path to the config folder where configs are stored. Guaranteed to have a trailing forward slash '/'.
    Urho3D::String ConfigFolder() const { return configFolder_; }

    /// Declares a setting, meaning that if the setting doesn't exist in the config it will be created.
    /** @return The value of the setting the config, if the setting existed, or default value if the setting did not exist. */
    Urho3D::Variant DeclareSetting(const Urho3D::String &file, const Urho3D::String &section, const Urho3D::String &key, const Urho3D::Variant &defaultValue);
     /// @overload
    /** @note ConfigData::value will take precedence over ConfigData::defaultValue, if both are set, as the value that will be used for the default value. */
    Urho3D::Variant DeclareSetting(const ConfigData &data);
    Urho3D::Variant DeclareSetting(const ConfigData &data, const Urho3D::String &key, const Urho3D::Variant &defaultValue); /**< @overload */

private:
    friend class Framework;

    /// @note Framework takes ownership of the object.
    explicit ConfigAPI(Framework *framework);

    /// Get absolute file path for file. Guarantees that it ends with .ini.
    Urho3D::String GetFilePath(const Urho3D::String &file) const;

    /// Returns if file provided by the ConfigAPI caller is secure and we should write/read from it.
    /** The purpose of this function is to verify the file provided by calling code
        does not go out of the confined ConfigAPI folder. For security reasons we cannot let
        eg. scripts open configs where they like. The whole operation will be canceled if this validation fails. */
    bool IsFilePathSecure(const Urho3D::String &file) const;

    /// Prepare string for config usage. Removes spaces from end and start, replaces mid string spaces with '_' and forces to lower case.
    void PrepareString(Urho3D::String &str) const;

    /// Opens up the Config API to the given data folder. This call will make sure that the required folders exist.
    /** @param configFolderName The name of the folder to store Tundra Config API data to. */
    void PrepareDataFolder(Urho3D::String configFolderName);

    Framework *owner;
    Urho3D::String configFolder_; ///< Absolute path to the folder where to store the config files.
    mutable Urho3D::HashMap<Urho3D::String, ConfigFile> configFiles_; // Configuration file in-memory data.
};

}
