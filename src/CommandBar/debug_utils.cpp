#include "debug_utils.h"

#include <Windows.h>
#include <stdarg.h>
#include "newstring_builder.h"


void DebugWriteLine(const wchar_t* format, ...)
{
    assert(false); // @TODO: Not implemented
    
    // This code fails with some weird error:


    //va_list args;
    //va_start(args, format);

    //NewstringBuilder sb;
    //sb.allocator = &g_tempAllocator;
    //sb.Append(Newstring::FormatWithAllocator(&g_tempAllocator, format, args, false));
    //sb.Reserve(sb.count + 3);
    //sb.Append(L'\n');
    //sb.ZeroTerminate();

    //va_end(args);

    //OutputDebugStringW(sb.data);
}
