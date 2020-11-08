#include "command_window_tray.h"
#include <assert.h>
#include <windowsx.h>
#include "newstring.h"
#include "os_utils.h"

static const UINT IconID = 1;
static const UINT MessageID = WM_USER + 15;


bool CommandWindowTray::Initialize(Newstring* failureReason)
{
    assert(hwnd);
    assert(icon);

    Dispose();

    NOTIFYICONDATAW data{ 0 };
    data.cbSize = sizeof(data);
    data.uVersion = NOTIFYICON_VERSION_4;
    data.hWnd = hwnd;
    data.uID = IconID;
    data.uCallbackMessage = MessageID;
    data.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
    data.hIcon = icon;
    lstrcpyW(data.szTip, L"Command Bar");

    if (!Shell_NotifyIconW(NIM_ADD, &data))
        goto fail;
    if (!Shell_NotifyIconW(NIM_SETVERSION, &data))
        goto fail;

    menu = CreatePopupMenu();
    if (!menu)
        goto fail;

    AppendMenuW(menu, MF_STRING, (UINT_PTR)TrayMenuAction::Show, L"Show");
    AppendMenuW(menu, MF_STRING, (UINT_PTR)TrayMenuAction::ReloadCommandsFile, L"Reload commands file");
    AppendMenuW(menu, MF_STRING, (UINT_PTR)TrayMenuAction::OpenCommandsFile, L"Open commands file");
    AppendMenuW(menu, MF_SEPARATOR, (UINT_PTR)TrayMenuAction::None, nullptr);
    AppendMenuW(menu, MF_STRING, (UINT_PTR)TrayMenuAction::Exit, L"Exit");

    return true;
fail:
    Dispose();

    if (failureReason)
    {
        *failureReason = Newstring::FormatTemp(
            L"Unable to create tray menu: %s",
            OSUtils::FormatErrorCode(GetLastError(), 0, &g_tempAllocator).data);
    }
    return false;
}

void CommandWindowTray::Dispose()
{
    NOTIFYICONDATAW data = { 0 };
    data.cbSize = sizeof(data);
    data.uVersion = NOTIFYICON_VERSION_4;
    data.hWnd = hwnd;
    data.uID = IconID;
    data.uCallbackMessage = MessageID;
    data.uFlags = NIF_MESSAGE;

    Shell_NotifyIconW(NIM_DELETE, &data);

    DeleteMenu(menu, 0, 0);
    menu = 0;
}

bool CommandWindowTray::ProcessEvent(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam, TrayMenuAction* action)
{
    assert(action);
    *action = TrayMenuAction::None;

    if (MessageID != msg)  return false;

    if (LOWORD(lParam) == WM_LBUTTONDOWN)
    {
        *action = TrayMenuAction::Show;
        return true;
    }
    else if (LOWORD(lParam) == WM_CONTEXTMENU)
    {
        int x = GET_X_LPARAM(wParam);
        int y = GET_Y_LPARAM(wParam);

        DWORD flags = TPM_NONOTIFY | TPM_RETURNCMD;

        bool isLeftAligned = GetSystemMetrics(SM_MENUDROPALIGNMENT) == 0;
        flags = flags | (isLeftAligned ? TPM_LEFTALIGN | TPM_HORPOSANIMATION : TPM_RIGHTALIGN | TPM_HORNEGANIMATION);

        // MSDN: To display a context menu for a notification icon, the current window must be the
        // foreground window before the application calls TrackPopupMenu or TrackPopupMenuEx
        SetForegroundWindow(hwnd);
        *action = (TrayMenuAction)TrackPopupMenuEx(menu, flags, x, y, hwnd, nullptr);
        
        return true;
    }

    return false;
}