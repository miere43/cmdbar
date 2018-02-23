#pragma once
#include "string_type.h"
#include "newstring.h"

struct StringUtils
{
    static bool parseInt(const String& text, int* result, int defaultValue);

    static bool ParseInt32(const Newstring& string, int* result, int defaultValue);
};