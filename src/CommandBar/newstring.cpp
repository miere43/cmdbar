#include "newstring.h"
#include <wchar.h>
#include <stdarg.h>

Newstring::Newstring()
{ }

Newstring::Newstring(wchar_t* data, uint32_t count)
	: data(data)
	, count(count)
{ }

bool Newstring::operator==(const Newstring& rhs) const
{
    return Equals(rhs, StringComparison::CaseSensitive);
}

bool Newstring::operator!=(const Newstring& rhs) const
{
    return !operator==(rhs);
}

bool Newstring::operator==(const wchar_t* rhs) const
{
    return operator==(Newstring::WrapConstWChar(rhs));
}

bool Newstring::operator!=(const wchar_t* rhs) const
{
    return !operator==(rhs);
}

bool Newstring::Equals(const Newstring& rhs, StringComparison comparison) const
{
    if (IsNullOrEmpty(this) && IsNullOrEmpty(rhs))  return true;

    auto compareProcedure = comparison == StringComparison::CaseSensitive ? wcsncmp : _wcsnicmp;

    return count == rhs.count && compareProcedure(data, rhs.data, count) == 0;
}

bool Newstring::Equals(const wchar_t* rhs, StringComparison comparison) const
{
    return Equals(Newstring::WrapConstWChar(rhs), comparison);
}

int Newstring::IndexOf(wchar_t c) const
{
    if (IsNullOrEmpty(this))  return -1;

    for (uint32_t i = 0; i < count; ++i)
    {
        if (data[i] == c)
            return i;
    }

    return -1;
}

int Newstring::LastIndexOf(wchar_t c) const
{
    if (IsNullOrEmpty(this))  return -1;

    for (uint32_t i = count - 1; i > 0; --i)
    {
        if (data[i] == c)
            return i;
    }

    return -1;
}

bool Newstring::StartsWith(const Newstring& string, StringComparison comparison) const
{
    if (IsNullOrEmpty(string))
        return false;

    if (string.count > this->count)
        return false;

    auto compareProc = comparison == StringComparison::CaseSensitive ? wcsncmp : _wcsnicmp;

    return compareProc(this->data, string.data, string.count) == 0;
}

uint32_t Newstring::CopyTo(Newstring* dest, uint32_t fromIndex, uint32_t destIndex, uint32_t copyCount) const
{
    if (IsNullOrEmpty(this))  return 0;
    if (IsNullOrEmpty(dest))  return 0;

    if (fromIndex >= this->count)  return 0;
    if (destIndex >= dest->count)  return 0;

    if (copyCount == 0)  return 0;

    wchar_t* const thisData = &this->data[fromIndex];
    wchar_t* const destData = &dest->data[destIndex];

    if (dest->count - destIndex > copyCount)
        copyCount = dest->count - destIndex;
    if (this->count - fromIndex > copyCount)
        copyCount = this->count - fromIndex;
    if (copyCount > this->count)
        copyCount = this->count;

    wmemcpy_s(destData, dest->count - destIndex, thisData, copyCount);
    return copyCount;
}

Newstring Newstring::RefSubstring(uint32_t index, uint32_t count) const
{
    if (IsNullOrEmpty(this))  return Empty();

    if (index > this->count - 1)
        index = this->count - 1;
    if (count > this->count) 
        count = this->count - index;

    Newstring result;
    result.data  = this->data + index;
    result.count = count;

    return result;
}

Newstring Newstring::Clone(IAllocator* allocator) const
{
    assert(allocator);
    return Newstring::Clone(this, allocator);
}

wchar_t* Newstring::CloneAsCString(IAllocator* allocator) const
{
    assert(allocator);

    if (!data)  return nullptr;

    wchar_t* cstring = (wchar_t*)allocator->Allocate(sizeof(wchar_t) * (count + 1));
    if (!cstring)  return nullptr;
    
    if (count > 0)
    {
        wmemcpy(cstring, data, count);
    }
    cstring[count] = L'\0';

    return cstring;
}

Newstring Newstring::Trimmed() const
{
    if (IsNullOrEmpty(this))  return Empty();

    uint32_t i;
    for (i = 0; i < count; ++i)
        if (!(data[i] == L' ' || data[i] == L'\t'))
            break;

    uint32_t j;
    for (j = count; j > i; --j)
        if (!(data[j - 1] == L' ' || data[j - 1] == L'\t'))
            break;

    return Newstring(data + i, j);
}

Newstring Newstring::TrimmedRight() const
{
    uint32_t j;
    for (j = count; j >= 0; --j)
        if (!(data[j - 1] == L' ' || data[j - 1] == L'\t'))
            break;

    return Newstring(data, j);
}

void Newstring::Dispose(IAllocator* allocator)
{
    assert(allocator);
    allocator->Deallocate(data);
    data  = nullptr;
    count = 0;
}

Newstring Newstring::SkipChar(wchar_t c) const
{
    if (IsNullOrEmpty(this))
        return *this;

    wchar_t* result = data;
    uint32_t i = 0;
    
    for (; i < count; ++i) {
        if (*result == c) {
            ++result;
        } else {
            break;
        }
    }

    return Newstring(result, count - i);
}

bool Newstring::IsNullOrEmpty(const Newstring& string)
{
    return IsNullOrEmpty(&string);
}

bool Newstring::IsNullOrEmpty(const Newstring* const string)
{
    return string == nullptr || string->data == nullptr || string->count == 0;
}

Newstring Newstring::New(uint32_t count, IAllocator* allocator)
{
    assert(allocator);
    if (count == 0)  return Empty();

    wchar_t* data = (wchar_t*)allocator->Allocate(count * sizeof(wchar_t));
    if (!data)  return Empty();

    data[0] = L'\0';

    return Newstring(data, count);
}

Newstring Newstring::NewFromWChar(const wchar_t* string, IAllocator* allocator)
{
    assert(allocator);
    return Newstring::Clone(WrapConstWChar(string), allocator);
}

Newstring Newstring::NewFromWChar(const wchar_t* string, uint32_t count, IAllocator* allocator)
{
    assert(allocator);
    return Newstring::Clone(Newstring::WrapConstWChar(string, count), allocator);
}

wchar_t* Newstring::NewCStringFromWChar(const wchar_t* string, IAllocator* allocator) noexcept
{
    assert(allocator);
    if (!string)  return nullptr;

    uint32_t count = (uint32_t)wcslen(string);
    wchar_t* result = (wchar_t*)allocator->Allocate(sizeof(wchar_t) * (count + 1));
    if (!result)  return nullptr;

    wmemcpy(result, string, count + 1);

    return result;
}

Newstring Newstring::Clone(const Newstring& string, IAllocator* allocator)
{
    return Newstring::Clone(&string, allocator);
}

Newstring Newstring::Clone(const Newstring* string, IAllocator* allocator)
{
    assert(allocator);
    if (IsNullOrEmpty(string))  return Empty();

    Newstring ns = New(string->count, allocator);
    if (IsNullOrEmpty(ns))  return Empty();

    string->CopyTo(&ns);

    return ns;
}

Newstring Newstring::Join(std::initializer_list<Newstring> strings, IAllocator* allocator)
{
    assert(allocator);
    
    uint32_t totalCount = 0;
    for (auto string : strings)
        totalCount += string.count;

    if (totalCount == 0)  return Empty();

    Newstring ns = New(totalCount, allocator);
    if (IsNullOrEmpty(ns))  return Empty();

    uint32_t index = 0;
    for (auto string : strings)
    {
        index += string.CopyTo(&ns, 0, index);
    }

    return ns;
}

Newstring Newstring::Format(const wchar_t* format, ...)
{
    assert(format);

    va_list args;
    va_start(args, format);

    Newstring result = FormatWithAllocator(&g_standardAllocator, format, args, false);

    va_end(args);

    return result;
}

wchar_t* Newstring::FormatCString(const wchar_t* format, ...)
{
    assert(format);

    va_list args;
    va_start(args, format);

    Newstring result = FormatWithAllocator(&g_standardAllocator, format, args, true);

    va_end(args);

    return result.data;
}

Newstring Newstring::FormatTemp(const wchar_t* format, ...)
{
    assert(format);

    va_list args;
    va_start(args, format);

    Newstring result = FormatWithAllocator(&g_tempAllocator, format, args, false);

    va_end(args);

    return result;
}

wchar_t* Newstring::FormatTempCString(const wchar_t* format, ...)
{
    assert(format);

    va_list args;
    va_start(args, format);

    Newstring result = FormatWithAllocator(&g_tempAllocator, format, args, true);

    va_end(args);

    return result.data;
}

Newstring Newstring::FormatWithAllocator(IAllocator* allocator, const wchar_t* format, ...)
{
    assert(allocator);
    assert(format);

    va_list args;
    va_start(args, format);

    Newstring result = FormatWithAllocator(allocator, format, args, false);

    va_end(args);

    return result;
}

Newstring Newstring::FormatWithAllocator(IAllocator* allocator, const wchar_t* format, va_list args, bool includeTerminatingNull)
{
    assert(allocator);
    assert(format);

    va_list argsCopy;
    va_copy(argsCopy, args);

    // Number of characters required to format specified string, **without terminating null**.
    int charCount = _vscwprintf(format, argsCopy);
    assert(charCount != -1);

    va_end(argsCopy);

    int allocCount = charCount + (includeTerminatingNull ? 1 : 0);

    Newstring result = New(allocCount, allocator);
    if (IsNullOrEmpty(result))  return Empty();

    int written = _vsnwprintf(result.data, allocCount, format, args);  // @TODO: Check out why _vsnwprintf_s doesn't work (has 'buffer too small' error)!
    assert(written == charCount);

    result.count = written;
    if (result.count - 1 == '\0')
        __debugbreak();
    return result;
}

Newstring Newstring::WrapWChar(wchar_t* string)
{
    if (!string)  return Empty();

    return WrapWChar(string, (uint32_t)wcslen(string));
}

Newstring Newstring::WrapWChar(wchar_t* string, uint32_t count)
{
    if (!string || count == 0)  return Empty();

    return Newstring(string, count);
}

const Newstring Newstring::WrapConstWChar(const wchar_t* string)
{
    if (!string)  return Empty();

    return WrapConstWChar(string, (uint32_t)wcslen(string));
}

const Newstring Newstring::WrapConstWChar(const wchar_t* string, uint32_t count)
{
    if (!string || count == 0)  return Empty();

    return Newstring((wchar_t*)string, count);
}

Newstring Newstring::Empty()
{
    return Newstring(nullptr, 0);
}
