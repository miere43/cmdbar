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
#include "string_type.h"
#include "unicode.h"
#include "hint_window.h"
#include "command_window_style_loader.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


SingleInstance g_singleInstanceGuard;

void initDefaultStyle(CommandWindowStyle* style);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* lpCmdLine, int nCmdShow)
{
#ifndef _DEBUG
	if (!g_singleInstanceGuard.checkOrInitInstanceLock(L"CommandBarSingleInstanceGuard"))
    {
		if (!g_singleInstanceGuard.postMessageToOtherInstance(CommandWindow::g_showWindowMessageId, 0, 0))
        {
			MessageBoxW(0, L"Already running.", L"Command Bar", MB_OK);
		}

		return 0;
	}

    CloseClipboard();
#endif

    HRESULT hr;
	
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
	if (!InitCommonControlsEx(&icc)) 
    {
        MessageBoxW(0, L"Failed to initialize common controls.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

	hr = CoInitialize(nullptr);
    if (FAILED(hr))
    {
        MessageBoxW(0, L"Failed to initialize COM.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

	CommandEngine commandEngine;

	CommandWindowStyle windowStyle;
    initDefaultStyle(&windowStyle);

    String styleFilePath = String(L"D:/Vlad/cb/style.ini");
    if (OSUtils::fileExists(styleFilePath))
    {
        if (!CommandWindowStyleLoader::loadFromFile(styleFilePath, &windowStyle))
        {
            initDefaultStyle(&windowStyle);
            MessageBoxW(0, L"Failed to load style.", L"Error", MB_OK | MB_ICONERROR);
        }
    }

	CommandWindow commandWindow;
    commandWindow.style = &windowStyle;
    commandWindow.commandEngine = &commandEngine;

    CommandLoader commandLoader;
    registerBasicCommands(&commandLoader);

    Array<Command*> commands = commandLoader.loadFromFile(L"D:/Vlad/cb/cmds.ini");
    for (uint32_t i = 0; i < commandLoader.commandInfoArray.count; ++i)
        commandEngine.registerCommandInfo(commandLoader.commandInfoArray.data[i]);
    for (uint32_t i = 0; i < commands.count; ++i)
        commandEngine.registerCommand(commands.data[i]);

    bool initialized = commandWindow.init(400, 40);
    assert(initialized);

	String commandLine{ lpCmdLine };
	if (commandLine.indexOf(String(L"/noshow")) == -1)
		commandWindow.showAfterAllEventsProcessed(); // Make sure all controls are initialized.

    LOGFONTW font = {};
    lstrcpyW(font.lfFaceName, L"Segoe UI");

    HFONT hfont = CreateFontIndirectW(&font);

    MSG msg;
    uint32_t ret;

    while ((ret = GetMessageW(&msg, 0, 0, 0)) != 0)
    {
        if (ret == -1)
        {
#ifdef _DEBUG
            String error = OSUtils::formatErrorCode(GetLastError());
            __debugbreak();
            g_standardAllocator.dealloc(error.data);
#endif
            continue;
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    commandWindow.dispose();
    
    return static_cast<int>(msg.wParam);
}

void initDefaultStyle(CommandWindowStyle* windowStyle)
{
    windowStyle->textMarginLeft = 4.0f;
    windowStyle->textColor = D2D1::ColorF(D2D1::ColorF::Black);
    windowStyle->autocompletionTextColor = D2D1::ColorF(D2D1::ColorF::Gray);
    windowStyle->textboxBackgroundColor = D2D1::ColorF(D2D1::ColorF::White);
    windowStyle->selectedTextBackgroundColor = D2D1::ColorF(D2D1::ColorF::Aqua);
    windowStyle->fontFamily = String(L"Segoe UI");
    windowStyle->fontHeight = 22.0f;
    windowStyle->fontStyle = DWRITE_FONT_STYLE_NORMAL;
    windowStyle->fontStretch = DWRITE_FONT_STRETCH_NORMAL;
    windowStyle->fontWeight = DWRITE_FONT_WEIGHT_REGULAR;
    windowStyle->borderColor = D2D1::ColorF(D2D1::ColorF::Black);
    windowStyle->borderSize = 5;
}