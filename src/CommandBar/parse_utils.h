#pragma once
#include "string_type.h"


struct ParseUtils
{
    static void getLine(const String& str, String* lineSubstr, int* lineBreakLength);
    static bool stringToBool(const String& str, bool* value);
};