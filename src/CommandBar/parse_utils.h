#pragma once
#include "string_type.h"
#include "newstring.h"

struct ParseUtils
{
    static void GetLine(const Newstring& str, Newstring* lineSubstr, int* lineBreakLength);
    static bool StringToBool(const Newstring& str, bool* value);

    static bool stringToBool(const String& str, bool* value); // @Deprecated
};