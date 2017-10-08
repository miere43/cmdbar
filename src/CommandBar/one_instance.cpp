#include "one_instance.h"
#include "command_window.h"

//const UINT g_oneInstanceMessage = WM_USER + 64;

bool checkOrInitInstanceLock()
{
	HANDLE mutex = CreateMutexW(nullptr, true, L"CommandBarPleaseOneInstance");
	return GetLastError() != ERROR_ALREADY_EXISTS;
}

bool postMessageToOtherInstance(UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hwnd = FindWindowW(CommandWindow::g_className, CommandWindow::g_windowName);
	if (hwnd == 0)
		return 0;

	return 0 != PostMessageW(hwnd, message, wParam, lParam);
}
