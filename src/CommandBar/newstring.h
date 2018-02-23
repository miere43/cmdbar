#pragma once
#include "common.h"
#include <initializer_list>
#include "allocators.h"

struct Newstring
{
    wchar_t* data  = nullptr;
    uint32_t count = 0;

	explicit Newstring();
	explicit Newstring(wchar_t* data, uint32_t count);

    bool operator==(const Newstring& rhs) const;
    bool operator!=(const Newstring& rhs) const;

    bool operator==(const wchar_t* rhs) const;
    bool operator!=(const wchar_t* rhs) const;

    int IndexOf(wchar_t c) const;
    int LastIndexOf(wchar_t c) const;

    // Copies characters from this string to another.
    // 'fromIndex': index in this string which specified start index to copy characters from
    // 'copyCount': maximum number of characters to copy to destination.
    // 'destIndex': index in destination string where characters should be copied.
    // Returns number of characters copied.
    uint32_t CopyTo(Newstring* dest, uint32_t fromIndex = 0, uint32_t destIndex = 0, uint32_t copyCount = UINT32_MAX) const;

    Newstring RefSubstring(uint32_t index, uint32_t count = 0xFFFFFFFF) const;

    Newstring Clone(IAllocator* allocator = &g_standardAllocator);
    Newstring CloneAsCString(IAllocator* allocator = &g_standardAllocator) const;

    Newstring Trimmed() const;

    void Dispose(IAllocator* allocator = &g_standardAllocator);

    bool IsZeroTerminated() const;

    static bool IsNullOrEmpty(const Newstring& string);
    static bool IsNullOrEmpty(const Newstring* const string);
    
    static Newstring New(uint32_t count, IAllocator* allocator = &g_standardAllocator);
    static Newstring NewFromWChar(const wchar_t* string, IAllocator* allocator = &g_standardAllocator);
    static Newstring NewFromWChar(const wchar_t* string, uint32_t count, IAllocator* allocator = &g_standardAllocator);
    static Newstring NewCStringFromWChar(const wchar_t* string, IAllocator* allocator = &g_standardAllocator);
    static Newstring Clone(const Newstring& string, IAllocator* allocator);
    static Newstring Clone(const Newstring* string, IAllocator* allocator);

    static Newstring Join(std::initializer_list<Newstring> strings, IAllocator* allocator = &g_standardAllocator);

    static Newstring WrapWChar(wchar_t* string);
    static Newstring WrapWChar(wchar_t* string, uint32_t count);

    static const Newstring WrapConstWChar(const wchar_t* string);
    static const Newstring WrapConstWChar(const wchar_t* string, uint32_t count);


    static Newstring Empty();
};
