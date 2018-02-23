#pragma once
#include "string_type.h"
#include "newstring.h"

enum class INIValueType
{
    None,
    KeyValuePair,
    Group,
    Error
};

struct INIParser
{
    INIValueType type;
    union
    {
        struct
        {
            Newstring key;
            Newstring value;
        };
        struct
        {
            Newstring group;
        };
    };

    INIParser();

    Newstring source;
    uint32_t sourceIndex;
    uint32_t currentLine;

    void Initialize(Newstring source);
    bool Next();
};