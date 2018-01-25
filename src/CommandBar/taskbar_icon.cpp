#include <windowsx.h>

#include "taskbar_icon.h"
#include "trace.h"


bool TaskbarIcon::enable(HWND hwnd, HICON icon, int iconID, int messageID)
{
	if (isAdded)
		return true;

	NOTIFYICONDATAW data{ 0 };
	data.cbSize = sizeof(data);
	data.uVersion = NOTIFYICON_VERSION_4;
	data.hWnd = hwnd;
	data.uID = iconID;
	data.uCallbackMessage = messageID;
	data.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	data.hIcon = icon;
	lstrcpyW(data.szTip, L"Command Bar");

	if (!Shell_NotifyIconW(NIM_ADD, &data))
		return false;
	if (!Shell_NotifyIconW(NIM_SETVERSION, &data))
		return false;

	this->hwnd = hwnd;
	this->iconID = iconID;
	this->messageID = messageID;
	
	isAdded = true;

	return true;
}

bool TaskbarIcon::disable()
{
	if (!isAdded)
		return true;

	NOTIFYICONDATAW data = { 0 };
	data.cbSize = sizeof(data);
	data.uVersion = NOTIFYICON_VERSION_4;
	data.hWnd = hwnd;
	data.uID = iconID;
	data.uCallbackMessage = messageID;
	data.uFlags = NIF_MESSAGE;

	if (Shell_NotifyIconW(NIM_DELETE, &data)) {
		isAdded = false;
		return true;
	}

	return false;
}

bool TaskbarIcon::isClicked(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam)
{
	return (this->hwnd == hwnd && msg == messageID && LOWORD(lParam) == WM_LBUTTONDOWN);
}

bool TaskbarIcon::isContextMenuRequested(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam)
{
	(void)wParam;
	return (this->hwnd == hwnd && msg == messageID && LOWORD(lParam) == WM_CONTEXTMENU);
}

bool TaskbarIcon::getMousePosition(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam, int * x, int * y)
{
	if (x == nullptr || y == nullptr)
		return false;

	if (this->hwnd == hwnd && msg == messageID) {
		// Yes, we are using GET_*_LPARAM on wParam value.
		*x = GET_X_LPARAM(wParam);
		*y = GET_Y_LPARAM(wParam);

		return true;
	}

	return false;
}
