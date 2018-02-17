#pragma once
#include <stdint.h>

#include "string_type.h"
#include "newstring.h"

namespace unicode
{
    Newstring DecodeString(const void* data, uint32_t dataSize, Encoding encoding, IAllocator* allocator = &g_standardAllocator); 
    void* EncodeString(const Newstring& string, uint32_t* encodedStringByteSize, Encoding encoding, IAllocator* allocator = &g_standardAllocator);


    const wchar_t* decode16(const wchar_t* text, uint32_t* codepoint);

    inline bool isHighSurrogate(uint32_t lowpart)
    {
        return lowpart >= 0xD800 && lowpart <= 0xDBFF;
    }

    inline bool isLowSurrogate(uint32_t highpart)
    {
        return highpart >= 0xDC00 && highpart <= 0xDFFF;
    }

    inline bool isSurrogatePair(uint32_t codepoint)
    {
        return isLowSurrogate(codepoint & 0x0000FFFF) && isHighSurrogate((codepoint & 0xFFFF0000) >> 16);
    }
  
    
    String decodeString(const void* data, uint32_t dataSize, Encoding encoding, IAllocator* allocator = &g_standardAllocator); // @Deprecated
};
