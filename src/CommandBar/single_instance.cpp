#include "command_window.h"
#include "single_instance.h"

//const UINT g_oneInstanceMessage = WM_USER + 64;

bool SingleInstance::CheckOrInitInstanceLock(const wchar_t* instanceId)
{
    HANDLE mutex = CreateMutexW(nullptr, true, instanceId);
    return GetLastError() != ERROR_ALREADY_EXISTS;
}

bool SingleInstance::PostMessageToOtherInstance(UINT messageId, WPARAM wParam, LPARAM lParam)
{
    HWND hwnd = FindWindowW(CommandWindow::g_className, CommandWindow::g_windowName);
    if (hwnd == 0)
        return false;

    return 0 != PostMessageW(hwnd, messageId, wParam, lParam);
}
