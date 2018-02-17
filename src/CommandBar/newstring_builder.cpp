#include "newstring_builder.h"

NewstringBuilder::NewstringBuilder()
    : capacity(0)
    , allocator(&g_standardAllocator)
{
    data  = nullptr;
    count = 0;
}

void NewstringBuilder::Dispose()
{
    assert(allocator);
    allocator->dealloc(data);
    data = nullptr;
    count = 0;
    capacity = 0;
}

void NewstringBuilder::Append(wchar_t c)
{
    if (Reserve(capacity + 1))
    {
        data[count++] = c;
    }
}

void NewstringBuilder::Append(const wchar_t* string)
{
    return Append(Newstring::WrapConstWChar(string));
}

void NewstringBuilder::Append(const wchar_t* string, uint32_t count)
{
    return Append(Newstring::WrapConstWChar(string, count));
}

void NewstringBuilder::Append(const Newstring& string)
{
    static Newstring Null = Newstring::WrapWChar(L"(null)");

    if (string.data == nullptr)
    {
        Append(Null);
        return;
    }

    if (string.count == 0)  return;
    if (Reserve(capacity + string.count))
    {
        uint32_t prevCount = count;
        count = capacity;
        count = prevCount + string.CopyTo(&this->string, 0, prevCount);
    }
}

void NewstringBuilder::ZeroTerminate()
{
    if (!data)  return;
    if (count > 0 && data[count - 1] == L'\0')  return;

    if (Reserve(capacity + 1))
    {
        data[count++] = L'\0';
    }
    else
    {   
        assert(false);
        if (count > 0) data[count - 1] = L'\0';
    }
}

uint32_t NewstringBuilder::GetRemainingCapacity() const
{
    if (!data || capacity == 0)  return 0;
    return capacity - count;
}

bool NewstringBuilder::Reserve(uint32_t newCapacity)
{
    assert(allocator);

    if (newCapacity < 32) newCapacity = 32;
    if (data && capacity >= newCapacity)  return true;

    wchar_t* newData = (wchar_t*)allocator->realloc(data, newCapacity);
    if (!newData)  return false;

    data = newData;
    capacity = newCapacity;

    return true;
}

Newstring NewstringBuilder::TransferToString()
{
    Newstring ns = string;
    string.data = nullptr;
    string.count = 0;
    capacity = 0;
    return ns;
}

Newstring& NewstringBuilder::GetStringRef()
{
    return string;
}
