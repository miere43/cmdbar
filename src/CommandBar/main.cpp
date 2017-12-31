#include <assert.h>
#include <Windows.h>
#include <CommCtrl.h>
#include <d2d1.h>
#include <dwrite.h>

#include "trace.h"
#include "command_window.h"
#include "os_utils.h"
#include "allocators.h"
#include "command_engine.h"
#include "one_instance.h"
#include "basic_commands.h"
#include "command_loader.h"
#include "string_type.h"
#include "unicode.h"


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* lpCmdLine, int nCmdShow)
{
#ifndef _DEBUG
	if (!checkOrInitInstanceLock()) {
		if (!postMessageToOtherInstance(g_oneInstanceMessage, 0, 0)) {
			MessageBoxW(0, L"Already running.", L"Command Bar", MB_OK);
		}

		return 0;
	}

    CloseClipboard();
#endif

    Array<int> arr;

    HRESULT hr;

	Trace::init();
	
	InitCommonControls();
	hr = CoInitialize(nullptr);
    if (FAILED(hr))
    {
        MessageBoxW(0, L"Failed to initialize COM.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Initialize Direct2D and DirectWrite
    ID2D1Factory* d2d1 = nullptr;
    IDWriteFactory* dwrite = nullptr;
    {
        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d1)))
        {
            // @TODO: better error message
            MessageBoxW(0, L"Cannot initialize Direct2D.", L"Error", MB_OK | MB_ICONERROR);
            return 1;
        }

        if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&dwrite))))
        {
            // @TODO: better error message
            MessageBoxW(0, L"Cannot initialize DirectWrite.", L"Error", MB_OK | MB_ICONERROR);
            return 1;
        }
    }

	CommandEngine commandEngine;

	CommandWindowStyle windowStyle;
	windowStyle.textMarginLeft = 4.0f;
    windowStyle.textColor = D2D1::ColorF(D2D1::ColorF::Black);
    windowStyle.autocompletionTextColor = D2D1::ColorF(D2D1::ColorF::Gray);
    windowStyle.textboxBackgroundColor = D2D1::ColorF(D2D1::ColorF::White);
    windowStyle.selectedTextBackgroundColor = D2D1::ColorF(D2D1::ColorF::Aqua);
	windowStyle.fontFamily = String(L"Segoe UI");
    windowStyle.fontHeight = 22.0f;
    windowStyle.fontStyle = DWRITE_FONT_STYLE_NORMAL;
    windowStyle.fontStretch = DWRITE_FONT_STRETCH_NORMAL;
    windowStyle.fontWeight = DWRITE_FONT_WEIGHT_REGULAR;
    windowStyle.borderColor = D2D1::ColorF(D2D1::ColorF::Black);
    windowStyle.borderSize = 5;

	CommandWindow commandWindow;
    commandWindow.style = &windowStyle;
    commandWindow.commandEngine = &commandEngine;
    commandWindow.d2d1 = d2d1;
    commandWindow.dwrite = dwrite;

    CommandLoader cmdLoader;
    registerBasicCommands(&cmdLoader);

    Array<Command*> cmds = cmdLoader.loadFromFile(L"D:/Vlad/cb/cmds.ini");
    for (int i = 0; i < cmdLoader.commandInfoArray.count; ++i)
        commandEngine.registerCommandInfo(cmdLoader.commandInfoArray.data[i]);
    for (int i = 0; i < cmds.count; ++i)
        commandEngine.registerCommand(cmds.data[i]);

    bool initialized = commandWindow.init(hInstance, 400, 40);
    assert(initialized);

	String commandLine { lpCmdLine };
	if (commandLine.indexOf(String(L"/noshow")) == -1)
		commandWindow.showAfterAllEventsProcessed(); // Make sure all controls are initialized.

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
