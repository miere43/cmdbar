#pragma once
#include "newstring.h"


struct Clipboard
{
    static bool Open(HWND owner);
    static bool CopyText(const Newstring& text);
    static bool GetText(Newstring* text);
    static bool Close();
};