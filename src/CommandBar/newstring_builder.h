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
    //void Insert(uint32_t pos, wchar_t c);
    void ZeroTerminate();

    void Insert(uint32_t pos, const Newstring& string);
    void Insert(uint32_t pos, wchar_t c);

    void Remove(uint32_t pos, uint32_t count);

    uint32_t GetRemainingCapacity() const;

    bool Reserve(uint32_t newCapacity);
    Newstring TransferToString();
};