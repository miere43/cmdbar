#pragma once
#include "newstring.h"

struct ParseUtils
{
    static void GetLine(const Newstring& str, Newstring* lineSubstr, int* lineBreakLength);
    static bool StringToBool(const Newstring& str, bool* value);
};