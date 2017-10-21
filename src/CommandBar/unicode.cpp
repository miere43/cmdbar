#include <assert.h>

#include "unicode.h"
#define TINY_UTF_IMPLEMENTATION
#include "tinyutf.h"


String unicode::decodeString(const void* data, int dataSize, Encoding encoding, IAllocator* allocator)
{
    assert(data != nullptr);
    assert(dataSize >= 0);
    assert(allocator != nullptr);

    switch (encoding)
    {
        case Encoding::ASCII:
        {
            const int bufCount = (dataSize);
            const int bufSize  = (dataSize + 1) * sizeof(wchar_t);
            wchar_t* buf = static_cast<wchar_t*>(allocator->alloc(bufSize));
            if (buf == nullptr)
                return String::null;

            const char* strData = reinterpret_cast<const char*>(data);

            for (int i = 0; i < bufCount; ++i)
                *buf++ = static_cast<wchar_t>(*strData++);
            *buf = L'\0';

            return String{ buf, static_cast<int>(bufCount) };
        }
        case Encoding::UTF8:
        {
            Array<wchar_t> buf{ allocator };

            const int bestCaseCount = dataSize + 2 + 1;
            if (!buf.reserve(bestCaseCount))
                return false;

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
                    if (!buf.reserve(buf.count + 3))
                    {
                        buf.dealloc();
                        return String::null;
                    }

                    bufData = buf.data + buf.count;
                }

                wchar_t* newBufData = tuEncode16(bufData, codepoint);
                buf.count += static_cast<int>(newBufData - bufData);
                bufData = newBufData;
            }

            return String{ buf.data, buf.count };
        }
    }

    assert(false);
    return nullptr;
}

const wchar_t* unicode::decode16(const wchar_t* text, uint32_t* codepoint)
{
    assert(text);
    assert(codepoint);

    return tuDecode16(text, reinterpret_cast<int*>(codepoint));
}
