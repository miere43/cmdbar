#include <assert.h>
#include <Windows.h>

#include "unicode.h"
#define TINY_UTF_IMPLEMENTATION
#include "tinyutf.h"

#include "array.h"

Newstring Unicode::DecodeString(const void* data, uint32_t dataSize, Encoding encoding, IAllocator* allocator)
{
    assert(data != nullptr);
    assert(dataSize >= 0);
    assert(allocator != nullptr);

    switch (encoding)
    {
        case Encoding::ASCII:
        {
            const uint32_t bufCount = (dataSize);
            const uint32_t bufSize  = (dataSize + 1) * sizeof(wchar_t);
            wchar_t* buf = static_cast<wchar_t*>(allocator->Allocate(bufSize));
            if (buf == nullptr)
                return Newstring::Empty();

            const char* strData = reinterpret_cast<const char*>(data);

            for (uint32_t i = 0; i < bufCount; ++i)
                *buf++ = static_cast<wchar_t>(*strData++);
            *buf = L'\0';

            return Newstring(buf, bufCount);
        }
        case Encoding::UTF8:
        {
            Array<wchar_t> buf{ allocator };

            const uint32_t bestCaseCount = dataSize + 2 + 1;
            if (!buf.Reserve(bestCaseCount))
                return Newstring::Empty();

            const char* strData = reinterpret_cast<const char*>(data);
            const char* strDataEnd = strData + dataSize;
            wchar_t* bufData = buf.data;

            int codepoint;
            while (strData < strDataEnd)
            {
                strData = tuDecode8(strData, &codepoint);
                if (codepoint == 0xFFFD) continue;

                if (buf.capacity < buf.count + 3)
                {
                    if (!buf.Reserve(buf.count + 3))
                    {
                        buf.Dispose();
                        return Newstring::Empty();
                    }

                    bufData = buf.data + buf.count;
                }

                wchar_t* newBufData = tuEncode16(bufData, codepoint);
                buf.count += static_cast<uint32_t>(newBufData - bufData);
                bufData = newBufData;
            }

            return Newstring(buf.data, buf.count);
        }
    }

    assert(false);
    return Newstring::Empty();
}

void* Unicode::EncodeString(const Newstring& string, uint32_t* encodedStringByteSize, Encoding encoding, IAllocator* allocator)
{
    assert(allocator);
    assert(encodedStringByteSize);
    //encoding = OSUtils::normalizeEncoding(encoding);

    if (Newstring::IsNullOrEmpty(string))  return nullptr;

    switch (encoding)
    {
        case Encoding::ASCII:
        {
            char* data = (char*)allocator->Allocate(string.count * sizeof(char));
            if (!data)  return nullptr;

            for (uint32_t i = 0; i < string.count; ++i)
            {
                data[i] = (char)string.data[i];
            }

            *encodedStringByteSize = string.count * sizeof(char);
            return data;
        }
        case Encoding::UTF8:
        {
            const size_t dataSize = string.count * 4;
            char* data = (char*)allocator->Allocate(dataSize);
            if (!data)  return nullptr;

            int nwritten = WideCharToMultiByte(
                CP_UTF8,
                0,
                string.data,
                (int)string.count,
                data,
                (int)dataSize,
                0,
                0);

            assert(nwritten != 0); // @TODO
            
            *encodedStringByteSize = nwritten;
            return data;
        }
        default:
            assert(false);
            return nullptr;
    }
}

const wchar_t* Unicode::Decode16(const wchar_t* text, uint32_t* codepoint)
{
    assert(text);
    assert(codepoint);

    return tuDecode16(text, reinterpret_cast<int*>(codepoint));
}
