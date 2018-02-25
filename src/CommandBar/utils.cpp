#include "utils.h"


bool ShowErrorBox(HWND hwnd, const Newstring& text)
{
    Newstring msg = text;
    if (Newstring::IsNullOrEmpty(msg))
    {
        // @TODO: Insert debug build debugbreak here.
        msg = Newstring::WrapConstWChar(L"Unknown error.");
    }

    if (!msg.IsZeroTerminated())
    {
        msg = msg.CloneAsCString(&g_tempAllocator);
    }

    MessageBoxW(hwnd, msg.data, L"Error", MB_ICONERROR);
    return false;
}
