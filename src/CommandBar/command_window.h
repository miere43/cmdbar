#pragma once
#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include "command_window_textbox.h"
#include "taskbar_icon.h"
#include "command_engine.h"


struct CommandWindowStyle;


struct CommandWindow
{
	bool init(HINSTANCE hInstance);

	void showWindow();
	void showAfterAllEventsProcessed();
	void hideWindow();
	void toggleVisibility();

	void exit();

	int enterEventLoop();
	LRESULT wndProc(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam);
	void beforeCommandRun();


	HWND hwnd = 0;
	HMENU trayMenu = 0;

    ID2D1Factory* d2d1 = nullptr;
    IDWriteFactory* dwrite = nullptr;

	const CommandWindowStyle* style = nullptr;
	CommandWindowTextbox textbox;
	TaskbarIcon taskbarIcon;
	CommandEngine* commandEngine = nullptr;

    int windowWidth = 400;
    int windowHeight = 40;

	static const wchar_t* g_className;
	static const wchar_t* g_windowName;
private:
    bool isInitialized = false;
	bool shouldCatchInvalidUsageErrors = false;
	float showWindowYRatio = 0.4f;


	void evaluate();
	void dispose();

	static bool initGlobalResources(HINSTANCE hInstance);

	static bool g_globalResourcesInitialized;
	static ATOM g_windowClass;
	static HICON g_appIcon;
};

struct CommandWindowStyle
{
	HBRUSH backgroundBrush = 0;
};

//#include <Windows.h>
//#include "context.h"
//
//
//int CB_EnterMainUILoop(CB_Context* context, HINSTANCE hInstance, wchar_t * lpCmdLine, int nCmdShow, HANDLE fileMapping);
//
////void CB_SetUIVisibility(bool visible);
////bool CB_IsUIVisible();
//
//extern LARGE_INTEGER g_mainStartupTick;
//extern LARGE_INTEGER g_mainTickFrequency;
