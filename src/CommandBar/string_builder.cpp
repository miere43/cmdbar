#include <assert.h>
#include <stdarg.h>

#include "string_builder.h"
#include "unicode.h"


void StringBuilder::reserve(uint32_t newCapacity)
{
    assert(newCapacity >= 0);

    if (str.count + capacity >= newCapacity)
        return;

    const size_t newSize = sizeof(wchar_t) * newCapacity;

    wchar_t* newData = (wchar_t*)allocator->Allocate(newSize);
    assert(newData != nullptr);

    if (str.data != nullptr && str.count > 0)
        wmemcpy(newData, str.data, str.count);

    allocator->Deallocate(str.data);
    str.data = newData;
    capacity = newCapacity;
}

void StringBuilder::appendChar(wchar_t c)
{
    reserve2(str.count + 1);
    str.data[str.count++] = c;
}

void StringBuilder::appendString(const wchar_t* newstr, uint32_t count)
{
    if (newstr == nullptr)
    {
        newstr = L"(null)";
        count = sizeof(L"(null)") / sizeof(wchar_t) - 1;
    }
    
    reserve2(str.count + count);
    wmemcpy(&str.data[str.count], newstr, count);
    str.count += count;
}

void StringBuilder::appendString(const wchar_t* newstr)
{
    appendString(newstr, newstr == nullptr ? 0 : static_cast<int>(wcslen(newstr)));
}

void StringBuilder::appendString(const String& str)
{
    appendString(str.data, str.count);
}

void StringBuilder::appendFormat(const wchar_t* format, ...)
{
    assert(format != nullptr);

    va_list arg;
    va_start(arg, format);

    int expected = _vscwprintf(format, arg);
    assert(expected != -1);

    reserve2(str.count + expected + 1);
    int actual = vswprintf_s(str.data, capacity, format, arg);
    assert(actual != -1);
    assert(actual == expected);

    str.count += actual;

    va_end(arg);
}

void StringBuilder::appendChar(uint32_t codepoint)
{
    if (unicode::isSurrogatePair(codepoint))
    {
        reserve2(str.count + 2);
        
        *(reinterpret_cast<uint32_t*>(str.data + str.count)) = codepoint;
        str.count += 2;
    }
    else
    {
        reserve2(str.count + 1);
        str.data[str.count++] = static_cast<wchar_t>(codepoint & 0x0000FFFF);
    }
}

void StringBuilder::nullTerminate()
{
    reserve2(str.count + 1);
    str.data[str.count] = L'\0';
}

void StringBuilder::reserve2(int newCapacity)
{
    if (newCapacity <= 32)
        reserve(32);
    else
        if (newCapacity < capacity * 2)
            reserve(capacity * 2);
        else
            reserve(newCapacity);
}
