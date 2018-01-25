#include <assert.h>
#include <Windows.h>
#include <wchar.h>

#include "clipboard.h"
#include "string_type.h"


bool Clipboard::open(HWND owner)
{
    return !!OpenClipboard(owner);
}

bool Clipboard::copyText(const wchar_t* str, int count)
{
    assert(!!EmptyClipboard());
    assert(count >= 0);
    if (count == 0)
        return true;

    const size_t strSize = (count + 1) * sizeof(wchar_t);

    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, strSize);
    if (mem == 0) return false;

    void* block = GlobalLock(mem);
    wmemcpy((wchar_t*)block, str, strSize);
    GlobalUnlock(mem);

    return 0 != SetClipboardData(CF_UNICODETEXT, mem);
}

bool Clipboard::getText(String* result)
{
    HGLOBAL mem = GetClipboardData(CF_UNICODETEXT);
    if (mem == 0)
        return false;

    wchar_t* text = static_cast<wchar_t*>(GlobalLock(mem));
    if (text == nullptr)
        return false;

    size_t memSize = GlobalSize(mem);
    if (memSize == 0)
    {
        GlobalUnlock(mem);
        return false;
    }
    
    int maxCount = static_cast<int>((memSize - 1) / sizeof(wchar_t)); // Don't count terminating null.
    *result = String::alloc(maxCount);
    assert(!result->isEmpty());

    int i = 0;
    for (; i < maxCount; ++i)
    {
        wchar_t c = text[i];
        if (c == '\r' || c == '\n') continue;
        if (c == '\0') break;
        result->data[i] = c;
    }
    result->count = i;
    result->data[result->count] = L'\0';

    GlobalUnlock(mem);
    text = nullptr;

    return true;
}

bool Clipboard::close()
{
    return CloseClipboard();
}

void Clipboard::debugDumpClipboardFormats()
{
    int count = CountClipboardFormats();
    if (count == 0)
    {
        DWORD err = GetLastError();
        if (err != NO_ERROR)
            __debugbreak();
        else
            OutputDebugStringW(L"no clipboard formats\n");
        return;
    }

    wchar_t buf[512];
    wchar_t* bufCurr = buf;
    
    UINT format = 0;
    do
    {
        format = EnumClipboardFormats(format);
        bufCurr += wsprintfW(bufCurr, L"%d ", format);

    } while (format != 0);

    if (GetLastError() == ERROR_SUCCESS)
    {
        *bufCurr++ = L'\n';
        *bufCurr++ = L'\0';
        OutputDebugStringW(buf);
    }
    else
        __debugbreak();
}
