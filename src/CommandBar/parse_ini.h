#pragma once
#include "string_type.h"


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
            String key;
            String value;
        };
        struct
        {
            String group;
        };
    };

    INIParser();

    String source;
    uint32_t sourceIndex;
    
    uint32_t currentLine;
    void init(String source);

    bool next();
};