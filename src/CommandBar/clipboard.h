#pragma once
#include "newstring.h"


struct Clipboard
{
    static bool Open(HWND owner);
    static bool CopyText(const Newstring& text);
    static bool GetText(Newstring* text, IAllocator* allocator = &g_standardAllocator);
    static bool Close();
};