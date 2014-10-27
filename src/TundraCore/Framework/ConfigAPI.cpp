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

/* Prepare string for config usage. Removes spaces from end and start, replaces mid string spaces with '_' and forces to lower case.
   @param str String to prepare.
   @param isKey If true "[" and "]" are replaces by "(" and ")".
   They are not allowed in keys as the parsing code will mistake them as sections */
void PrepareString(String &str, bool isKey = false)
{
    if (!str.Empty())
    {
        str = str.Trimmed().ToLower();   // Remove spaces from start/end, force to lower case
        str = str.Replaced(" ", "_");    // Replace ' ' with '_', so we don't get %20 in the config as spaces
        str = str.Replaced("=", "_");    // Replace '=' with '_', as = has special meaning in .ini files
        str = str.Replaced("/", "_");    // Replace '/' with '_', as / has a special meaning in .ini file keys/sections. Also file name cannot have a forward slash.
        if (isKey)
        {
            // Keys cannot contain the "[" or "]" tokens so they wont be mistaken as sections.
            str = str.Replaced("[", "(");
            str = str.Replaced("]", ")");
        }
    }
}

VariantType GetVariantTypeFromString(const String& in)
{
    if (in == "true" || in == "false")
        return VAR_BOOL;
    else if ((in.Length() && IsDigit(in[0])) || (in.Length() >= 2 && in[0] == '-' && IsDigit(in[1])))
    {
        uint num = in.Trimmed().Split(' ').Size();
        if (in.Contains('.'))
            return (num <= 1 ? VAR_FLOAT : (num == 2 ? VAR_VECTOR2 : (num == 3 ? VAR_VECTOR3 : VAR_VECTOR4)));
        else
            return (num <= 1 ? VAR_INT : VAR_INTVECTOR2);
    }    
    else
        return VAR_STRING;
}

// ConfigFile

bool ConfigFile::HasKey(const String &section, const String &key) const
{
    HashMap<String, ConfigSection>::ConstIterator iter = sections_.Find(section);
    if (iter != sections_.End())
        return iter->second_.keys.Contains(key);
    return false;
}

void ConfigFile::Set(const String &section, const String &key, const Variant &value)
{
    if (section.Empty() || key.Empty())
        return;

    sections_[section].keys[key] = value;
    modified_ = true;
}

void ConfigFile::Set(const String &section, const HashMap<String, Variant> &values)
{
    if (section.Empty() || values.Empty())
        return;

    ConfigSection &s = sections_[section];
    for(HashMap<String, Variant>::ConstIterator iter = values.Begin(); iter != values.End(); ++iter)
    {
        String key = iter->first_;
        PrepareString(key, true);
        s.keys[key] = iter->second_;
    }
    modified_ = true;
}

Variant ConfigFile::Get(const String &section, const String &key, const Variant &defaultValue)
{
    if (HasKey(section, key))
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
        if (line.Length() && line[0] == ('['))
            currentSection = line.Substring(1, line.Length() - 2);
        else
        {
            Vector<String> parts = line.Split('=');
            if (parts.Size() == 2)
                sections_[currentSection].keys[parts[0]].FromString(GetVariantTypeFromString(parts[1]), parts[1]);
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
            file->WriteLine(j->first_ + "=" + j->second_.ToString());
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
    if (data.file.Empty() || data.section.Empty() || data.key.Empty())
    {
        LogWarning("ConfigAPI::HasKey: ConfigData does not have enough information (file, section, and key).");
        return false;
    }
    return HasKey(data.file, data.section, data.key);
}

bool ConfigAPI::HasKey(const ConfigData &data, String key) const
{
    if (data.file.Empty() || data.section.Empty())
    {
        LogWarning("ConfigAPI::HasKey: ConfigData does not have enough information (file, and section).");
        return false;
    }
    return HasKey(data.file, data.section, key);
}

bool ConfigAPI::HasKey(String file, String section, String key) const
{
    if (configFolder_.Empty())
    {
        LogError("ConfigAPI::HasKey: Config folder has not been prepared, returning false.");
        return false;
    }

    PrepareString(file);
    PrepareString(section);
    PrepareString(key, true);

    if (!IsFilePathSecure(file))
        return false;
    
    ConfigFile &f = configFiles_[file];
    f.Load(GetContext(), GetFilePath(file)); // No-op after loading once
    return f.HasKey(section, key);
}

Variant ConfigAPI::Read(const ConfigData &data) const
{
    if (data.file.Empty() || data.section.Empty() || data.key.Empty())
    {
        LogWarning("ConfigAPI::Read: ConfigData does not have enough information (file, section, and key).");
        return data.defaultValue;
    }
    return Read(data.file, data.section, data.key, data.defaultValue);
}

Variant ConfigAPI::Read(const ConfigData &data, String key, const Variant &defaultValue) const
{
    if (data.file.Empty() || data.section.Empty())
    {
        LogWarning("ConfigAPI::Read: ConfigData does not have enough information (file and section).");
        return data.defaultValue;
    }
    if (defaultValue.IsEmpty())
        return Read(data.file, data.section, key, data.defaultValue);
    else
        return Read(data.file, data.section, key, defaultValue);
}

Variant ConfigAPI::Read(String file, String section, String key, const Variant &defaultValue) const
{
    if (configFolder_.Empty())
    {
        LogError("ConfigAPI::Read: Config folder has not been prepared, returning null Variant.");
        return "";
    }

    PrepareString(file);
    PrepareString(section);
    PrepareString(key, true);

    // Don't return 'defaultValue' but null Variant
    // as this is an error situation.
    if (!IsFilePathSecure(file))
        return Variant();

    ConfigFile &f = configFiles_[file];
    f.Load(GetContext(), GetFilePath(file)); // No-op after loading once
    return f.Get(section, key, defaultValue);
}

void ConfigAPI::Write(const ConfigData &data)
{
    if (data.file.Empty() || data.section.Empty() || data.key.Empty() || data.value.IsEmpty())
    {
        LogWarning("ConfigAPI::Write: ConfigData does not have enough information (file, section, key, and value).");
        return;
    }
    Write(data.file, data.section, data.key, data.value);
}

void ConfigAPI::Write(const ConfigData &data, const Variant &value)
{
    Write(data, data.key, value);
}

void ConfigAPI::Write(const ConfigData &data, String key, const Variant &value)
{
    if (data.file.Empty() || data.section.Empty())
    {
        LogWarning("ConfigAPI::Write: ConfigData does not have enough information (file and section).");
        return;
    }
    Write(data.file, data.section, key, value);
}

void ConfigAPI::Write(String file, String section, String key, const Variant &value)
{
    if (configFolder_.Empty())
    {
        LogError("ConfigAPI::Write: Config folder has not been prepared, can not write value to config.");
        return;
    }

    PrepareString(file);
    PrepareString(section);
    PrepareString(key, true);

    if (!IsFilePathSecure(file))
        return;

    WriteInternal(file, section, key, value, true);
}

void ConfigAPI::Write(String file, String section, HashMap<String, Variant> values)
{
    if (values.Empty())
        return;
    if (configFolder_.Empty())
    {
        LogError("ConfigAPI::Write: Config folder has not been prepared, can not write value to config.");
        return;
    }

    PrepareString(file);
    PrepareString(section);

    if (!IsFilePathSecure(file))
        return;

    ConfigFile &f = configFiles_[file];
    /* No-op after loading once. We must load first to ensure disk
       stored values won't later overwrite the now set value. */
    f.Load(GetContext(), GetFilePath(file)); 
    f.Set(section, values);
    f.Save(GetContext(), GetFilePath(file));
}

void ConfigAPI::WriteInternal(const String &file, const String &section, const String &key, const Variant &value, bool writeToDisk)
{
    if (file.Empty() || section.Empty() || key.Empty())
        return;

    ConfigFile &f = configFiles_[file];
    /* No-op after loading once. We must load first to ensure disk
       stored values won't later overwrite the now set value. */
    f.Load(GetContext(), GetFilePath(file)); 

    f.Set(section, key, value);
    if (writeToDisk)
        f.Save(GetContext(), GetFilePath(file));
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
