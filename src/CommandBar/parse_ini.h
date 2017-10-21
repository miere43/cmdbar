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
    int sourceIndex;
    
    int currentLine;
    void init(String source);

    bool next();
};