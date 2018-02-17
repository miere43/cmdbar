#pragma once
#include "newstring.h"

struct NewstringBuilder
{
    union
    {
        Newstring string;
        struct
        {
            wchar_t* data;
            uint32_t count;
        };
    };
    uint32_t capacity;
    IAllocator* allocator;

    NewstringBuilder();

    void Dispose();

    void Append(wchar_t c);
    void Append(const wchar_t* string);
    void Append(const wchar_t* string, uint32_t count);
    void Append(const Newstring& string);
    void ZeroTerminate();

    uint32_t GetRemainingCapacity() const;

    bool Reserve(uint32_t newCapacity);
    Newstring TransferToString();
    Newstring& GetStringRef();
};