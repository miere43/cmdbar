#include <assert.h>

#include "parse_ini.h"
#include "parse_utils.h"

INIParser::INIParser()
{
}

void INIParser::init(String source)
{
    this->source = source;
    this->currentLine = 0;
    this->sourceIndex = 0;
}

bool INIParser::next()
{
    if (sourceIndex >= source.count)
        return false;
    if (source.data == nullptr)
        return false;

    String currSource = source.substring(sourceIndex);
    String line;
    int lineBreakLength;

    ParseUtils::getLine(currSource, &line, &lineBreakLength);
    sourceIndex += line.count + lineBreakLength;

    line = line.trimmed();

    if (line.isEmpty())
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
        this->group = line.substring(1, line.count - 2);
    }
    else
    {
        int sepIndex = line.indexOf(L'=');
        if (sepIndex == -1)
        {
            // No value present for key-value pair.
            this->type = INIValueType::Error;
            return true;
        }

        this->type = INIValueType::KeyValuePair;
        this->key = line.substring(0, sepIndex).trimmed();
        this->value = line.substring(sepIndex + 1).trimmed();
    }

    return true;
}
