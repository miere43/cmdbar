#pragma once
#include <stdint.h>

#include "newstring.h"


namespace Unicode
{

Newstring DecodeString(const void* data, uint32_t dataSize, Encoding encoding, IAllocator* allocator = &g_standardAllocator);
void* EncodeString(const Newstring& string, uint32_t* encodedStringByteSize, Encoding encoding, IAllocator* allocator = &g_standardAllocator);
const wchar_t* Decode16(const wchar_t* text, uint32_t* codepoint);

inline bool IsHighSurrogate(uint32_t lowpart)
{
    return lowpart >= 0xD800 && lowpart <= 0xDBFF;
}

inline bool IsLowSurrogate(uint32_t highpart)
{
    return highpart >= 0xDC00 && highpart <= 0xDFFF;
}

inline bool IsSurrogatePair(uint32_t codepoint)
{
    return IsLowSurrogate(codepoint & 0x0000FFFF) && IsHighSurrogate((codepoint & 0xFFFF0000) >> 16);
}

}; // namespace Unicode
