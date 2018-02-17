#include "newstring.h"
#include <wchar.h>


bool Newstring::operator==(const Newstring& rhs) const
{
    if (IsNullOrEmpty(this) && IsNullOrEmpty(rhs))  return true;
    return count == rhs.count && wcsncmp(data, rhs.data, count);
}

bool Newstring::operator!=(const Newstring & rhs) const
{
    return !operator==(rhs);
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

Newstring Newstring::Clone(IAllocator* allocator)
{
    assert(allocator);
    return Newstring::Clone(this, allocator);
}

Newstring Newstring::CloneAsCString(IAllocator* allocator) const
{
    assert(allocator);
    if (IsNullOrEmpty(this))  return Empty();

    Newstring ns = New(count + 1, allocator);
    if (IsNullOrEmpty(ns))  return Empty();

    CopyTo(&ns);
    ns.data[ns.count - 1] = L'\0';

    return ns;
}

void Newstring::Dispose(IAllocator* allocator)
{
    assert(allocator);
    allocator->dealloc(data);
    data  = nullptr;
    count = 0;
}

bool Newstring::IsZeroTerminated() const
{
    if (IsNullOrEmpty(this))  return false;
    return data[count - 1] == L'\0';
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

    wchar_t* data = (wchar_t*)allocator->alloc(count * sizeof(wchar_t));
    if (!data)  return Empty();

    data[0] = L'\0';

    return { data, count };
}

Newstring Newstring::NewFromWChar(const wchar_t* string, IAllocator* allocator)
{
    assert(allocator);
    auto wrapper = WrapConstWChar(string);

    return IsNullOrEmpty(wrapper) ? Empty() : Newstring::Clone(wrapper, allocator);
}

Newstring Newstring::NewFromWChar(const wchar_t* string, uint32_t count, IAllocator* allocator)
{
    assert(allocator);
    Newstring wrapper;//= Newstringstring, count };
    wrapper.data = (wchar_t*)string;
    wrapper.count = count;

    return IsNullOrEmpty(wrapper) ? Empty() : Newstring::Clone(wrapper, allocator);
}

Newstring Newstring::NewCStringFromWChar(const wchar_t * string, IAllocator * allocator)
{
    assert(allocator);

    auto wrapper = WrapConstWChar(string);
    if (IsNullOrEmpty(wrapper))  return Empty();

    Newstring ns = New(wrapper.count + 1, allocator);
    if (IsNullOrEmpty(ns))  return Empty();

    wrapper.CopyTo(&ns);
    ns.data[ns.count - 1] = L'\0';

    return ns;
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

Newstring Newstring::WrapWChar(wchar_t* string)
{
    if (!string)  return Empty();

    return WrapWChar(string, (uint32_t)wcslen(string));
}

Newstring Newstring::WrapWChar(wchar_t* string, uint32_t count)
{
    if (!string || count == 0)  return Empty();

    return { string, count };
}

const Newstring Newstring::WrapConstWChar(const wchar_t* string)
{
    if (!string)  return Empty();

    return WrapConstWChar(string, (uint32_t)wcslen(string));
}

const Newstring Newstring::WrapConstWChar(const wchar_t* string, uint32_t count)
{
    if (!string || count == 0)  return Empty();

    return { (wchar_t*)string, count };
}

Newstring Newstring::Empty()
{
    return { nullptr, 0 };
}