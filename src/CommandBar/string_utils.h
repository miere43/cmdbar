#pragma once
#include "string_type.h"


struct StringUtils
{
    static bool insertChars(
        wchar_t* target, 
        int targetCount,
        int targetMaxCount, 
        const wchar_t* strToInsert,
        int strToInsertCount,
        int insertPos);

    static bool parseInt(const String& text, int* result, int defaultValue);
};