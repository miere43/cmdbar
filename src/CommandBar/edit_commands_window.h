#pragma once
#include <Windows.h>
#include "command_engine.h"


struct EditCommandsWindow
{
    HWND hwnd = 0;
    HWND listHwnd = 0;
    HWND labelHwnd = 0;

    bool init(HWND parent, CommandEngine* commandEngine);
private:
    HFONT font = 0;
    bool isInitialized = false;
    CommandEngine* commandEngine;

    LRESULT onCreate();
    LRESULT onSize(WPARAM type, int newWidth, int newHeight);

    static bool initGlobalResources();
    LRESULT WINAPI wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT WINAPI staticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static bool g_globalResourcesInitialized;
    static ATOM g_windowClass;
};