#include "utils.h"


bool ShowErrorBox(HWND hwnd, const wchar_t* text)
{
    if (!text)
    {
        text = L"Unknown error.";    
    }

    MessageBoxW(hwnd, text, L"Error", MB_ICONERROR);
    return false;
}
