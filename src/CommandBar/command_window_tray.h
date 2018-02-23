#pragma once
#include <Windows.h>

struct CommandWindow;
struct Newstring;

enum class TrayMenuAction
{
    None = 0,
    Show = 1,
    Exit = 2,
};

struct CommandWindowTray
{
    HMENU menu = 0;
    HWND  hwnd = 0;

    bool Initialize(HWND hwnd, HICON icon, Newstring* failureReason);
    void Dispose();

    bool ProcessEvent(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam, TrayMenuAction* action);
};