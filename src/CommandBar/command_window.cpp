#include <assert.h>

#include "command_window.h"
#include "CommandBar.h"
#include "trace.h"
#include "os_utils.h"
#include "one_instance.h"



LRESULT WINAPI commandWindowWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void beforeRunCallback(CommandEngine* engine, void* userdata);

bool CommandWindow::g_globalResourcesInitialized = false;
HICON CommandWindow::g_appIcon = 0;
ATOM CommandWindow::g_windowClass = 0;
const wchar_t* CommandWindow::g_windowName = L"Command Bar";
const wchar_t* CommandWindow::g_className = L"CommandWindow";

enum class CustomMessage
{
    ShowAfterAllEventsProcessed = WM_USER + 14
};

enum class TrayMenuItem
{
    None = 0,
    Show = 1,
    Exit = 2,
};

bool CommandWindow::initGlobalResources(HINSTANCE hInstance)
{
    if (g_globalResourcesInitialized)
        return true;

    g_globalResourcesInitialized = true;

    if (g_appIcon == 0)
    {
        g_appIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APPTRAYICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);

        if (g_appIcon == 0)
            g_appIcon = LoadIconW(0, IDI_APPLICATION);
    }

    WNDCLASSEXW wc ={ 0 };
    wc.cbSize = sizeof(wc);
    wc.lpszClassName = g_className;
    wc.hInstance = hInstance;
    wc.lpfnWndProc = commandWindowWndProc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon = g_appIcon;

    g_windowClass = RegisterClassExW(&wc);
    if (g_windowClass == 0)
        return false;

    return true;
}

bool CommandWindow::init(HINSTANCE hInstance)
{
    if (isInitialized)
        return true;

    assert(style);
    assert(windowWidth > 0);
    assert(windowHeight > 0);
    assert(d2d1);
    assert(dwrite);
    assert(commandEngine);

    isInitialized = false;
    if (!initGlobalResources(hInstance))
        return false;

    enum
    {
        mainWindowMarginValue = 5
    };

    DWORD mainWindowFlags = WS_POPUP;
    hwnd = CreateWindowExW(
        0,
        (LPCWSTR)g_windowClass,
        L"Command Bar",
        mainWindowFlags,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowWidth,
        windowHeight,
        0,
        0,
        hInstance,
        0);

    if (hwnd == 0)
        return false;

    SetWindowLongW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    textbox.d2d1 = d2d1;
    textbox.dwrite = dwrite;

    if (!textbox.init(hInstance, this, mainWindowMarginValue, mainWindowMarginValue, this->windowWidth - mainWindowMarginValue * 2, this->windowHeight - mainWindowMarginValue * 2))
        return false;

    if (!taskbarIcon.addToStatusArea(hwnd, g_appIcon, 1, WM_USER + 15))
    {
        // We can live without it.
    }

    if (!RegisterHotKey(hwnd, 0, MOD_ALT | MOD_NOREPEAT, 0x51))
        MessageBoxW(hwnd, L"Hotkey Alt+Q is already claimed.", L"Command Bar", MB_OK);

    if ((trayMenu = CreatePopupMenu()) != 0)
    {
        AppendMenuW(trayMenu, MF_STRING, (UINT_PTR)TrayMenuItem::Show, L"Show");
        AppendMenuW(trayMenu, MF_SEPARATOR, (UINT_PTR)TrayMenuItem::None, nullptr);
        AppendMenuW(trayMenu, MF_STRING, (UINT_PTR)TrayMenuItem::Exit, L"Exit");
    }

    commandEngine->setBeforeRunCallback(beforeRunCallback, this);

    textbox.setText(String(L"unsigned"));

    isInitialized = true;
    return isInitialized;
}

void CommandWindow::showWindow()
{
    if (IsWindowVisible(hwnd))
        return;

    int desktopWidth  = GetSystemMetrics(SM_CXFULLSCREEN);
    int desktopHeight = GetSystemMetrics(SM_CYFULLSCREEN);

    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);

    int windowWidth = windowRect.right - windowRect.left;
    SetWindowPos(hwnd, HWND_TOPMOST, desktopWidth / 2 - windowWidth / 2, (int)(desktopHeight * showWindowYRatio), 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);

    SetForegroundWindow(hwnd);
    SetFocus(textbox.hwnd);
}

void CommandWindow::showAfterAllEventsProcessed()
{
    // Why we want to do this:
    // when we show window for the first time and there are no messages
    // processed by event loop (GetMessage function), we don't display
    // textbox widget because it's message is not yet processed.
    // This leads to textbox 'popping up' after main window has initialized.
    PostMessageW(hwnd, (UINT)CustomMessage::ShowAfterAllEventsProcessed, 0, 0);
}

void CommandWindow::hideWindow()
{
    ShowWindow(hwnd, SW_HIDE);
    textbox.setText(String(L"", 0));
}

void CommandWindow::toggleVisibility()
{
    if (IsWindowVisible(hwnd))
        hideWindow();
    else
        showWindow();
}

void CommandWindow::exit()
{
    PostQuitMessage(0);
}

int CommandWindow::enterEventLoop()
{
    MSG msg;
    uint32_t ret;

    while ((ret = GetMessageW(&msg, NULL, 0, 0)) != 0)
    {
        if (ret == -1)
        {
            if (shouldCatchInvalidUsageErrors)
            {
                String error = OSUtils::formatErrorCode(GetLastError());
                __debugbreak();
                g_standardAllocator.deallocate(error.data);
            }
            else
            {
                continue;
            }
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    dispose();

    return 0;
}

void CommandWindow::evaluate()
{
    String empty{ L"", 0 };
    String expr{ textbox.textBuffer, (int)textbox.textBufferLength };

    if (!commandEngine->evaluate(expr))
    {
        MessageBoxW(hwnd, L"Unknown command", L"Error", MB_OK | MB_ICONERROR);
        SetFocus(textbox.hwnd); // We lose focus after MessageBox.s
        textbox.redraw();

        return;
    }

    textbox.clearText();

    this->hideWindow();
    //CB_TipHideWindow(&ui.tip);
}

void CommandWindow::dispose()
{
    taskbarIcon.deleteFromStatusArea();

    DeleteMenu(trayMenu, 0, 0);
    trayMenu = 0;
}

LRESULT CommandWindow::wndProc(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam)
{
    switch (msg)
    {
        case WM_ERASEBKGND:
        {
            return 1;
        }

        case WMAPP_TEXTBOX_TEXT_CHANGED:
        {
            if ((HWND)lParam == textbox.hwnd)
            {
                //static wchar_t temp[512];
                //int textLength = GetWindowTextW(textbox.hwnd, temp, sizeof(temp) / sizeof(wchar_t));
                //CB_TipSetInputText(&ui.tip, temp, textLength);
            }

            return 0;
        }

        case WMAPP_TEXTBOX_ENTER_PRESSED:
        {
            if ((HWND)lParam == textbox.hwnd)
            {
                evaluate();
            }

            return 0;
        }

        case WM_HOTKEY:
        {
            if (wParam == 0)
            {
                if (IsWindowVisible(hwnd))
                {
                    hideWindow();
                    //CB_TipHideWindow(&ui.tip);
                }
                else
                {
                    showWindow();
                }
            }

            return 0;
        }

        case (UINT)CustomMessage::ShowAfterAllEventsProcessed:
        {
            showWindow();
            return 0;
        }

        case g_oneInstanceMessage:
        {
            showWindow();
            return 0;
        }
    }

    // Handle tray icon context menu.
    if (taskbarIcon.isContextMenuRequested(hwnd, msg, lParam, wParam))
    {
        int x, y;
        if (!taskbarIcon.getMousePosition(hwnd, msg, lParam, wParam, &x, &y))
            return 0;

        DWORD flags = TPM_NONOTIFY | TPM_RETURNCMD;

        bool isLeftAligned = GetSystemMetrics(SM_MENUDROPALIGNMENT) == 0;
        flags = flags | (isLeftAligned ? TPM_LEFTALIGN | TPM_HORPOSANIMATION : TPM_RIGHTALIGN | TPM_HORNEGANIMATION);

        // MSDN: To display a context menu for a notification icon, the current window must be the
        // foreground window before the application calls TrackPopupMenu or TrackPopupMenuEx
        SetForegroundWindow(hwnd);
        TrayMenuItem item = (TrayMenuItem)TrackPopupMenuEx(trayMenu, flags, x, y, hwnd, nullptr);

        switch (item)
        {
            case TrayMenuItem::Show: showWindow(); break;
            case TrayMenuItem::Exit: exit(); break;
            case TrayMenuItem::None: break;
            default:                 Trace::debug("Unknown TrayMenuItem selected."); return 0;
        }

        return 0;
    }
    else if (taskbarIcon.isClicked(hwnd, msg, lParam, wParam))
    {
        toggleVisibility();
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void CommandWindow::beforeCommandRun()
{
    hideWindow();
}

LRESULT WINAPI commandWindowWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CREATE)
        return 0;

    CommandWindow* window = (CommandWindow*)GetWindowLongW(hwnd, GWLP_USERDATA);

    if (window != nullptr)
        return window->wndProc(hwnd, msg, lParam, wParam);

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void beforeRunCallback(CommandEngine * engine, void * userdata)
{
    if (userdata != nullptr)
        ((CommandWindow*)userdata)->beforeCommandRun();
}


//LARGE_INTEGER g_mainStartupTick;
//LARGE_INTEGER g_mainTickFrequency;
//
//
//CB_TipUI_AutocompletionStyle g_defaultAutocompletionStyle;
//CB_TextboxStyle g_defaultTextboxStyle;
//
//
//static struct {
//	HWND mainWindow;
//	bool shouldQuit;
//
//	CB_Context* context;
//
//	HBRUSH editControlBackgroundBrush;
//	HBRUSH editControlTextBrush;
//
//	CB_TipUI tip;
//	CB_TextboxUI textbox;
//	HICON icon = 0;
//
//	TaskbarIcon taskbarIcon;
//} ui;
//

//	int inputWidth = 300;
//	int inputHeight = 20;
//	
//	//HDC hdc = GetDC(hWnd);
//	//LOGFONTW logFont;
//	//memset(&logFont, 0, sizeof(logFont));
//	//memcpy(logFont.lfFaceName, L"Segoe UI", sizeof(L"Segoe UI"));
//	//logFont.lfWeight = FW_REGULAR;
//	//logFont.lfHeight = 20;
//	//HFONT font = CreateFontIndirectW(&logFont);
//	//SendMessageW(input, WM_SETFONT, (WPARAM)font, TRUE);
//	//ReleaseDC(hWnd, hdc);
//
//	ui.editControlBackgroundBrush = CreateSolidBrush(RGB(230, 230, 230));
//	ui.editControlTextBrush = CreateSolidBrush(RGB(29, 29, 29));
//
//	ui.taskbarIcon.addToTray(hWnd, ui.icon);
//
//

//	if (i == 0) {
//		MessageBoxA(hWnd, "u are running two instances of this app baka", "kek", MB_OK);
//	}
//
//	if (!CB_TipCreateWindow(&ui.tip, ui.mainWindow, hInstance, inputWidth, 200)) {
//		return 1;
//	}
//	if (!CB_TextboxCreateWindow(&ui.textbox, ui.mainWindow, hInstance, mainWindowMarginValue, mainWindowMarginValue, 300, 20)) {
//		return 1;
//	}
//
//	CB_TipSetAutocompletionStyle(&ui.tip, &g_defaultAutocompletionStyle);
//	CB_TipSetShowOffset(&ui.tip, mainWindowMarginValue, mainWindowMarginValue + inputHeight);
//	CB_TipSetCommandDefVector(&ui.tip, &ui.context->commands);
//
//	CB_TextboxSetStyle(&ui.textbox, &g_defaultTextboxStyle);
//
//	UpdateWindow(hWnd);
//
//	//if (fileMapping != NULL)
//	//{
//	//	void* map = MapViewOfFile(fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 32);
//	//	if (!map)
//	//	{
//	//		MessageBoxW(ui.mainWindow, L"Unable to map", L"Error", MB_OK);
//	//	}
//	//	else
//	//	{
//	//		*((HWND*)map) = ui.mainWindow;
//	//		UnmapViewOfFile(map);
//	//	}
//	//	CloseHandle(fileMapping);
//	//}
//	CB_LoadUIOptions();
//
//#if _DEBUG
//	CB_ShowUI();
//#endif
//
//	LARGE_INTEGER lastTick;
//	QueryPerformanceCounter(&lastTick);
//
//	double startupSeconds = ((lastTick.QuadPart - g_mainStartupTick.QuadPart) / (double)g_mainTickFrequency.QuadPart);
//	
//	char ss[64];
//	sprintf(ss, "startup: %f\n", startupSeconds);
//	OutputDebugStringA(ss);
//
//	MSG msg;
//	ui.shouldQuit = 0;
//	int ret;
//	while (!ui.shouldQuit && (ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
//		if (ret == -1) {
//			//int err = GetLastError();
//			//__debugbreak();
//		}
//		TranslateMessage(&msg);
//		DispatchMessageW(&msg);
//	}
//
//	return 0;
//}
//
//void InitStyles()
//{
//	{
//		CB_TipUI_AutocompletionStyle * s = &g_defaultAutocompletionStyle;
//		ZeroMemory(s, sizeof(*s));
//
//		s->backgroundColor = RGB(64, 64, 64);
//		s->itemDrawFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
//		s->itemMarginX = 5;
//		s->itemMarginY = 5;
//		s->itemMaxHeight = 100;
//		s->marginBottom = 5;
//		s->textColor = RGB(230, 230, 230);
//	}
//}