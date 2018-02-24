#include <assert.h>

#include "string_utils.h"


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
