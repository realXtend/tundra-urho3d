// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "FrameworkFwd.h"
#include "CoreTypes.h"

#include <Object.h>
#include <StringUtils.h>

namespace Tundra
{

/// Convenience structure for dealing constantly with same config file/sections.
struct TUNDRACORE_API ConfigData
{
    ConfigData() {}

    ConfigData(const String &cfgFile, const String &cfgSection, const String &cfgKey = String(),
        const Variant &cfgValue = Variant(), const Variant &cfgDefaultValue = Variant()) :
        file(cfgFile),
        section(cfgSection),
        key(cfgKey),
        value(cfgValue),
        defaultValue(cfgDefaultValue)
    {
    }

    String file;
    String section;
    String key;
    Variant value;
    Variant defaultValue;

    /// Returns string presentation of the contained data.
    String ToString() const
    {
        return Urho3D::ToString("ConfigData(file:%s section:%s key:%s value:%s defaultValue:%s)", file.CString(), section.CString(), value.ToString().CString(), defaultValue.ToString().CString());
    }

    /// @cond PRIVATE
    /// Same as ToString, exists for QtScript-compatibility.
    String toString() const { return ToString(); }
    /// @endcond
};

/// Structure for a config file section.
struct ConfigSection
{
    HashMap<String, Variant> keys;
};

/// Structure for sections contained within a config file.
struct ConfigFile
{
    ConfigFile() :
        loaded(false)
    {
    }

    HashMap<String, ConfigSection> sections;
    bool loaded;

    void Load(Urho3D::Context* ctx, const String& fileName);
    void Save(Urho3D::Context* ctx, const String& fileName);    
};

/// Configuration API for accessing config files.
/** The Configuration API utilizes Variants extensively for script-compatibility.
    In C++ code use the Variant::Get*() functions to convert the values to the correct type.
    The Config API supports ini sections but you may also write to the root of the ini document without a section.

    @note All file, key and section parameters are case-insensitive. This means all of them are transformed to 
    lower case before any accessing files. "MyKey" will get and set you same value as "mykey". */
class TUNDRACORE_API ConfigAPI : public Object
{
    OBJECT(ConfigAPI);

public:
    ///\todo Make these properties so that can be obtained to scripts too.
    static String FILE_FRAMEWORK;
    static String SECTION_FRAMEWORK;
    static String SECTION_SERVER;
    static String SECTION_CLIENT;
    static String SECTION_RENDERING;
    static String SECTION_UI;
    static String SECTION_SOUND;

    /// Returns if a key exists in the config.
    /** @param file Name of the file. For example: "foundation" or "foundation.ini" you can omit the .ini extension.
        @param section The section in the config where key is. For example: "login".
        @param key Key to look for in the file under section. */
    bool HasKey(String file, String section, String key) const;
    bool HasKey(const ConfigData &data) const; /**< @overload @param data Filled ConfigData object */
    bool HasKey(const ConfigData &data, String key) const; /**< @overload */

    /// @todo Add DeleteKey

    /// Returns value for a key in a config file
    /** @param file Name of the file. For example: "foundation" or "foundation.ini" you can omit the .ini extension.
        @param section The section in the config where key is. For example: "login".
        @param key Key that value gets returned. For example: "username".
        @param defaultValue What you expect to get back if the file/section/key combination was not found.
        @return The value of key/section in file. */
    Variant Read(String file, String section, String key, const Variant &defaultValue = Variant()) const;
    Variant Read(const ConfigData &data) const; /**< @overload @param data Filled ConfigData object. */
    /// @overload
    /** @param data ConfigData object that has file and section filled, also may have defaultValue and it will be used if input defaultValue is null. */
    Variant Read(const ConfigData &data, String key, const Variant &defaultValue = Variant()) const;

    /** Sets the value of key in a config file.
        @param file Name of the file. For example: "foundation" or "foundation.ini" you can omit the .ini extension.
        @param section The section in the config where key is. For example: "login".
        @param key Key that value gets set. For example: "username".
        @param value New value for the key.
        @note If setting value of type float, convert to double if you want the value to be human-readable in the file. */
    void Write(String file, String section, String key, const Variant &value);
    void Write(const ConfigData &data, String key, const Variant &value); /**< @overload @param data ConfigData object that has file and section filled. */
    void Write(const ConfigData &data, const Variant &value); /**< @overload @param data ConfigData object that has file, section and key filled. */
    void Write(const ConfigData &data); /**< @overload @param data Filled ConfigData object.*/

    /// Returns the absolute path to the config folder where configs are stored. Guaranteed to have a trailing forward slash '/'.
    String ConfigFolder() const { return configFolder_; }

    /// Declares a setting, meaning that if the setting doesn't exist in the config it will be created.
    /** @return The value of the setting the config, if the setting existed, or default value if the setting did not exist. */
    Variant DeclareSetting(const String &file, const String &section, const String &key, const Variant &defaultValue);
     /// @overload
    /** @note ConfigData::value will take precedence over ConfigData::defaultValue, if both are set, as the value that will be used for the default value. */
    Variant DeclareSetting(const ConfigData &data);
    Variant DeclareSetting(const ConfigData &data, const String &key, const Variant &defaultValue); /**< @overload */

private:
    friend class Framework;

    /// @note Framework takes ownership of the object.
    explicit ConfigAPI(Framework *framework);

    /// Get absolute file path for file. Guarantees that it ends with .ini.
    String GetFilePath(const String &file) const;

    /// Returns if file provided by the ConfigAPI caller is secure and we should write/read from it.
    /** The purpose of this function is to verify the file provided by calling code
        does not go out of the confined ConfigAPI folder. For security reasons we cannot let
        eg. scripts open configs where they like. The whole operation will be canceled if this validation fails. */
    bool IsFilePathSecure(const String &file) const;

    /// Prepare string for config usage. Removes spaces from end and start, replaces mid string spaces with '_' and forces to lower case.
    void PrepareString(String &str) const;

    /// Opens up the Config API to the given data folder. This call will make sure that the required folders exist.
    /** @param configFolderName The name of the folder to store Tundra Config API data to. */
    void PrepareDataFolder(String configFolderName);

    Framework *owner;
    String configFolder_; ///< Absolute path to the folder where to store the config files.
    mutable HashMap<String, ConfigFile> configFiles_; // Configuration file in-memory data.
};

}
