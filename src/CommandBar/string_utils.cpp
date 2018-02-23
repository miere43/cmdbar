#include <assert.h>

#include "string_utils.h"


bool StringUtils::parseInt(const String& text, int* result, int defaultValue)
{
    if (result == nullptr)
        return false;

    if (text.isEmpty() || 1 != _snwscanf_s(text.data, text.count, L"%i", result))
    {
        *result = defaultValue;
        return false;
    }
    
    return true;
}

bool StringUtils::ParseInt32(const Newstring& string, int* result, int defaultValue)
{
    assert(result);

    if (Newstring::IsNullOrEmpty(string) || 1 != _snwscanf_s(string.data, string.count, L"%i", result))
    {
        *result = defaultValue;
        return false;
    }

    return true;
}
