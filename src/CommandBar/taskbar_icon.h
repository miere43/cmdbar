#pragma once
#include <Windows.h>

struct TaskbarIcon
{
    HWND hwnd = 0;
    bool isAdded = false;
    int iconID = 0;
    int messageID = 0;

	bool addToStatusArea(HWND hwnd, HICON icon, int iconID, int messageID);
	bool deleteFromStatusArea();

	bool isClicked(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam);
	bool isContextMenuRequested(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam);
	bool getMousePosition(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam, int* x, int* y);
};
