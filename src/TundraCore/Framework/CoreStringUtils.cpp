// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "CoreStringUtils.h"
#include "Math/MathFunc.h"

#include <kNet/DataSerializer.h>
#include <kNet/DataDeserializer.h>

#include <StringUtils.h>

namespace Tundra
{

String ReadUtf8String(kNet::DataDeserializer &dd)
{
    String ret;
    ret.Resize(dd.Read<u16>());
    if (ret.Length())
        dd.ReadArray<u8>((u8*)&ret[0], ret.Length());
    return ret;
}

void WriteUtf8String(kNet::DataSerializer& ds, const String& str)
{
    ds.Add<u16>(static_cast<u16>(str.Length()));
    if (str.Length())
        ds.AddArray<u8>((const u8*)&str[0], str.Length());
}

void ParseCommand(String input, String &command, StringVector &parameters)
{
    input = input.Trimmed();
    if (input.Empty())
        return;

    uint parenPos = input.Find('(', 0);
    uint spacePos = input.Find(' ', 0);
    if (parenPos == String::NPOS && spacePos == String::NPOS)
    {
        command = input;
        return;
    }
    StringVector parts;
    if (parenPos != String::NPOS && parenPos < spacePos)
    {
        uint parenEndPos = input.FindLast(')');
        String insideParens = (parenEndPos != String::NPOS ? 
            input.Substring(parenPos+1, parenEndPos-parenPos-1).Trimmed() : input.Substring(parenPos+1).Trimmed());
        command = input.Substring(0, parenPos).Trimmed();
        // "one, two, three", "one,two,three" and "one two three"
        parts = insideParens.Contains(',') ? insideParens.Split(',') : insideParens.Split(' ');
    }
    else
    {
        command = input.Substring(0, spacePos).Trimmed();
        String remaining = input.Substring(spacePos+1).Trimmed();
        // "one, two, three", "one,two,three" and "one two three"
        parts = remaining.Contains(',') ? remaining.Split(',') : remaining.Split(' ');
    }
    for(StringVector::Iterator iter=parts.Begin(); iter!=parts.End(); ++iter)
    {
        String part = (*iter).Trimmed();
        if (part.EndsWith(","))
            part = part.Substring(0, part.Length()-1);
        if (!part.Empty())
            parameters.Push(part);
    }
}

String PadString(String str, int pad)
{
    if (pad == 0)
        return str;
    int orig = pad;
    uint to = Abs(pad);
    pad = Abs(pad);
    while (pad > 0 && str.Length() < to)
    {
        if (orig < 0)
            str = " " + str;
        else
            str += " ";
        pad--;
    }
    return str;
}

String FormatDigitGrouping(uint value, const String &fillChar)
{
    String str;
    str.AppendWithFormat("%u", value);
    if (str.Length() >= 9)
    {
        str.Insert(6, fillChar);
        str.Insert(3, fillChar);
    }
    else if (str.Length() >= 8)
    {
        str.Insert(5, fillChar);
        str.Insert(2, fillChar);
    }
    else if (str.Length() >= 7)
    {
        str.Insert(4, fillChar);
        str.Insert(1, fillChar);
    }
    else if (str.Length() >= 6)
        str.Insert(3, fillChar);
    else if (str.Length() >= 5)
        str.Insert(2, fillChar);
    else if (str.Length() >= 4)
        str.Insert(1, fillChar);
    return str;
}

String Join(const StringVector &list, const String &separator)
{
    String joined = "";
    for (unsigned int i=0 ; i<list.Size() ; ++i)
    {
        if (i != 0)
            joined += separator;
        joined += list[i];
    }

    return joined;
}

}
