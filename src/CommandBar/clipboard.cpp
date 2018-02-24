#include <assert.h>
#include <Windows.h>
#include <wchar.h>

#include "clipboard.h"


bool Clipboard::Open(HWND owner)
{
    return !!OpenClipboard(owner);
}

bool Clipboard::CopyText(const Newstring& text)
{
    ::EmptyClipboard();
    if (Newstring::IsNullOrEmpty(text))  return true;

    const size_t strSize = (text.count + 1) * sizeof(wchar_t);

    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, strSize);
    if (mem == 0) return false;

    void* block = GlobalLock(mem);
    wmemcpy((wchar_t*)block, text.data, strSize);
    GlobalUnlock(mem);

    return 0 != SetClipboardData(CF_UNICODETEXT, mem);
}

bool Clipboard::GetText(Newstring* result)
{
    assert(result);

    HGLOBAL mem = GetClipboardData(CF_UNICODETEXT);
    if (mem == 0)
        return false;

    wchar_t* text = static_cast<wchar_t*>(GlobalLock(mem));
    if (text == nullptr)
        return false;

    size_t memSize = GlobalSize(mem);
    if (memSize == 0 || memSize > UINT32_MAX)
    {
        GlobalUnlock(mem);
        return false;
    }
    
    uint32_t maxCount = static_cast<uint32_t>((memSize - 1) / sizeof(wchar_t)); // Don't count terminating null.
    *result = Newstring::New(maxCount + 1);
    assert(!Newstring::IsNullOrEmpty(result));

    uint32_t i = 0;
    uint32_t actual = 0;
    for (; i < maxCount; ++i)
    {
        wchar_t c = text[i];
        if (c == '\r' || c == '\n') continue;
        if (c == '\0') break;
        result->data[i] = c;
        ++actual;
    }
    result->count = actual;
    result->data[result->count] = L'\0';

    GlobalUnlock(mem);
    text = nullptr;

    return true;
}

bool Clipboard::Close()
{
    return !!CloseClipboard();
}
