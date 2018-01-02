#pragma once
#include "string_type.h"


struct StringBuilder
{
    String str = { 0, 0 };
    int capacity = 0;
    IAllocator* allocator = nullptr;

    void reserve(uint32_t newCapacity);
    void appendChar(wchar_t c);
    void appendString(const wchar_t* str, uint32_t count);
    void appendString(const wchar_t* str);
    void appendString(const String& str);
    void appendFormat(const wchar_t* format, ...);
    void appendChar(uint32_t cp);
    void nullTerminate();
private:
    void reserve2(int newCapacity);// { reserve(newCapacity <= 32 ? 32 : newCapacity * 2); }
};