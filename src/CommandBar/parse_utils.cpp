#include <assert.h>


#include "array.h"
#include "parse_utils.h"


void ParseUtils::getLine(const String& str, String* lineSubstr, int* lineBreakLength)
{
    assert(lineSubstr);
    assert(lineBreakLength);

    if (!str.isEmpty())
    {
        for (int i = 0; i < str.count; ++i)
        {
            wchar_t c = str.data[i];

            if (c == '\r' && i + 1 < str.count && str.data[i + 1] == '\n')
            {
                *lineSubstr = String{ str.data, i };
                *lineBreakLength = 2;
                return;
            }
            else if (c == '\n')
            {
                *lineSubstr = String{ str.data, i };
                *lineBreakLength = 1;
                return;
            }
        }
    }

    *lineSubstr = String{ str.data, str.count };
    *lineBreakLength = 0;
}

bool ParseUtils::stringToBool(const String& str, bool* value)
{
    assert(value);
    if (str.isEmpty()) return false;

    switch (str.count)
    {
        case 1:
            switch (towlower(str.data[0]))
            {
                case L'0': goto setFalse;
                case L'1': goto setTrue;
                case L'n': goto setFalse;
                case L'y': goto setTrue;
            }
            break;
        case 2: if (0 == wcsncmp(str.data, L"no", 2)) goto setFalse;
        case 3: if (0 == wcsncmp(str.data, L"yes", 3)) goto setTrue;
        case 4: if (0 == wcsncmp(str.data, L"true", 4)) goto setTrue;
        case 5: if (0 == wcsncmp(str.data, L"false", 5)) goto setFalse;
    }

    return false;
setTrue:
    *value = true;
    return true;
setFalse:
    *value = false;
    return true;
}
