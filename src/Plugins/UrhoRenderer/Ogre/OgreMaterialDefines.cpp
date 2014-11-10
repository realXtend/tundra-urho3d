// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "OgreMaterialDefines.h"
#include "CoreStringUtils.h"
#include "LoggingFunctions.h"

#include <Engine/Core/StringUtils.h>

namespace Tundra
{

namespace Ogre
{

// MaterialBlock

MaterialBlock::MaterialBlock(MaterialBlock *parent_, MaterialPart part_, int t, int p, int tu) : 
    parent(parent_),
    part(part_),
    technique(t),
    pass(p),
    textureUnit(tu)
{
}

MaterialBlock::~MaterialBlock()
{
    for(uint i=0; i<blocks.Size(); ++i)
        delete blocks[i];
    blocks.Clear();
}

bool MaterialBlock::IsSupported()
{
    return (part != MP_Unsupported);
}

uint MaterialBlock::NumChildren(MaterialPart part) const
{
    uint num = 0;
    foreach(const MaterialBlock *block, blocks)
    {
        if (block->part == part)
            num++;
    }
    return num;
}

uint MaterialBlock::NumTechniques() const
{
    return NumChildren(MP_Technique);
}

uint MaterialBlock::NumPasses() const
{
    return NumChildren(MP_Pass);
}

uint MaterialBlock::NumTextureUnits() const
{
    return NumChildren(MP_TextureUnit);
}

MaterialBlock *MaterialBlock::Technique(uint index) const
{
    foreach(MaterialBlock *block, blocks)
    {
        if (block->part == MP_Technique && block->technique == static_cast<int>(index))
            return block;
    }
    return nullptr;
}

MaterialBlock *MaterialBlock::Pass(uint index) const
{
    foreach(MaterialBlock *block, blocks)
    {
        if (block->part == MP_Pass && block->pass == static_cast<int>(index))
            return block;
    }
    return nullptr;
}

MaterialBlock *MaterialBlock::TextureUnit(uint index) const
{
    foreach(MaterialBlock *block, blocks)
    {
        if (block->part == MP_TextureUnit && block->textureUnit == static_cast<int>(index))
            return block;
    }
    return nullptr;
}

MaterialBlock *MaterialBlock::VertexProgram() const
{
    foreach(MaterialBlock *block, blocks)
    {
        if (block->part == MP_VertexProgram)
            return block;
    }
    return nullptr;
}

MaterialBlock *MaterialBlock::FragmentProgram() const
{
    foreach(MaterialBlock *block, blocks)
    {
        if (block->part == MP_FragmentProgram)
            return block;
    }
    return nullptr;
}

uint MaterialBlock::Num(const StringHash &name) const
{
    MaterialProperties::ConstIterator iter = properties.Find(name);
    if (iter != properties.End())
        return iter->second_.Size();
    return 0;
}

bool MaterialBlock::Has(const StringHash &name) const
{
    return (Num(name) > 0);
}

String MaterialBlock::StringValue(const StringHash &name, const String &defaultValue, uint index) const
{
    MaterialProperties::ConstIterator iter = properties.Find(name);
    if (iter != properties.End() && index < iter->second_.Size())
        return iter->second_[index];
    return defaultValue;
}

StringVector MaterialBlock::StringVectorValue(const StringHash &name, uint index) const
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

Urho3D::Vector2 MaterialBlock::Vector2Value(const StringHash &name, const Urho3D::Vector2 &defaultValue, uint index) const
{
    StringVector parts = StringVectorValue(name, index);
    if (parts.Size() >= 2)
        return Urho3D::Vector2(Urho3D::ToFloat(parts[0]), Urho3D::ToFloat(parts[1]));
    return defaultValue;
}

Urho3D::Vector3 MaterialBlock::Vector3Value(const StringHash &name, const Urho3D::Vector3 &defaultValue, uint index) const
{
    StringVector parts = StringVectorValue(name, index);
    if (parts.Size() >= 3)
        return Urho3D::Vector3(Urho3D::ToFloat(parts[0]), Urho3D::ToFloat(parts[1]), Urho3D::ToFloat(parts[2]));
    return defaultValue;
}

Urho3D::Vector4 MaterialBlock::Vector4Value(const StringHash &name, const Urho3D::Vector4 &defaultValue, uint index) const
{
    StringVector parts = StringVectorValue(name, index);
    if (parts.Size() >= 4)
        return Urho3D::Vector4(Urho3D::ToFloat(parts[0]), Urho3D::ToFloat(parts[1]), Urho3D::ToFloat(parts[2]), Urho3D::ToFloat(parts[3]));
    return defaultValue;
}

Urho3D::Color MaterialBlock::ColorValue(const StringHash &name, const Urho3D::Color &defaultValue, uint index) const
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

bool MaterialBlock::BooleanValue(const StringHash &name, bool defaultValue, uint index) const
{
    String value = StringValue(name, "", index).Trimmed();
    if (value.Compare("on", false) == 0 || value.Compare("enabled", false) == 0 || value.Compare("true", false) == 0 || value.Compare("1", false) == 0)
        return true;
    else if (value.Compare("off", false) == 0 || value.Compare("disabled", false) == 0 || value.Compare("false", false) == 0 || value.Compare("0", false) == 0)
        return false;
    return defaultValue;
}

void MaterialBlock::Dump(bool recursive, uint indentation)
{
    String ind = "";
    while(ind.Length() < indentation) ind += " ";

    LogInfoF("%s%s '%s'", ind.CString(), MaterialPartToString(part).CString(), id.CString());

    indentation += 2;
    while(ind.Length() < indentation) ind += " ";

    for (auto iter = properties.Begin(); iter != properties.End(); ++iter)
    {
        const StringVector &values = iter->second_;
        if (!values.Empty())
        {
            for (int vi=0; vi<values.Size(); ++vi)
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

// MaterialParser

MaterialParser::MaterialParser() :
    root(0)
{
}

MaterialParser::~MaterialParser()
{
    SAFE_DELETE(root);
}

String MaterialParser::Error() const
{
    return state.error;
}

bool MaterialParser::Parse(const char *data_, uint lenght_)
{
    data = Urho3D::String(data_, lenght_);
    pos = 0;
    lineNum = 0;

    SAFE_DELETE(root);

    for(;;)
    {
        if (!ProcessLine())
            break;
    }

    // Make root safe even on failure
    if (!root)
        root = new MaterialBlock();

    return (state.error.Length() == 0);
}

void MaterialParser::Advance()
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

void MaterialParser::SkipBlock()
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

bool IsBlockIndentifier(const StringHash &hash)
{
    return (hash == Material::Block::Material
         || hash == Material::Block::Technique
         || hash == Material::Block::Pass
         || hash == Material::Block::TextureUnit
         || hash == Material::Block::VertexProgram
         || hash == Material::Block::FragmentProgram
         || hash == Material::Block::DefaultParameters);
}

bool MaterialParser::ProcessLine()
{
    // Read next line
    Advance();

    // No more lines?
    if (pos == String::NPOS)
        return false;
    else if (line.Empty())
        return true;

    /*if (lineNum < 10)
        PrintRaw(" " + String(lineNum) + " '" + line + "'\n");
    else
        PrintRaw(String(lineNum) + " '" + line + "'\n");*/

    // Filter comments. Spec only allows single line comments.
    if (line.Length() > 1 && line[0] == '/' && line[1] == '/')
        return true;

    // Block scope end
    if (line[0] == '}')
    {
        // Store parsed block to parent
        if (state.block->parent)
            state.block->parent->blocks.Push(state.block);

        // If traversed back to root we are done.
        /// @todo If we want to parse multiple materials from a single file change this logic.
        state.block = state.block->parent;
        return (state.block != 0);
    }
    // Block scope start
    else if (line[0] == '{')
    {
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
    
    // Is this a new block scope identifier?
    // Note that this w
    if (IsBlockIndentifier(key))
    {
        MaterialPart part = MP_Unsupported;

        // Detect block type
        if (key == Material::Block::Material)
        {
            /// @todo http://www.ogre3d.org/docs/manual/manual_25.html#Script-Inheritance
            part = MP_Material;
            state.technique = -1;
            state.pass = -1;
            state.textureUnit = -1;
        }
        else if (key == Material::Block::Technique)
        {
            part = MP_Technique;
            state.technique++;
            state.pass = -1;
            state.textureUnit = -1;
        }
        else if (key == Material::Block::Pass)
        {
            part = MP_Pass;
            state.pass++;
            state.textureUnit = -1;
        }
        else if (key == Material::Block::TextureUnit)
        {
            part = MP_TextureUnit;
            state.textureUnit++;
        }
        else if (key == Material::Block::VertexProgram)
            part = MP_VertexProgram;
        else if (key == Material::Block::FragmentProgram)
            part = MP_FragmentProgram;
        else if (key == Material::Block::DefaultParameters)
            part = MP_DefaultParameters;

        state.block = new MaterialBlock(state.block, part, state.technique, state.pass, state.textureUnit);
        state.block->id = value;

        if (!root && part == MP_Material)
            root = state.block;

        //LogInfoF("    tech %d pass %d tu %d", state.block->technique, state.block->pass, state.block->textureUnit);
        return true;
    }
    else if (splitPos == String::NPOS)
    {
        state.error = Urho3D::ToString("Ogre::MaterialParser: Invalid script tokens '%s' on line %d before column %d", line.CString(), lineNum, pos);
        return false;
    }

    // Add property to current block
    state.block->propertyNames[key] = keyStr;
    state.block->properties[key].Push(value);
    return true;
}

}

}
