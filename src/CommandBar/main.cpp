#include <assert.h>
#include <Windows.h>
#include <CommCtrl.h>
#include <d2d1.h>
#include <dwrite.h>

#include "command_window.h"
#include "os_utils.h"
#include "allocators.h"
#include "command_engine.h"
#include "single_instance.h"
#include "basic_commands.h"
#include "command_loader.h"
#include "unicode.h"
#include "hint_window.h"
#include "newstring.h"
#include "newstring_builder.h"
#include "command_window_style_loader.h"
#include "defer.h"
#include "parse_ini.h"
#include "common.h"
#include "utils.h"


#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")


SingleInstance g_singleInstanceGuard;

bool InitializeAllocators();
bool InitializeLibraries();
void DisposeAllocators();
void DisposeLibraries();

Newstring GetCommandsFilePath();
Newstring AskUserForCmdsFilePath();

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* lpCmdLine, int nCmdShow)
{
    if (!InitializeAllocators())
        return 1;
    defer(DisposeAllocators());

    if (!InitializeLibraries())
        return 1;
    defer(DisposeLibraries());

#ifndef _DEBUG
	if (!g_singleInstanceGuard.CheckOrInitInstanceLock(L"CommandBarSingleInstanceGuard"))
    {
		if (!g_singleInstanceGuard.PostMessageToOtherInstance(CommandWindow::g_showWindowMessageId, 0, 0))
        {
			MessageBoxW(0, L"Already running.", L"Command Bar", MB_OK);
		}

		return 0;
	}
#endif

	CommandEngine commandEngine;
    defer(commandEngine.Dispose());

	CommandWindowStyle windowStyle;

    Newstring styleFilePath = Newstring::WrapConstWChar(L"D:/Vlad/cb/style.ini");
    if (OSUtils::FileExists(styleFilePath))
    {
        if (!CommandWindowStyleLoader::LoadFromFile(styleFilePath, &windowStyle))
        {
            windowStyle = CommandWindowStyle();
            MessageBoxW(0, L"Failed to load style.", L"Error", MB_OK | MB_ICONERROR);
        }
    }

	CommandWindow commandWindow;
    defer(commandWindow.Dispose());

    nCmdShow = wcscmp(lpCmdLine, L"/noshow") == 0 ? 0 : SW_SHOW;

    bool initialized = commandWindow.Initialize(&commandEngine, &windowStyle, nCmdShow); // width=400, height=40
    if (!initialized)  return 1;

    commandWindow.ReloadCommandsFile();

    MSG msg;
    uint32_t ret;

    while ((ret = GetMessageW(&msg, 0, 0, 0)) != 0)
    {
        if (ret == -1)
        {
            continue;
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);

        g_tempAllocator.Reset();
    }

    return static_cast<int>(msg.wParam);
}

bool InitializeLibraries()
{
    HRESULT hr;

    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    if (!InitCommonControlsEx(&icc))
    {
        return ShowErrorBox(0, Newstring::FormatTempCString(
            L"Failed to initialize common controls.\n\nError code was 0x%08X.", GetLastError()));
    }

    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);
    if (FAILED(hr))
    {
        return ShowErrorBox(0, Newstring::FormatTempCString(
            L"Failed to initialize COM.\n\nError code was 0x%08X.", GetLastError()));
    }

    return true;
}

void DisposeAllocators()
{
    OutputDebugStringW(Newstring::FormatTempCString(L"std allocator leftover: %d\n", (int)g_standardAllocator.allocated));

    g_tempAllocator.Dispose();
}

void DisposeLibraries()
{
    CoUninitialize();
}

bool InitializeAllocators()
{
    if (!g_tempAllocator.SetSize(4096))
    {
        return ShowErrorBox(0, L"Unable to initialize temporary allocator.");
    }

    return true;
}