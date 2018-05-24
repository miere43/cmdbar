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
    allocator->Deallocate(data);
    data = nullptr;
    count = 0;
    capacity = 0;
}

void NewstringBuilder::Append(wchar_t c)
{
    if (Reserve(count + 1))
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
    if (Reserve(count + string.count))
    {
        uint32_t prevCount = count;
        count = count + string.count;
        count = prevCount + string.CopyTo(&this->string, 0, prevCount);
    }
}

void NewstringBuilder::ZeroTerminate()
{
    if (!data)  return;
    if (count > 0 && data[count - 1] == L'\0')  return;

    if (Reserve(count + 1))
    {
        data[count++] = L'\0';
    }
    else
    {   
        assert(false);
        if (count > 0) data[count - 1] = L'\0';
    }
}

void NewstringBuilder::Insert(uint32_t pos, const Newstring& string)
{
    // @TODO(Critical): Fix crash when pos is out of range.

    if (Newstring::IsNullOrEmpty(string))  return;
    if (!Reserve(count + string.count))  return;

    // Move right string builder part.
    uint32_t moveIndex = pos + string.count;
    uint32_t moveCount = this->count - pos;

    wmemmove(&data[moveIndex], &data[pos], moveCount);

    // Insert.
    wmemmove(&data[pos], string.data, string.count);

    this->count += string.count;
}

void NewstringBuilder::Insert(uint32_t pos, wchar_t c)
{
    wchar_t copy = c;
    Newstring ns(&copy, 1);
    return Insert(pos, ns);
}

void NewstringBuilder::Remove(uint32_t pos, uint32_t count)
{
    assert(pos + count <= this->count);

    if (pos == this->count - count)
    {
        this->count -= count;
        return;
    }

    uint32_t moveIndex = pos + count;
    uint32_t moveCount = this->count - pos - count;
    if (moveCount == 0)  return;

    wmemmove(&data[pos], &data[moveIndex], moveCount);

    this->count -= count;
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

    wchar_t* newData = (wchar_t*)allocator->Reallocate(data, sizeof(wchar_t) * newCapacity);
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
