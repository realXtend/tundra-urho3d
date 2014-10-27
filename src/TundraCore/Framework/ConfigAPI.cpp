// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Framework.h"
#include "ConfigAPI.h"
#include "LoggingFunctions.h"

#include <Context.h>
#include <File.h>
#include <FileSystem.h>

using namespace Urho3D;

namespace Tundra
{

String ConfigAPI::FILE_FRAMEWORK        = "tundra";
String ConfigAPI::SECTION_FRAMEWORK     = "framework";
String ConfigAPI::SECTION_SERVER        = "server";
String ConfigAPI::SECTION_CLIENT        = "client";
String ConfigAPI::SECTION_RENDERING     = "rendering";
String ConfigAPI::SECTION_GRAPHICS      = "graphics";
String ConfigAPI::SECTION_UI            = "ui";
String ConfigAPI::SECTION_SOUND         = "sound";

/* Prepare string for config usage. Removes spaces from end and start, replaces mid string 
   spaces with '_' and forces to lower case. Do not call this function with a config value.
   @note This is an implementation detail, the user of ConfigAPI does not need to worry about getting the strings right.
   @param str String to prepare. */
void PrepareString(String &str)
{
    str = str.Trimmed().ToLower(); // Remove spaces from start/end, force to lower case
    if (str.Empty())
        return;
    str = str.Replaced(" ", "_");  // Replace ' ', so we don't get %20 in the config as spaces
    str = str.Replaced("/", "_");  // Replace '/', as / has a special meaning in .ini file keys/sections. Also file name cannot have a forward slash.
    str = str.Replaced("=", "_");  // Replace '=', as = has special meaning in .ini files
    str = str.Replaced("[", "(");  // Replace '[' and ']' as they have special meaning for a section.
    str = str.Replaced("]", ")");  
}

VariantType GetVariantTypeFromString(String &value)
{
    /* Empty values are ok, but if not prefixed with
       type information it is an invalid config entry.
       Type info is at minimum "@1" */
    if (value.Length() < 2 || value[0] != '@')
        return VAR_NONE;

    // "@<VAR_TYPE_AS_INT> <value>
    int valueLength = (!IsDigit(value[2]) ? 1 : 2);
    int type = ToInt(value.Substring(1, valueLength));
    value = value.Substring(1+valueLength).Trimmed();
    return (type > VAR_NONE && type < MAX_VAR_TYPES ? static_cast<VariantType>(type) : VAR_NONE);
}

// ConfigFile

bool ConfigFile::HasKey(String section, String key) const
{
    PrepareString(section);
    PrepareString(key);
    if (section.Empty() || key.Empty())
        return false;

    HashMap<String, ConfigSection>::ConstIterator iter = sections_.Find(section);
    if (iter != sections_.End())
        return iter->second_.keys.Contains(key);
    return false;
}

void ConfigFile::Set(String section, String key, const Variant &value)
{
    PrepareString(section);
    PrepareString(key);
    if (section.Empty() || key.Empty())
        return;

    sections_[section].keys[key] = value;
    modified_ = true;
}

void ConfigFile::Set(String section, const HashMap<String, Variant> &values)
{
    PrepareString(section);
    if (section.Empty() || values.Empty())
        return;
    
    ConfigSection &s = sections_[section];
    for(HashMap<String, Variant>::ConstIterator iter = values.Begin(); iter != values.End(); ++iter)
    {
        String key = iter->first_;
        PrepareString(key);
        s.keys[key] = iter->second_;
    }
    modified_ = true;
}

Variant ConfigFile::Get(String section, String key, const Variant &defaultValue)
{
    // Intentionally not using HasKey to reduce making copies.
    PrepareString(section);
    PrepareString(key);
    if (section.Empty() || key.Empty())
        return false;

    HashMap<String, ConfigSection>::ConstIterator iter = sections_.Find(section);
    if (iter != sections_.End())
        return sections_[section].keys[key];
    return defaultValue;
}

void ConfigFile::Load(Context* ctx, const String& fileName)
{
    // Already loaded to memory from disk?
    if (loaded_)
        return;
    loaded_ = true;

    if (!ctx->GetSubsystem<FileSystem>()->FileExists(fileName))
        return;
    SharedPtr<File> file(new File(ctx, fileName, FILE_READ));
    if (!file->IsOpen())
        return;

    String currentSection;
    while (!file->IsEof())
    {
        String line = file->ReadLine().Trimmed();
        if (line.Length() == 0)
            continue;

        if (line[0] == ('['))
            currentSection = line.Substring(1, line.Length() - 2);
        else
        {
            // Find first '='. It is allowed characted in value of eg. the String type.
            uint splitIndex = line.Find('=');
            if (splitIndex != String::NPOS)
            {
                // Trim the parts if human has edited the config width indentation etc.
                String key = line.Substring(0, splitIndex).Trimmed();
                String value = line.Substring(splitIndex+1).Trimmed();
                // Will remove the type information from value.
                VariantType type = GetVariantTypeFromString(value);

                if (type > VAR_NONE && type < MAX_VAR_TYPES)
                    sections_[currentSection].keys[key].FromString(type, value);
                else
                    LogError(Urho3D::ToString("ConfigAPI: Failed to determine value type for '%s' in section '%s' of '%s'.",
                        value.CString(), currentSection.CString(), fileName.CString()));
            }
            else
                LogError(Urho3D::ToString("ConfigAPI: Invalid line '%s' in section '%s' of '%s'.",
                    line.CString(), currentSection.CString(), fileName.CString()));
        }
    }
}

void ConfigFile::Save(Context* ctx, const String& fileName)
{
    // No in memory changes done. Skip saving to disk.
    if (!modified_)
        return;

    SharedPtr<File> file(new File(ctx, fileName, FILE_WRITE));
    if (!file->IsOpen())
        return;

    for (HashMap<String, ConfigSection>::ConstIterator i = sections_.Begin(); i != sections_.End(); ++i)
    {
        String sectionStr = "[" + i->first_ + "]";
        file->WriteLine(sectionStr);
        for (HashMap<String, Variant>::ConstIterator j = i->second_.keys.Begin(); j != i->second_.keys.End(); ++j)
        {
            String value = j->second_.ToString();
            file->WriteLine(Urho3D::ToString("%s=@%d %s", j->first_ .CString(), static_cast<int>(j->second_.GetType()), value.CString()));
        }
        file->WriteLine("");
    }
    modified_ = false;
}

// ConfigAPI

ConfigAPI::ConfigAPI(Framework *framework) :
    Object(framework->GetContext()),
    owner(framework)
{
}

ConfigAPI::~ConfigAPI()
{
    for(HashMap<String, ConfigFile>::Iterator iter = configFiles_.Begin(); iter != configFiles_.End(); ++iter)
        iter->second_.Save(GetContext(), GetFilePath(iter->first_));
}

void ConfigAPI::PrepareDataFolder(String configFolder)
{
    FileSystem* fs = GetSubsystem<FileSystem>();
    String configPath = owner->ParseWildCardFilename(configFolder.Trimmed());
    
    if (!fs->DirExists(configPath))
    {
        bool success = fs->CreateDir(configPath);
        if (!success)
        {
            LogError("Failed to create configuration folder \"" + configPath + "\"! Check that this path is valid, and it is write-accessible!");
            return;
        }
    }
    configFolder_ = AddTrailingSlash(configPath);
}

String ConfigAPI::GetFilePath(const String &file) const
{
    if (configFolder_.Empty())
    {
        LogError("ConfigAPI::GetFilePath: Config folder has not been prepared, returning empty string.");
        return "";
    }

    String filePath = configFolder_ + file;
    if (!filePath.EndsWith(".ini"))
        filePath += ".ini";
    return filePath;
}

bool ConfigAPI::IsFilePathSecure(const String &file) const
{
    if (file.Trimmed().Empty())
    {
        LogError("ConfigAPI: File path to perform read/write operations is not permitted as it's an empty string.");
        return false;
    }

    bool secure = true;
    if (IsAbsolutePath(file))
        secure = false;
    else if (file.Contains(".."))
        secure = false;
    if (!secure)
        LogError("ConfigAPI: File path to perform read/write operations is not permitted: " + file);
    return secure;
}

bool ConfigAPI::HasKey(const ConfigData &data) const
{
    return HasKey(data.file, data.section, data.key);
}

bool ConfigAPI::HasKey(const ConfigData &data, String key) const
{
    return HasKey(data.file, data.section, key);
}

bool ConfigAPI::HasKey(String file, const String &section, const String &key) const
{
    if (configFolder_.Empty())
    {
        LogError("ConfigAPI::HasKey: Config folder has not been prepared.");
        return false;
    }
    if (section.Empty())
    {
        LogError("ConfigAPI::HasKey: Section is empty for " + file);
        return false;
    }
    if (key.Empty())
    {
        LogError("ConfigAPI::HasKey: Key is empty for section '" + section + "' in " + file);
        return false;
    }

    PrepareString(file);
    if (!IsFilePathSecure(file))
        return false;
    
    ConfigFile &f = configFiles_[file];
    f.Load(GetContext(), GetFilePath(file)); // No-op after loading once
    return f.HasKey(section, key);
}

Variant ConfigAPI::Read(const ConfigData &data) const
{
    return Read(data.file, data.section, data.key, data.defaultValue);
}

Variant ConfigAPI::Read(const ConfigData &data, const String &key, const Variant &defaultValue) const
{
    if (defaultValue.IsEmpty())
        return Read(data.file, data.section, key, data.defaultValue);
    else
        return Read(data.file, data.section, key, defaultValue);
}

Variant ConfigAPI::Read(String file, const String &section, const String &key, const Variant &defaultValue) const
{
    if (configFolder_.Empty())
    {
        LogError("ConfigAPI::Read: Config folder has not been prepared, returning null Variant.");
        return Variant::EMPTY;
    }
    
    PrepareString(file);
    if (!IsFilePathSecure(file))
        return Variant::EMPTY;
    if (section.Empty())
    {
        LogError("ConfigAPI::Read: Section not defined for " + file + ", returning null Variant.");
        return Variant::EMPTY;
    }

    ConfigFile &f = configFiles_[file];
    f.Load(GetContext(), GetFilePath(file)); // No-op after loading once
    return f.Get(section, key, defaultValue);
}

bool ConfigAPI::Write(const ConfigData &data)
{
    return Write(data.file, data.section, data.key, data.value);
}

bool ConfigAPI::Write(const ConfigData &data, const Variant &value)
{
    return Write(data.file, data.section, data.key, value);
}

bool ConfigAPI::Write(const ConfigData &data, const String &key, const Variant &value)
{
    return Write(data.file, data.section, key, value);
}

bool ConfigAPI::Write(const String &file, const String &section, const String &key, const Variant &value)
{
    return WriteInternal(file, section, key, value, true);
}

bool ConfigAPI::Write(const String &file, const String &section, const HashMap<String, Variant> &values)
{
    return WriteInternal(file, section, values, true);
}

bool ConfigAPI::WriteInternal(String file, const String &section, const String &key, const Variant &value, bool writeToDisk)
{
    if (configFolder_.Empty())
    {
        LogError("ConfigAPI::Write: Config folder has not been prepared, can not write value to config.");
        return false;
    }
    PrepareString(file);
    if (!IsFilePathSecure(file))
        return false;
    if (section.Empty())
    {
        LogError("ConfigAPI::Write: Section is empty for " + file);
        return false;
    }
    if (key.Empty())
    {
        LogError("ConfigAPI::Write: Value is empty for section '" + section + "' in " + file);
        return false;
    }

    ConfigFile &f = configFiles_[file];
    String filepath = GetFilePath(file);
    /* No-op after loading once. We must load first to ensure disk
       stored values won't later overwrite the now set value. */
    f.Load(GetContext(), filepath); 
    f.Set(section, key, value);
    if (writeToDisk)
        f.Save(GetContext(), filepath);
    return true;
}

bool ConfigAPI::WriteInternal(String file, const String &section, const HashMap<String, Variant> &values, bool writeToDisk)
{
    if (configFolder_.Empty())
    {
        LogError("ConfigAPI::Write: Config folder has not been prepared, can not write value to config.");
        return false;
    }
    PrepareString(file);
    if (!IsFilePathSecure(file))
        return false;
    if (section.Empty())
    {
        LogError("ConfigAPI::Write: Section is empty for section '" + section + "' in " + file);
        return false;
    }

    ConfigFile &f = configFiles_[file];
    String filepath = GetFilePath(file);
    /* No-op after loading once. We must load first to ensure disk
       stored values won't later overwrite the now set value. */
    f.Load(GetContext(), filepath); 
    
    for (HashMap<String, Variant>::ConstIterator iter = values.Begin(); iter != values.End(); ++iter)
    {
        String key = iter->first_;
        if (!key.Empty())
            f.Set(section, key, iter->second_);
    }
    if (writeToDisk)
        f.Save(GetContext(), filepath);
    return true;
}

ConfigFile &ConfigAPI::GetFile(String file)
{
    PrepareString(file);
    ConfigFile &f = configFiles_[file];
    /* No-op after loading once. We must load first to ensure disk
       stored values won't later overwrite the now set value. */
    f.Load(GetContext(), GetFilePath(file));
    return f;
}

void ConfigAPI::WriteFile(String file)
{
    PrepareString(file);
    ConfigFile &f = configFiles_[file];
    /* No-op after loading once. We must load first to ensure disk
       stored values won't later overwrite the now set value. */
    f.Load(GetContext(), GetFilePath(file));
    // No-op if ConfigFile::Set has not been invoked since last Save.
    f.Save(GetContext(), GetFilePath(file));
}

Variant ConfigAPI::DeclareSetting(const String &file, const String &section, const String &key, const Variant &defaultValue)
{
    if (HasKey(file, section, key))
    {
        return Read(file, section, key);
    }
    else
    {
        Write(file, section, key, defaultValue);
        return defaultValue;
    }
}

Variant ConfigAPI::DeclareSetting(const ConfigData &data)
{
    return DeclareSetting(data.file, data.section, data.key, !data.value.IsEmpty() ? data.value : data.defaultValue);
}

Variant ConfigAPI::DeclareSetting(const ConfigData &data, const String &key, const Variant &defaultValue)
{
    return DeclareSetting(data.file, data.section, key, defaultValue);
}

}
