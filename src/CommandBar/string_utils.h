#pragma once
#include "newstring.h"

struct StringUtils
{
    static bool ParseInt32(const Newstring& string, int* result, int defaultValue);
};