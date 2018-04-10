#include <assert.h>

#include "parse_ini.h"
#include "parse_utils.h"

INIParser::INIParser()
{
}

INIParser::INIParser(Newstring source)
{
    Initialize(source);
}

void INIParser::Initialize(Newstring source)
{
    this->source = source;
    this->currentLine = 0;
    this->sourceIndex = 0;
}

bool INIParser::Next()
{
    if (sourceIndex >= source.count)
        return false;
    if (source.data == nullptr)
        return false;

    Newstring currSource = source.RefSubstring(sourceIndex);
    Newstring line;
    int lineBreakLength;

    ParseUtils::GetLine(currSource, &line, &lineBreakLength);
    sourceIndex += line.count + lineBreakLength;

    line = line.Trimmed();

    if (Newstring::IsNullOrEmpty(line))
    {
        this->type = INIValueType::None;
    }
    else if (line.data[0] == L'[')
    {
        if (line.count < 2)
        {
            // Group closing paren is missing.
            this->type = INIValueType::Error;
            return true;
        }

        this->type = INIValueType::Group;
        this->group = line.RefSubstring(1, line.count - 2);
    }
    else
    {
        int sepIndex = line.IndexOf(L'=');
        if (sepIndex == -1)
        {
            // No value present for key-value pair.
            this->type = INIValueType::Error;
            return true;
        }

        this->type  = INIValueType::KeyValuePair;
        this->key   = line.RefSubstring(0,  sepIndex).Trimmed();
        this->value = line.RefSubstring(sepIndex + 1).Trimmed();
    }

    return true;
}
