// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "OgreParticleSystemDefines.h"
#include "CoreStringUtils.h"
#include "LoggingFunctions.h"

#include <Engine/Core/StringUtils.h>

namespace Tundra
{

namespace Ogre
{

// ParticleSystemBlock

ParticleSystemBlock::ParticleSystemBlock(ParticleSystemBlock *parent_, ParticleSystemPart part_, int e, int a) : 
    parent(parent_),
    part(part_),
    emitter(e),
    affector(a)
{
}

ParticleSystemBlock::~ParticleSystemBlock()
{
    for(uint i=0; i<blocks.Size(); ++i)
        delete blocks[i];
    blocks.Clear();
}

bool ParticleSystemBlock::IsSupported()
{
    return (part != PSP_Unsupported);
}

uint ParticleSystemBlock::NumChildren(ParticleSystemPart part) const
{
    uint num = 0;
    foreach(const ParticleSystemBlock *block, blocks)
    {
        if (block->part == part)
            num++;
    }
    return num;
}

uint ParticleSystemBlock::NumEmitters() const
{
    return NumChildren(PSP_Emitter);
}

ParticleSystemBlock *ParticleSystemBlock::Emitter(uint index) const
{
    foreach(ParticleSystemBlock *block, blocks)
    {
        if (block->part == PSP_Emitter && block->emitter == static_cast<int>(index))
            return block;
    }
    return nullptr;
}

uint ParticleSystemBlock::NumAffectors() const
{
    return NumChildren(PSP_Affector);
}

ParticleSystemBlock *ParticleSystemBlock::Affector(uint index) const
{
    foreach(ParticleSystemBlock *block, blocks)
    {
        if (block->part == PSP_Affector && block->affector == static_cast<int>(index))
            return block;
    }
    return nullptr;
}

uint ParticleSystemBlock::Num(const StringHash &name) const
{
    ParticleSystemProperties::ConstIterator iter = properties.Find(name);
    if (iter != properties.End())
        return iter->second_.Size();
    return 0;
}

bool ParticleSystemBlock::Has(const StringHash &name) const
{
    return (Num(name) > 0);
}

String ParticleSystemBlock::StringValue(const StringHash &name, const String &defaultValue, uint index) const
{
    ParticleSystemProperties::ConstIterator iter = properties.Find(name);
    if (iter != properties.End() && index < iter->second_.Size())
        return iter->second_[index];
    return defaultValue;
}

StringVector ParticleSystemBlock::StringVectorValue(const StringHash &name, uint index) const
{
    String value = StringValue(name, "", index).Trimmed();
    StringVector parts = (!value.Empty() ? value.Split(' ') : StringVector());
    // Remove empty parts to correctly parse "1.0   1.0  1.0 1.0" type of values
    for(auto iter = parts.Begin(); iter != parts.End();)
    {
        *iter = iter->Trimmed();
        if (iter->Empty())
            iter = parts.Erase(iter);
        else
            iter++;
    }
    return parts;
}

Urho3D::Vector2 ParticleSystemBlock::Vector2Value(const StringHash &name, const Urho3D::Vector2 &defaultValue, uint index) const
{
    StringVector parts = StringVectorValue(name, index);
    if (parts.Size() >= 2)
        return Urho3D::Vector2(Urho3D::ToFloat(parts[0]), Urho3D::ToFloat(parts[1]));
    return defaultValue;
}

Urho3D::Vector3 ParticleSystemBlock::Vector3Value(const StringHash &name, const Urho3D::Vector3 &defaultValue, uint index) const
{
    StringVector parts = StringVectorValue(name, index);
    if (parts.Size() >= 3)
        return Urho3D::Vector3(Urho3D::ToFloat(parts[0]), Urho3D::ToFloat(parts[1]), Urho3D::ToFloat(parts[2]));
    return defaultValue;
}

Urho3D::Vector4 ParticleSystemBlock::Vector4Value(const StringHash &name, const Urho3D::Vector4 &defaultValue, uint index) const
{
    StringVector parts = StringVectorValue(name, index);
    if (parts.Size() >= 4)
        return Urho3D::Vector4(Urho3D::ToFloat(parts[0]), Urho3D::ToFloat(parts[1]), Urho3D::ToFloat(parts[2]), Urho3D::ToFloat(parts[3]));
    return defaultValue;
}

Urho3D::Color ParticleSystemBlock::ColorValue(const StringHash &name, const Urho3D::Color &defaultValue, uint index) const
{
    StringVector parts = StringVectorValue(name, index);
    if (parts.Size() >= 3)
    {
        Urho3D::Color color(Urho3D::ToFloat(parts[0]), Urho3D::ToFloat(parts[1]), Urho3D::ToFloat(parts[2]));
        if (parts.Size() >= 4)
            color.a_ = Urho3D::ToFloat(parts[3]);
        return color;
    }
    return defaultValue;
}

bool ParticleSystemBlock::BooleanValue(const StringHash &name, bool defaultValue, uint index) const
{
    String value = StringValue(name, "", index).Trimmed();
    if (value.Compare("on", false) == 0 || value.Compare("enabled", false) == 0 || value.Compare("true", false) == 0 || value.Compare("1", false) == 0)
        return true;
    else if (value.Compare("off", false) == 0 || value.Compare("disabled", false) == 0 || value.Compare("false", false) == 0 || value.Compare("0", false) == 0)
        return false;
    return defaultValue;
}

int ParticleSystemBlock::IntValue(const StringHash &name, int defaultValue, uint index) const
{
    String value = StringValue(name, "", index).Trimmed();
    return value.Length() > 0 ? ToInt(value) : defaultValue;
}

float ParticleSystemBlock::FloatValue(const StringHash &name, float defaultValue, uint index) const
{
    String value = StringValue(name, "", index).Trimmed();
    return value.Length() > 0 ? ToFloat(value) : defaultValue;
}

void ParticleSystemBlock::Dump(bool recursive, uint indentation)
{
    String ind = "";
    while(ind.Length() < indentation) ind += " ";

    LogInfoF("%s%s '%s'", ind.CString(), ParticleSystemPartToString(part).CString(), id.CString());

    indentation += 2;
    while(ind.Length() < indentation) ind += " ";

    for (auto iter = properties.Begin(); iter != properties.End(); ++iter)
    {
        const StringVector &values = iter->second_;
        if (!values.Empty())
        {
            for (uint vi=0; vi<values.Size(); ++vi)
            {
                if (vi == 0)
                    LogInfoF("%s%s '%s'", ind.CString(), PadString(propertyNames[iter->first_], 20).CString(), values[vi].CString());
                else
                    LogInfoF("%s%s '%s'", ind.CString(), propertyNames[iter->first_].CString(), values[vi].CString());
            }
        }
    }

    if (recursive)
    {
        for (auto iter = blocks.Begin(); iter != blocks.End(); ++iter)
            (*iter)->Dump(recursive, indentation);
    }

    indentation -= 2;
    if (indentation == 0)
        LogInfo("");
}

// ParticleSystemParser

ParticleSystemParser::ParticleSystemParser() :
    root(nullptr)
{
}

ParticleSystemParser::~ParticleSystemParser()
{
    foreach(ParticleSystemBlock* tmp, templates)
        SAFE_DELETE(tmp);

    templates.Clear();
}

String ParticleSystemParser::Error() const
{
    return state.error;
}

bool ParticleSystemParser::Parse(const char *data_, uint lenght_)
{
    data = String(data_, lenght_);
    pos = 0;
    lineNum = 0;

    foreach(ParticleSystemBlock* tmp, templates)
        SAFE_DELETE(tmp);
    templates.Clear();
    root = nullptr;

    for(;;)
    {
        if (!ProcessLine())
            break;
    }

    return (state.error.Length() == 0);
}

void ParticleSystemParser::Advance()
{
    uint endPos = data.Find('\n', pos);
    if (endPos == String::NPOS || endPos < pos)
    {
        pos = String::NPOS;
        return;
    }
    // Empty line with only \n at the start
    else if (endPos == pos)
    {
        pos += 1;
        line = "";
        return;
    }
    line = data.Substring(pos, endPos - pos - (data[endPos-1] == '\r' ? 1 : 0)).Trimmed();
    pos = endPos + 1;
    lineNum++;
}

void ParticleSystemParser::SkipBlock()
{
    uint endPos = data.Find('}', pos);
    if (endPos == String::NPOS || endPos < pos)
    {
        state.error = Urho3D::ToString("Failed to find Block scope end '}', started looking from index %d", pos);
        pos = String::NPOS;
        return;
    }
    // There is a newline after found '}' advance over it.
    pos = endPos + 1;
    Advance();
}

namespace
{
bool IsBlockIndentifier(const StringHash &hash)
{
    return (hash == ParticleSystem::Block::ParticleSystem
         || hash == ParticleSystem::Block::Emitter
         || hash == ParticleSystem::Block::Affector
         || hash == ParticleSystem::Block::DefaultParameters);
}
}

bool ParticleSystemParser::ProcessLine()
{
    // Read next line
    Advance();

    // No more lines?
    if (pos == String::NPOS)
        return false;
    else if (line.Empty())
        return true;

    //if (lineNum < 10)
    //    PrintRaw(" " + String(lineNum) + " '" + line + "'\n");
    //else
    //    PrintRaw(String(lineNum) + " '" + line + "'\n");

    // Filter comments. Spec only allows single line comments.
    if (line.Length() > 1 && line[0] == '/' && line[1] == '/')
        return true;

    // Block scope end
    if (line[0] == '}')
    {
        // Material not yet started
        if (!state.block)
            return true;

        // Store parsed block to parent
        if (state.block->parent)
            state.block->parent->blocks.Push(state.block);

        state.block = state.block->parent;
        return true;
    }
    // Block scope start
    else if (line[0] == '{')
    {
        // Material not yet started
        if (!state.block)
            return true;

        // Skip invalid blocks
        if (!state.block || !state.block->IsSupported())
            SkipBlock();
        return true;
    }

    // Split to "<key> <value>" from the first space.
    // Note that value can contain spaces, it is stored as is.
    uint splitPos = line.Find(' ');
    String keyStr = (splitPos == String::NPOS ? line : line.Substring(0, splitPos).ToLower());
    StringHash key(keyStr);
    String value = (splitPos == String::NPOS ? "" : line.Substring(splitPos+1));
    
    // Do not begin default_params block if material not started yet
    if (key == ParticleSystem::Block::DefaultParameters && !state.block)
        return true;

    // Is this a new block scope identifier?
    if (IsBlockIndentifier(key))
    {
        ParticleSystemPart part = PSP_Unsupported;

        // Detect block type
        if (key == ParticleSystem::Block::ParticleSystem)
        {
            part = PSP_ParticleSystem;
            state.emitter = -1;
        }
        else if (key == ParticleSystem::Block::Emitter)
        {
            part = PSP_Emitter;
            state.emitter++;
        }
        else if (key == ParticleSystem::Block::Affector)
        {
            part = PSP_Affector;
            state.affector++;
        }
        else if (key == ParticleSystem::Block::DefaultParameters)
            part = PSP_DefaultParameters;

        state.block = new ParticleSystemBlock(state.block, part, state.emitter, state.affector);
        state.block->id = value;

        if (part == PSP_ParticleSystem)
        {
            root = state.block;
            templates.Push(root);
        }

        //LogInfoF("    emitter %d affector %d", state.block->emitter, state.block->affector);
        return true;
    }
    else if (splitPos == String::NPOS)
    {
        state.error = Urho3D::ToString("Ogre::ParticleSystemParser: Invalid script tokens '%s' on line %d before column %d", line.CString(), lineNum, pos);
        return false;
    }
    // Material not yet started
    if (!state.block)
        return true;

    // Add property to current block
    state.block->propertyNames[key] = keyStr;
    state.block->properties[key].Push(value);
    return true;
}

}

}
