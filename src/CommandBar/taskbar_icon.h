#pragma once
#include <Windows.h>

struct TaskbarIcon
{
	bool addToStatusArea(HWND hwnd, HICON icon, int iconID, int messageID);
	bool deleteFromStatusArea();

	bool isClicked(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam);
	bool isContextMenuRequested(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam);
	bool getMousePosition(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam, int* x, int* y);

	bool isAdded = false;
	HWND hwnd = 0;
	int iconID = 0;
	int messageID = 0;
};
