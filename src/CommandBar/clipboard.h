#pragma once
struct String;


struct Clipboard
{
    static bool open(HWND owner);
    static bool copyText(const wchar_t* str, int count);
    static bool getText(String* result);
    static bool close();

    static void debugDumpClipboardFormats();
};