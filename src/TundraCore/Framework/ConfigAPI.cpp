// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Framework.h"
#include "ConfigAPI.h"

#include <Context.h>
#include <File.h>
#include <FileSystem.h>
#include <Log.h>

using namespace Urho3D;

namespace Tundra
{

String ConfigAPI::FILE_FRAMEWORK = "tundra";
String ConfigAPI::SECTION_FRAMEWORK = "framework";
String ConfigAPI::SECTION_SERVER = "server";
String ConfigAPI::SECTION_CLIENT = "client";
String ConfigAPI::SECTION_RENDERING = "rendering";
String ConfigAPI::SECTION_UI = "ui";
String ConfigAPI::SECTION_SOUND = "sound";

VariantType GetVariantTypeFromString(const String& in)
{
    if (in == "true" || in == "false")
        return VAR_BOOL;
    else if (in.Length() && IsDigit(in[0]))
    {
        if (in.Contains('.'))
            return VAR_FLOAT;
        else
            return VAR_INT;
    }    
    else
        return VAR_STRING;

}

void ConfigFile::Load(Context* ctx, const String& fileName)
{
    if (loaded)
        return; // Already loaded

    loaded = true;
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
                sections[currentSection].keys[parts[0]].FromString(GetVariantTypeFromString(parts[1]), parts[1]);
        }
    }
}

void ConfigFile::Save(Context* ctx, const String& fileName)
{
    SharedPtr<File> file(new File(ctx, fileName, FILE_WRITE));
    if (!file->IsOpen())
        return;    

    for (HashMap<String, ConfigSection>::ConstIterator i = sections.Begin(); i != sections.End(); ++i)
    {
        String sectionStr = "[" + i->first_ + "]";
        file->WriteLine(sectionStr);
        for (HashMap<String, Variant>::ConstIterator j = i->second_.keys.Begin(); j != i->second_.keys.End(); ++j)
            file->WriteLine(j->first_ + "=" + j->second_.ToString());
        file->WriteLine("");
    }
}

ConfigAPI::ConfigAPI(Framework *framework) :
    Object(framework->GetContext()),
    owner(framework)
{
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
            LOGERROR("Failed to create configuration folder \"" + configPath + "\"! Check that this path is valid, and it is write-accessible!");
            return;
        }
    }
    configFolder_ = AddTrailingSlash(configPath);
}

String ConfigAPI::GetFilePath(const String &file) const
{
    if (configFolder_.Empty())
    {
        LOGERROR("ConfigAPI::GetFilePath: Config folder has not been prepared, returning empty string.");
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
        LOGERROR("ConfigAPI: File path to perform read/write operations is not permitted as it's an empty string.");
        return false;
    }

    bool secure = true;
    if (IsAbsolutePath(file))
        secure = false;
    else if (file.Contains(".."))
        secure = false;
    if (!secure)
        LOGERROR("ConfigAPI: File path to perform read/write operations is not permitted: " + file);
    return secure;
}

void ConfigAPI::PrepareString(String &str) const
{
    if (!str.Empty())
    {
        str = str.Trimmed().ToLower();  // Remove spaces from start/end, force to lower case
        str = str.Replaced(" ", "_");    // Replace ' ' with '_', so we don't get %20 in the config as spaces
        str = str.Replaced("=", "_");    // Replace '=' with '_', as = has special meaning in .ini files
        str = str.Replaced("/", "_");    // Replace '/' with '_', as / has a special meaning in .ini file keys/sections. Also file name cannot have a forward slash.
    }
}

bool ConfigAPI::HasKey(const ConfigData &data) const
{
    if (data.file.Empty() || data.section.Empty() || data.key.Empty())
    {
        LOGWARNING("ConfigAPI::HasKey: ConfigData does not have enough information (file, section, and key).");
        return false;
    }
    return HasKey(data.file, data.section, data.key);
}

bool ConfigAPI::HasKey(const ConfigData &data, String key) const
{
    if (data.file.Empty() || data.section.Empty())
    {
        LOGWARNING("ConfigAPI::HasKey: ConfigData does not have enough information (file, and section).");
        return false;
    }
    return HasKey(data.file, data.section, key);
}

bool ConfigAPI::HasKey(String file, String section, String key) const
{
    if (configFolder_.Empty())
    {
        LOGERROR("ConfigAPI::HasKey: Config folder has not been prepared, returning false.");
        return false;
    }

    PrepareString(file);
    PrepareString(section);
    PrepareString(key);

    if (!IsFilePathSecure(file))
        return false;
        
    configFiles_[file].Load(GetContext(), GetFilePath(file)); // No-op after loading once
    if (configFiles_[file].sections.Contains(section))
        return configFiles_[file].sections[section].keys.Contains(key);
    else
        return false;
}

Variant ConfigAPI::Read(const ConfigData &data) const
{
    if (data.file.Empty() || data.section.Empty() || data.key.Empty())
    {
        LOGWARNING("ConfigAPI::Read: ConfigData does not have enough information (file, section, and key).");
        return data.defaultValue;
    }
    return Read(data.file, data.section, data.key, data.defaultValue);
}

Variant ConfigAPI::Read(const ConfigData &data, String key, const Variant &defaultValue) const
{
    if (data.file.Empty() || data.section.Empty())
    {
        LOGWARNING("ConfigAPI::Read: ConfigData does not have enough information (file and section).");
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
        LOGERROR("ConfigAPI::Read: Config folder has not been prepared, returning null Variant.");
        return "";
    }

    PrepareString(file);
    PrepareString(section);
    PrepareString(key);

    // Don't return 'defaultValue' but null Variant
    // as this is an error situation.
    if (!IsFilePathSecure(file))
        return Variant();

    configFiles_[file].Load(GetContext(), GetFilePath(file)); // No-op after loading once
    if (configFiles_[file].sections.Contains(section))
    {
        if (configFiles_[file].sections[section].keys.Contains(key))
            return configFiles_[file].sections[section].keys[key];
    }

    return defaultValue;
}

void ConfigAPI::Write(const ConfigData &data)
{
    if (data.file.Empty() || data.section.Empty() || data.key.Empty() || data.value.IsEmpty())
    {
        LOGWARNING("ConfigAPI::Write: ConfigData does not have enough information (file, section, key, and value).");
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
        LOGWARNING("ConfigAPI::Write: ConfigData does not have enough information (file and section).");
        return;
    }
    Write(data.file, data.section, key, value);
}

void ConfigAPI::Write(String file, String section, String key, const Variant &value)
{
    if (configFolder_.Empty())
    {
        LOGERROR("ConfigAPI::Write: Config folder has not been prepared, can not write value to config.");
        return;
    }

    PrepareString(file);
    PrepareString(section);
    PrepareString(key);

    if (!IsFilePathSecure(file))
        return;

    configFiles_[file].sections[section].keys[key] = value;
    configFiles_[file].Save(GetContext(), GetFilePath(file));
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
