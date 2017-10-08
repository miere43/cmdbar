#include <assert.h>
#include <windowsx.h>

#include "command_window.h"
#include "CommandBar.h"
#include "math.h"
#include "trace.h"
#include "os_utils.h"
#include "one_instance.h"



LRESULT WINAPI commandWindowWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void beforeRunCallback(CommandEngine* engine, void* userdata);

bool CommandWindow::g_globalResourcesInitialized = false;
HICON CommandWindow::g_appIcon = 0;
ATOM CommandWindow::g_windowClass = 0;
HKL CommandWindow::g_englishKeyboardLayout = 0;
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
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.hCursor = LoadCursorW(0, IDC_IBEAM);
    wc.hIcon = g_appIcon;

    g_windowClass = RegisterClassExW(&wc);
    if (g_windowClass == 0)
        return false;

    g_englishKeyboardLayout = LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE);

    return true;
}

void CommandWindow::clearText()
{
    textBuffer[0] = L'\0';
    textBufferLength = 0;

    cursorPos = 0;
    clearSelection();

    this->redraw();
}

bool CommandWindow::setText(const String & text)
{
    if (text.data == nullptr)
        return false;

    int length = text.count;
    if (length > textBufferLength) return false;

    wmemcpy(textBuffer, text.data, length);
    textBufferLength = length;
    cursorPos = length;
    clearSelection();

    redraw();

    return true;
}

void CommandWindow::redraw()
{
    isTextLayoutDirty = true;
    InvalidateRect(hwnd, nullptr, true);
}

void CommandWindow::updateTextLayout(bool forced)
{
    if (textLayout == nullptr || isTextLayoutDirty || forced)
    {
        if (textLayout)
        {
            textLayout->Release();
            textLayout = nullptr;
        }

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        float clientWidth  = static_cast<float>(clientRect.right - clientRect.left);
        float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);

        HRESULT hr;

        hr = dwrite->CreateTextLayout(
            textBuffer,
            textBufferLength,
            textFormat,
            clientWidth,
            clientHeight,
            &this->textLayout
        );

        assert(SUCCEEDED(hr));

        textLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        isTextLayoutDirty = false;
    }
}

void CommandWindow::clearSelection()
{
    selectionPos = 0;
    selectionLength = 0;
}

bool CommandWindow::getSelectionRange(int * rangeStart, int * rangeLength)
{
    assert(rangeStart);
    assert(rangeLength);

    if (selectionLength == 0)
        return false;

    *rangeStart  = selectionPos;
    *rangeLength = selectionLength;

    return true;
}

void CommandWindow::onKeyDown(wchar_t c)
{
    // Skip if not printable.
    if (!iswprint(c))
        return;
    if (isTextBufferFilled())
        return;

    if (insertCharAt(textBuffer, textBufferLength, textBufferMaxLength, cursorPos, c))
    {
        ++textBufferLength;
        ++cursorPos;

        onTextChanged();
        redraw();
    }
}

void CommandWindow::onLeftMouseButtonUp()
{
    selectionStartCursorPos = -1;

    if (GetCapture() == hwnd)
        ReleaseCapture();
}

void CommandWindow::onFocusAcquired()
{
    //originalKeyboardLayout = GetKeyboardLayout(GetCurrentThreadId());
    
    ActivateKeyboardLayout(g_englishKeyboardLayout, KLF_REORDER);
}

void CommandWindow::onFocusLost()
{
    //ActivateKeyboardLayout(originalKeyboardLayout, KLF_REORDER);
    //OutputDebugStringA("lost focus.\n");
    //InvalidateRect(hwnd, nullptr, true);
    hideWindow();
}

void CommandWindow::onTextChanged()
{

}

LRESULT CommandWindow::paint(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ID2D1HwndRenderTarget* rt = this->hwndRenderTarget;
    assert(rt);

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    float clientWidth  = static_cast<float>(clientRect.right - clientRect.left);
    float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);
    float cursorUpY = style->borderSize + 3.0f;
    float cursorBottomY = clientHeight - style->borderSize - 3.0f;

    float marginX = style->textMarginLeft + style->borderSize;

    updateTextLayout();

    D2D1_RECT_F textboxRect = D2D1::RectF(
        style->borderSize,
        style->borderSize,
        clientWidth  - style->borderSize,
        clientHeight - style->borderSize
    );

    rt->BeginDraw();
    rt->Clear(style->borderColor);

    rt->FillRectangle(textboxRect, textboxBackgroundBrush);

    float cursorRelativeX = 0.0f, cursorRelativeY = 0.0f;

    DWRITE_HIT_TEST_METRICS metrics;
    textLayout->HitTestTextPosition(cursorPos, false, &cursorRelativeX, &cursorRelativeY, &metrics);

    // Draw selection background
    int selectionStart = 0;
    int selectionLength = 0;
    if (getSelectionRange(&selectionStart, &selectionLength))
    {
        float unused;
        float selectionRelativeStartX = 0.0f;
        float selectionRelativeEndX = 0.0f;

        textLayout->HitTestTextPosition(selectionStart, false, &selectionRelativeStartX, &unused, &metrics);
        textLayout->HitTestTextPosition(selectionStart + selectionLength, false, &selectionRelativeEndX, &unused, &metrics);

        rt->FillRectangle(
            D2D1::RectF(
                marginX + selectionRelativeStartX,
                style->borderSize, 
                marginX + selectionRelativeEndX,
                clientHeight - style->borderSize), 
            selectedTextBrush);
    }

    float textDrawY = style->borderSize;
    rt->DrawTextLayout(D2D1::Point2F(marginX, textDrawY), textLayout, textForegroundBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

    // Center cursor X at pixel center to disable anti-aliasing.
    float cursorActualX = floor(marginX + cursorRelativeX) + 0.5f;
    rt->DrawLine(D2D1::Point2F(cursorActualX, cursorUpY), D2D1::Point2F(cursorActualX, cursorBottomY), textForegroundBrush);

    rt->EndDraw();

    ValidateRect(hwnd, nullptr);
    return 0;
}

bool CommandWindow::init(HINSTANCE hInstance, int windowWidth, int windowHeight)
{
    if (isInitialized)
        return true;

    assert(windowWidth > 0);
    assert(windowHeight > 0);
    assert(style);
    assert(d2d1);
    assert(dwrite);
    assert(commandEngine);

    isInitialized = false;
    if (!initGlobalResources(hInstance))
        return false;

    const uint32_t mainWindowFlags = WS_POPUP;
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

    textBufferMaxLength = 512;
    textBufferLength = 0;
    textBuffer = static_cast<wchar_t*>(malloc(textBufferMaxLength * sizeof(textBuffer[0])));
    if (textBuffer == nullptr)
        return false;

    {
        // Initialize Direct2D and DirectWrite resources.
        HRESULT hr = 0;

        hr = dwrite->CreateTextFormat(
            style->fontFamily.data,
            nullptr,
            style->fontWeight,
            style->fontStyle,
            style->fontStretch,
            style->fontHeight,
            L"en-us",
            &textFormat
        );
        assert(SUCCEEDED(hr));

        textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        D2D1_SIZE_U clientPixelSize = D2D1::SizeU(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

        hr = d2d1->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, clientPixelSize, D2D1_PRESENT_OPTIONS_IMMEDIATELY),
            &hwndRenderTarget);
        assert(SUCCEEDED(hr));

        hr = hwndRenderTarget->CreateSolidColorBrush(style->textColor, &textForegroundBrush);
        assert(SUCCEEDED(hr));

        hr = hwndRenderTarget->CreateSolidColorBrush(style->selectedTextBackgroundColor, &selectedTextBrush);
        assert(SUCCEEDED(hr));

        hr = hwndRenderTarget->CreateSolidColorBrush(style->textboxBackgroundColor, &textboxBackgroundBrush);
        assert(SUCCEEDED(hr));
    }

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

    SetWindowPos(
        hwnd,
        HWND_TOPMOST, 
        desktopWidth / 2 - windowWidth / 2,
        static_cast<int>(desktopHeight * showWindowYRatio), 
        0, 
        0, 
        SWP_NOSIZE | SWP_SHOWWINDOW
    );

    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
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
    clearText();
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
    String expr{ textBuffer, textBufferLength };

    if (!commandEngine->evaluate(expr))
    {
        MessageBoxW(hwnd, L"Unknown command", L"Error", MB_OK | MB_ICONERROR);
        SetFocus(hwnd); // We lose focus after MessageBox.s
        redraw();

        return;
    }

    clearText();

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

        case WM_CHAR:
        {
            this->onKeyDown(static_cast<wchar_t>(wParam));
            return 0;
        }

        case WM_KEYDOWN:
        {
            BOOL shiftPressed = GetKeyState(VK_LSHIFT) & 0x8000;

            if (wParam == VK_BACK) // Backspace
            {
                if (textBufferLength <= 0) return 0;
                if (selectionLength != 0)
                {
                    if (removeRange(textBuffer, textBufferLength, selectionPos, selectionLength))
                    {
                        textBufferLength -= selectionLength;
                        textBuffer[textBufferLength] = L'\0';
                        if (selectionInitialPos <= cursorPos)
                            cursorPos -= selectionLength;
                        selectionLength = 0;
                    }
                }
                else if (cursorPos != 0 && removeCharAt(textBuffer, textBufferLength, cursorPos - 1))
                {
                    --textBufferLength;
                    textBuffer[textBufferLength] = L'\0';
                    --cursorPos;
                }

                redraw();
            }
            else if (wParam == VK_DELETE)
            {
                if (textBufferLength <= 0) return 0;

                if (selectionLength != 0)
                {
                    if (removeRange(textBuffer, textBufferLength, selectionPos, selectionLength))
                    {
                        textBufferLength -= selectionLength;
                        textBuffer[textBufferLength] = L'\0';
                        if (selectionInitialPos <= cursorPos)
                            cursorPos -= selectionLength;
                        selectionLength = 0;
                    }
                }
                else if (removeCharAt(textBuffer, textBufferLength, cursorPos))
                {
                    textBuffer[textBufferLength] = '\0';
                    --textBufferLength;
                }

                redraw();
            }
            else if (wParam == VK_RETURN)
            {
                evaluate();
            }
            else if (wParam == VK_RIGHT)
            {
                if (cursorPos < textBufferLength)
                {
                    if (shiftPressed) // High-order bit == 1 => key down
                    {
                        if (selectionLength == 0)
                        {
                            selectionInitialPos = selectionPos = cursorPos;
                            selectionLength = 1;
                        }
                        else
                        {
                            if (cursorPos <= selectionInitialPos)
                            {
                                selectionPos = cursorPos + 1;
                                --selectionLength;
                            }
                            else
                            {
                                ++selectionLength;
                            }
                        }
                    }

                    ++cursorPos;
                    redraw();
                }

                if (!shiftPressed)
                {
                    if (selectionLength != 0)
                    {
                        cursorPos = selectionPos + selectionLength;
                    }

                    selectionLength = 0;
                    isTextLayoutDirty = true;
                    InvalidateRect(hwnd, nullptr, true);
                }
            }
            else if (wParam == VK_LEFT)
            {
                if (cursorPos > 0)
                {
                    --cursorPos;

                    if (shiftPressed)
                    {
                        if (selectionLength == 0)
                        {
                            selectionInitialPos = selectionPos = cursorPos;
                            selectionLength = 1;
                        }
                        else
                        {
                            if (cursorPos >= selectionInitialPos)
                            {
                                OutputDebugStringW(L"hello!\n");
                                //selectionPos = cursorPos;
                                --selectionLength;
                            }
                            else
                            {
                                selectionPos = cursorPos;
                                ++selectionLength;
                            }
                        }
                    }

                    isTextLayoutDirty = true;
                    InvalidateRect(hwnd, nullptr, true);
                }

                if (!shiftPressed)
                {
                    if (selectionLength != 0)
                    {
                        cursorPos = selectionPos;
                    }

                    selectionLength = 0;
                    isTextLayoutDirty = true;
                    InvalidateRect(hwnd, nullptr, true);
                }
            }

            return 0;
        }

        case WM_MOUSEMOVE:
        {
            int isLeftMouseDown = wParam & 0x0001;

            if (isLeftMouseDown && selectionStartCursorPos != -1)
            {
                SetCapture(hwnd);

                int mouseX = GET_X_LPARAM(lParam);
                int mouseY = GET_Y_LPARAM(lParam);

                updateTextLayout();

                BOOL isInside = false;
                BOOL isTrailingHit = false;
                DWRITE_HIT_TEST_METRICS metrics ={ 0 };
                HRESULT hr = 0;

                hr = textLayout->HitTestPoint(mouseX - style->textMarginLeft - style->borderSize, mouseY, &isTrailingHit, &isInside, &metrics);
                assert(SUCCEEDED(hr));

                if (selectionStartCursorPos != -1)
                {
                    int oldCursorPos = selectionStartCursorPos;

                    if (isTrailingHit)
                    {
                        cursorPos = metrics.textPosition + 1;
                    }
                    else
                    {
                        cursorPos = metrics.textPosition;
                    }

                    int calcSelectionLength = abs(oldCursorPos - cursorPos);
                    if (calcSelectionLength == 0)
                    {
                        clearSelection();
                    }
                    else
                    {
                        selectionInitialPos = selectionStartCursorPos;
                        selectionPos = min(cursorPos, oldCursorPos);
                        selectionLength = calcSelectionLength;
                    }
                }

                redraw();
            }

            return 0;
        }

        case WM_LBUTTONUP:
        {
            this->onLeftMouseButtonUp();
            return 0;
        }

        case WM_MOUSEACTIVATE:
        {
            if (GetFocus() != hwnd)
            {
                SetFocus(hwnd);
                redraw();
            }

            return MA_ACTIVATE;
        }

        case WM_SETFOCUS:
        {
            this->onFocusAcquired();
        	break;
        }

        case WM_KILLFOCUS:
        {
            this->onFocusLost();
        	return 0;
        }

        case WM_GETTEXT:
        {
            int maxCharsToCopy = ((int)wParam) - 1;
            wchar_t * buffer = (wchar_t *)lParam;

            int numCharsToCopy = min(maxCharsToCopy, textBufferLength);
            memcpy(buffer, textBuffer, sizeof(wchar_t *) * numCharsToCopy);
            buffer[numCharsToCopy] = L'\0';

            return (LRESULT)numCharsToCopy;
        }
        case WM_GETTEXTLENGTH:
        {
            return (LRESULT)textBufferLength;
        }

        case WM_LBUTTONDOWN:
        {
            int mouseX = GET_X_LPARAM(lParam);
            int mouseY = GET_Y_LPARAM(lParam);

            updateTextLayout(false);

            BOOL isInside = false;
            BOOL isTrailingHit = false;
            DWRITE_HIT_TEST_METRICS metrics ={ 0 };
            HRESULT hr = 0;

            hr = textLayout->HitTestPoint(mouseX - style->textMarginLeft - style->borderSize, mouseY, &isTrailingHit, &isInside, &metrics);
            assert(SUCCEEDED(hr));

            if (isTrailingHit)
            {
                cursorPos = metrics.textPosition + 1;
            }
            else
            {
                cursorPos = metrics.textPosition;
            }

            selectionStartCursorPos = cursorPos;

            clearSelection();
            redraw();

            return 0;
        }

        case WM_PAINT:
        {
            return paint(hwnd, msg, wParam, lParam);
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