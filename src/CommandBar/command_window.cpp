#include <assert.h>
#include <windowsx.h>
#include <math.h>

#include "command_window.h"
#include "CommandBar.h"
#include "math_utils.h"
#include "os_utils.h"
#include "basic_commands.h"
#include "clipboard.h"
#include "string_utils.h"
#include "edit_commands_window.h"
#include "defer.h"
#include "command_window_tray.h"

LRESULT WINAPI commandWindowWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void beforeRunCallback(CommandEngine* engine, void* userdata);

bool CommandWindow::g_globalResourcesInitialized = false;
HICON CommandWindow::g_appIcon = 0;
ATOM CommandWindow::g_windowClass = 0;
HKL CommandWindow::g_englishKeyboardLayout = 0;
const wchar_t* CommandWindow::g_windowName = L"Command Bar";
const wchar_t* CommandWindow::g_className = L"CommandWindow";
const UINT CommandWindow::g_showWindowMessageId = WM_USER + 64;

enum
{
    SHOW_APP_WINDOW_HOTKEY_ID = 0,
    CURSOR_BLINK_TIMER_ID = 0
};

bool CommandWindow::initGlobalResources(HINSTANCE hInstance)
{
    if (g_globalResourcesInitialized)
        return true;

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

    g_globalResourcesInitialized = true;
    return true;
}

void CommandWindow::ClearText()
{
    textBuffer.count = 0;
    cursorPos = 0;

    autocompletionCandidate = nullptr;

    ClearSelection();
    Redraw();
}

bool CommandWindow::SetText(const Newstring& text)
{
    textBuffer.count = 0;
    
    if (Newstring::IsNullOrEmpty(text))
    {
        ClearText();
        return true;
    }
    else
    {
        textBuffer.Append(text);
    }
    
    autocompletionCandidate = nullptr;

    ClearSelection();
    Redraw();

    return true;
}

void CommandWindow::Redraw()
{
    isTextLayoutDirty = true;
    InvalidateRect(hwnd, nullptr, true);
}

void CommandWindow::UpdateTextLayout(bool forced)
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

        int spaceIndex = textBuffer.string.IndexOf(L' ');

        hr = dwrite->CreateTextLayout(
            textBuffer.data,
            textBuffer.count,
            textFormat,
            clientWidth,
            clientHeight,
            &this->textLayout
        );

        assert(SUCCEEDED(hr));

        textLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        //if (autocompletionCandidate != nullptr)
        //{
        //    //assert(textBuffer.startsWith(
        //    //    autocompletionCandidate->name,
        //    //    math::min(textBuffer.count, autocompletionCandidate->name.count), 
        //    //    StringComparison::CaseInsensitive));
        //}

        DWRITE_TEXT_RANGE range;
        if (spaceIndex != -1)
            range = { 0, static_cast<UINT32>(spaceIndex) };
        else
            range = { 0, static_cast<UINT32>(textBuffer.count) };

        textLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, range);

        isTextLayoutDirty = false;
    }
}

void CommandWindow::ClearSelection()
{
    selectionPos = 0;
    selectionLength = 0;
}

LRESULT CommandWindow::OnChar(wchar_t c)
{
    // Skip if not printable.
    if (!iswprint(c)) return 0;

    if (IsTextSelected())
    {
        textBuffer.Remove(selectionPos, selectionLength);
        textBuffer.Insert(selectionPos, c);

        cursorPos = selectionPos + 1;
        ClearSelection();
    }
    else
    {
        textBuffer.Insert(cursorPos, c);
        ++cursorPos;
    }

    onTextChanged();
    shouldDrawCursor = true;
    setCursorTimer();

    Redraw();

    return 0;
}

LRESULT CommandWindow::OnKeyDown(LPARAM lParam, WPARAM vk)
{
    BOOL shiftPressed = GetKeyState(VK_LSHIFT) & 0x8000;
    BOOL ctrlPressed = GetKeyState(VK_LCONTROL) & 0x8000;

    switch (vk)
    {
        case VK_ESCAPE:
        {
            if (IsWindowVisible(hwnd))
            {
                HideWindow();
            }

            break;
        }
        case VK_TAB:
        {
            onUserRequestedAutocompletion();
            break;
        }
        // Backspace
        // Delete
        case VK_BACK:
        case VK_DELETE:
        {
            shouldDrawCursor = true;
            setCursorTimer();

            if (IsTextSelected())
            {
                textBuffer.Remove(selectionPos, selectionLength);
                if (selectionInitialPos <= cursorPos)
                    cursorPos -= selectionLength;
                selectionLength = 0;
            }
            else
            {
                if (vk == VK_BACK && cursorPos != 0)
                {
                    textBuffer.Remove(cursorPos - 1, 1);
                    --cursorPos;
                }
                else if (vk == VK_DELETE && cursorPos < textBuffer.count)
                {
                    textBuffer.Remove(cursorPos, 1);
                }
            }

            onTextChanged();
            Redraw();

            break;
        }

        case VK_RETURN:
        {
            Evaluate();

            break;
        }

        case VK_RIGHT:
        {
            shouldDrawCursor = true;
            setCursorTimer();

            if (cursorPos < textBuffer.count)
            {
                if (shiftPressed) // High-order bit == 1 => key down
                {
                    if (!IsTextSelected())
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
                Redraw();
            }

            if (!shiftPressed)
            {
                if (IsTextSelected())
                {
                    cursorPos = selectionPos + selectionLength;
                }

                selectionLength = 0;

                Redraw();
            }

            break;
        }

        case VK_LEFT:
        {
            shouldDrawCursor = true;
            setCursorTimer();

            if (cursorPos > 0)
            {
                --cursorPos;

                if (shiftPressed)
                {
                    if (!IsTextSelected())
                    {
                        selectionInitialPos = selectionPos = cursorPos;
                        selectionLength = 1;
                    }
                    else
                    {
                        if (cursorPos >= selectionInitialPos)
                        {
                            //OutputDebugStringW(L"hello!\n");
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

                Redraw();
            }

            if (!shiftPressed)
            {
                if (selectionLength != 0)
                {
                    cursorPos = selectionPos;
                }

                selectionLength = 0;

                Redraw();
            }
        }

        break;
    }

    if (ctrlPressed && vk == L'C')
    {
        if (!IsTextSelected())
            return 0;
        if (!Clipboard::Open(hwnd))
            assert(false);

        Newstring copyStr = textBuffer.string.RefSubstring(selectionPos, selectionLength);
        bool copied = Clipboard::CopyText(copyStr);
        assert(copied);

        Clipboard::Close();

        onTextChanged();
        shouldDrawCursor = true;
        setCursorTimer();

        Redraw();
    }
    else if (ctrlPressed && vk == L'V')
    {
        if (!Clipboard::Open(hwnd))
            assert(false);

        Newstring textToCopy;
        bool copied = Clipboard::GetText(&textToCopy);
        assert(copied);

        defer(textToCopy.Dispose());

        Clipboard::Close();

        if (Newstring::IsNullOrEmpty(textToCopy))
        {
            return 0;
        }

        if (IsTextSelected())
        {
            textBuffer.Remove(selectionPos, selectionLength);
            textBuffer.Insert(selectionPos, textToCopy);

            cursorPos = selectionPos + textToCopy.count;
            ClearSelection();

            onTextChanged();
            shouldDrawCursor = true;
            setCursorTimer();

            Redraw();
        }
        else
        {
            textBuffer.Insert(cursorPos, textToCopy);

            cursorPos += textToCopy.count;

            onTextChanged();
            shouldDrawCursor = true;
            setCursorTimer();

            Redraw();
        }

        return 0;
    }

    return 0;
}

LRESULT CommandWindow::OnMouseActivate()
{
    if (GetFocus() != hwnd)
    {
        SetFocus(hwnd);
        Redraw();
    }

    return MA_ACTIVATE;
}

LRESULT CommandWindow::OnLeftMouseButtonDown(LPARAM lParam, WPARAM wParam)
{
    float mouseX = static_cast<float>(GET_X_LPARAM(lParam));
    float mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
    const float borderSize = static_cast<float>(style->borderSize);

    UpdateTextLayout();
    shouldDrawCursor = true;
    setCursorTimer();

    BOOL isInside = false;
    BOOL isTrailingHit = false;
    DWRITE_HIT_TEST_METRICS metrics ={ 0 };
    HRESULT hr = 0;

    hr = textLayout->HitTestPoint(mouseX - style->textMarginLeft - borderSize, mouseY, &isTrailingHit, &isInside, &metrics);
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

    ClearSelection();
    Redraw();

    return 0;
}

LRESULT CommandWindow::OnLeftMouseButtonUp()
{
    selectionStartCursorPos = -1;

    if (GetCapture() == hwnd)
        ReleaseCapture();

    return 0;
}

LRESULT CommandWindow::OnFocusAcquired()
{
    //originalKeyboardLayout = GetKeyboardLayout(GetCurrentThreadId());
    
    ActivateKeyboardLayout(g_englishKeyboardLayout, KLF_REORDER);

    shouldDrawCursor = true;
    setCursorTimer();

    return 0;
}

LRESULT CommandWindow::OnFocusLost()
{
    //ActivateKeyboardLayout(originalKeyboardLayout, KLF_REORDER);
    //OutputDebugStringA("lost focus.\n");
    //InvalidateRect(hwnd, nullptr, true);
    HideWindow();
    
    // @TODO: use built-in CreateCaret/ShowCaret/etc. stuff!

    return 0;
}

void CommandWindow::onTextChanged()
{
    shouldDrawCursor = true;
    setCursorTimer();

    updateAutocompletion();
}

LRESULT CommandWindow::OnMouseMove(LPARAM lParam, WPARAM wParam)
{
    bool isLeftMouseDown = static_cast<bool>(wParam & 0x0001);

    if (!isLeftMouseDown || selectionStartCursorPos == -1)
        return 0;

    SetCapture(hwnd);

    float mouseX = static_cast<float>(GET_X_LPARAM(lParam));
    float mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
    const float borderSize = static_cast<float>(style->borderSize);

    UpdateTextLayout();

    BOOL isInside = false;
    BOOL isTrailingHit = false;
    DWRITE_HIT_TEST_METRICS metrics ={ 0 };
    HRESULT hr = 0;

    hr = textLayout->HitTestPoint(mouseX - style->textMarginLeft - borderSize, mouseY, &isTrailingHit, &isInside, &metrics);
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

        int calcSelectionLength = abs(oldCursorPos - (int)cursorPos);
        if (calcSelectionLength == 0)
        {
            ClearSelection();
        }
        else
        {
            selectionInitialPos = selectionStartCursorPos;
            selectionPos = math::min((int)cursorPos, oldCursorPos);
            selectionLength = calcSelectionLength;
        }
    }

    Redraw();

    return 0;
}

void CommandWindow::onUserRequestedAutocompletion()
{
    if (autocompletionCandidate == nullptr)
    {
        autocompletionCandidate = findAutocompletionCandidate();
        if (autocompletionCandidate == nullptr)
            return;
    }

    // @TODO: textBuffer may not have enough storage.
    wmemcpy(textBuffer.data, autocompletionCandidate->name.data, autocompletionCandidate->name.count);
    textBuffer.count = autocompletionCandidate->name.count;
    textBuffer.data[textBuffer.count] = L' ';
    textBuffer.count += 1;

    ClearSelection();
    cursorPos = textBuffer.count;

    Redraw();

    return;
}

Command* CommandWindow::findAutocompletionCandidate()
{
    if (textBuffer.count == 0)
        return nullptr;

    int index = textBuffer.string.IndexOf(L' ');

    Newstring command;
    command.data  = textBuffer.data;
    command.count = index == -1 ? textBuffer.count : index;

    if (command.count == 0)
        return nullptr;

    for (uint32_t i = 0; i < commandEngine->commands.count; ++i)
    {
        Command* candidate = commandEngine->commands.data[i];
        bool isMatchingCandidate = candidate->name.StartsWith(command, StringComparison::CaseInsensitive);

        if (isMatchingCandidate)
            return candidate;
    }

    return nullptr;
}

void CommandWindow::updateAutocompletion()
{
    Command* newCandidate = findAutocompletionCandidate();
    if (autocompletionCandidate != newCandidate)
    {
        autocompletionCandidate = newCandidate;
        Redraw();
    }
}

void CommandWindow::setCursorTimer()
{
    SetLastError(NO_ERROR);

    uintptr_t timerID = SetTimer(hwnd, CURSOR_BLINK_TIMER_ID, 600, nullptr);
    
    if (timerID == 0 && GetLastError() != NO_ERROR)
    {
        __debugbreak();
    }
}
void CommandWindow::killCursorTimer()
{
    KillTimer(hwnd, CURSOR_BLINK_TIMER_ID);
}

void CommandWindow::AnimateWindow(WindowAnimation animation)
{
    uint64_t clockFrequency;
    uint64_t clockCurrTick;
    QueryPerformanceFrequency((LARGE_INTEGER*)&clockFrequency);
    QueryPerformanceCounter((LARGE_INTEGER*)&clockCurrTick);

    const double animDuration = 0.05;
    double currSecs = clockCurrTick / (double)clockFrequency;
    double targetSecs = currSecs + animDuration;

    MSG msg;
    while (currSecs < targetSecs)
    {
        while (PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE) != 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        QueryPerformanceCounter((LARGE_INTEGER*)&clockCurrTick);
        currSecs = clockCurrTick / (double)clockFrequency;

        if (currSecs >= targetSecs)
        {
            SetLayeredWindowAttributes(hwnd, 0, animation == WindowAnimation::Show ? 255 : 0, LWA_ALPHA);
            break;
        }
        else
        {
            double diff = ((targetSecs - currSecs) * (1.0 / animDuration));
            if (animation == WindowAnimation::Show)
                diff = 1.0 - diff;
            SetLayeredWindowAttributes(hwnd, 0, (BYTE)(diff * 255), LWA_ALPHA);
        }

        Sleep(1);
    }
}
//
//void CommandWindow::setShouldDrawCursor(bool shouldDrawCursor)
//{
//
//}

LRESULT CommandWindow::OnPaint()
{
    ID2D1HwndRenderTarget* rt = this->hwndRenderTarget;
    assert(rt);

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    float clientWidth  = static_cast<float>(clientRect.right - clientRect.left);
    float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);
    float cursorUpY = style->borderSize + 3.0f;
    
    const float borderSize = static_cast<float>(style->borderSize);
    float cursorBottomY = clientHeight - borderSize - 3.0f;

    float marginX = style->textMarginLeft + style->borderSize;

    UpdateTextLayout();

    D2D1_RECT_F textboxRect = D2D1::RectF(
        borderSize,
        borderSize,
        clientWidth  - borderSize,
        clientHeight - borderSize
    );

    rt->BeginDraw();
    rt->Clear(style->borderColor);

    rt->FillRectangle(textboxRect, textboxBackgroundBrush);

    float cursorRelativeX = 0.0f;
    float cursorRelativeY = 0.0f;

    DWRITE_HIT_TEST_METRICS metrics;
    textLayout->HitTestTextPosition(cursorPos, false, &cursorRelativeX, &cursorRelativeY, &metrics);

    // Draw selection background
    if (IsTextSelected())
    {
        float unused;
        float selectionRelativeStartX = 0.0f;
        float selectionRelativeEndX = 0.0f;

        textLayout->HitTestTextPosition(selectionPos, false, &selectionRelativeStartX, &unused, &metrics);
        textLayout->HitTestTextPosition(selectionPos + selectionLength, false, &selectionRelativeEndX, &unused, &metrics);

        rt->FillRectangle(
            D2D1::RectF(
                marginX + selectionRelativeStartX,
                borderSize, 
                marginX + selectionRelativeEndX,
                clientHeight - borderSize), 
            selectedTextBrush);
    }

    float textDrawY = borderSize;
    
    // Draw actual user text
    rt->DrawTextLayout(D2D1::Point2F(marginX, textDrawY), textLayout, textForegroundBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

    if (shouldDrawCursor && !IsTextSelected())
    {
        // Center cursor X at pixel center to disable anti-aliasing.
        float cursorActualX = floorf(marginX + cursorRelativeX) + 0.5f;
        rt->DrawLine(D2D1::Point2F(cursorActualX, cursorUpY), D2D1::Point2F(cursorActualX, cursorBottomY), textForegroundBrush);
    }

    rt->EndDraw();

    ValidateRect(hwnd, nullptr);
    return 0;
}

LRESULT CommandWindow::OnHotkey(LPARAM lParam, WPARAM wParam)
{
    if (wParam == SHOW_APP_WINDOW_HOTKEY_ID)
    {
        if (IsWindowVisible(hwnd))
        {
            HideWindow();
        }
        else
        {
            ShowWindow();
        }
    }

    return 0;
}

LRESULT CommandWindow::OnTimer(LPARAM lParam, WPARAM timerID)
{
    switch (timerID)
    {
        case CURSOR_BLINK_TIMER_ID: return OnCursorBlinkTimerElapsed();
        default:
            __debugbreak(); // Unknown timer.
    }
    
    return 0;
}

LRESULT CommandWindow::OnShowWindow(LPARAM lParam, WPARAM wParam)
{
    // @TODO: this function is weird and called only when window is about to be hidden.

    bool isWindowShown = lParam == 1;
    //OutputDebugStringW(L"onShowWindow " + isWindowShown ? L"true\n" : L"false\n");

    if (isWindowShown)
    {
        setCursorTimer();
        __debugbreak();
    }
    else 
        killCursorTimer();

    return 0;
}

LRESULT CommandWindow::OnQuit()
{
    DestroyWindow(hwnd);

    return 0;
}

LRESULT CommandWindow::OnActivate()
{
    return 0;
}

LRESULT CommandWindow::OnCursorBlinkTimerElapsed()
{
    shouldDrawCursor = !shouldDrawCursor;
    Redraw();

    return 0;
}

bool CommandWindow::Initialize(int windowWidth, int windowHeight, int nCmdShow)
{
    if (isInitialized)
        return true;

    assert(windowWidth  > 0);
    assert(windowHeight > 0);
    assert(style);
    assert(commandEngine);

    HINSTANCE hInstance = GetModuleHandleW(0);

    isInitialized = false;
    if (!initGlobalResources(hInstance))
        return false;

    const uint32_t mainWindowFlags = WS_POPUP;
    const uint32_t mainWindowExtendedFlags = WS_EX_TOOLWINDOW;
    hwnd = CreateWindowExW(
        mainWindowExtendedFlags,
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
        this);

    if (hwnd == 0)
        return false;

    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, GetWindowLongPtrW(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    if (!RegisterHotKey(hwnd, SHOW_APP_WINDOW_HOTKEY_ID, MOD_ALT | MOD_NOREPEAT, 'Q'))
        MessageBoxW(hwnd, L"Hotkey Alt+Q is already claimed.", L"Command Bar", MB_ICONINFORMATION);

    Newstring trayFailureReason;
    tray.Initialize(hwnd, g_appIcon, &trayFailureReason);

    if (!Newstring::IsNullOrEmpty(trayFailureReason))
    {
        MessageBoxW(hwnd, trayFailureReason.data, L"Error", MB_ICONERROR);
    }

    if (!CreateGraphicsResources())
        return false;

    commandEngine->SetBeforeRunCallback(beforeRunCallback, this);

    assert(textBuffer.Reserve(512));

    setCursorTimer();

    QuitCommand* quitcmd = Memnew(QuitCommand);
    quitcmd->name = Newstring::NewFromWChar(L"quit");
    quitcmd->info = nullptr;
    quitcmd->commandWindow = this;
    commandEngine->RegisterCommand(quitcmd);

    QuitCommand* exitcmd = Memnew(QuitCommand);
    exitcmd->name = Newstring::NewFromWChar(L"exit");
    exitcmd->info = nullptr;
    exitcmd->commandWindow = this;
    commandEngine->RegisterCommand(exitcmd);
    
    //EditCommandsWindow* edit = new EditCommandsWindow();
    //edit->init(hwnd, commandEngine);

    if (nCmdShow != 0) ShowWindow();

    isInitialized = true;
    return isInitialized;
}

void CommandWindow::ShowWindow()
{
    if (IsWindowVisible(hwnd))
        return;

    int desktopWidth  = GetSystemMetrics(SM_CXFULLSCREEN);
    int desktopHeight = GetSystemMetrics(SM_CYFULLSCREEN);

    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);

    int windowWidth = windowRect.right - windowRect.left;

    // Change opacity before we display window.
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
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

    AnimateWindow(WindowAnimation::Show);
}

void CommandWindow::HideWindow()
{
    AnimateWindow(WindowAnimation::Hide);

    ::ShowWindow(hwnd, SW_HIDE);
    ClearText();
}

void CommandWindow::ToggleVisibility()
{
    IsWindowVisible(hwnd) ? HideWindow() : ShowWindow();
}

void CommandWindow::Exit()
{
    PostQuitMessage(0);
}

void CommandWindow::Evaluate()
{
    if (!commandEngine->Evaluate(textBuffer.string))
    {
        MessageBoxW(hwnd, L"Unknown command", L"Error", MB_OK | MB_ICONERROR);
        SetFocus(hwnd); // We lose focus after MessageBox
        Redraw();

        return;
    }

    ClearText();
    HideWindow();
    //CB_TipHideWindow(&ui.tip);
}

bool CommandWindow::CreateGraphicsResources()
{
    d2d1 = nullptr;
    dwrite = nullptr;
    const auto format = Newstring::FormatTempCStringWithFallback;

    HRESULT hr = E_UNEXPECTED;
    defer(
        if (FAILED(hr))
        {
            DisposeGraphicsResources();
        }
    );
    
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d1);
    if (FAILED(hr))
    {
        return ShowErrorBox(format(L"Cannot initialize Direct2D.\n\nError code was 0x%08X.", hr));
    }

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&dwrite));
    if (FAILED(hr))
    {
        return ShowErrorBox(format(L"Cannot initialize DirectWrite.\n\nError code was 0x%08X.", hr));
    }

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
    if (FAILED(hr))
    {
        return ShowErrorBox(format(L"Cannot create text format.\n\nError code was 0x%08X.", hr));
    }

    textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    D2D1_SIZE_U clientPixelSize = D2D1::SizeU(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

    hr = d2d1->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd, clientPixelSize, D2D1_PRESENT_OPTIONS_IMMEDIATELY),
        &hwndRenderTarget);
    if (FAILED(hr))
    {
        return ShowErrorBox(format(L"Cannot create window render target.\n\nError code was 0x%08X.", hr));
    }

    HRESULT brushHr = S_OK;
    hr = hwndRenderTarget->CreateSolidColorBrush(style->textColor, &textForegroundBrush);
    if (FAILED(hr))  brushHr = hr;

    hr = hwndRenderTarget->CreateSolidColorBrush(style->autocompletionTextColor, &autocompletionTextForegroundBrush);
    if (FAILED(hr))  brushHr = hr;

    hr = hwndRenderTarget->CreateSolidColorBrush(style->selectedTextBackgroundColor, &selectedTextBrush);
    if (FAILED(hr))  brushHr = hr;

    hr = hwndRenderTarget->CreateSolidColorBrush(style->textboxBackgroundColor, &textboxBackgroundBrush);
    if (FAILED(hr))  brushHr = hr;

    hr = brushHr;

    if (FAILED(hr))
    {
        return ShowErrorBox(format(L"Cannot create brushes.\n\nError code was 0x%08X.", hr));
    }

    return true;
}

void CommandWindow::DisposeGraphicsResources()
{
    SafeRelease(textForegroundBrush);
    SafeRelease(autocompletionTextForegroundBrush);
    SafeRelease(selectedTextBrush);
    SafeRelease(textboxBackgroundBrush);
    SafeRelease(textFormat);
    SafeRelease(hwndRenderTarget);
    SafeRelease(d2d1);
    SafeRelease(dwrite);
}

bool CommandWindow::ShowErrorBox(Newstring msg)
{
    if (Newstring::IsNullOrEmpty(msg))
    {
        // @TODO: Insert debug build debugbreak here.
        msg = Newstring::WrapConstWChar(L"Unknown error.");
    }

    if (!msg.IsZeroTerminated())
    {
        msg = msg.CloneAsCString(&g_tempAllocator);
    }

    MessageBoxW(hwnd, msg.data, L"Error", MB_ICONERROR);
    return false;
}

void CommandWindow::Dispose()
{
    tray.Dispose();
    DisposeGraphicsResources();
}

LRESULT CommandWindow::WindowProc(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam)
{
    switch (msg)
    {
        case WM_ERASEBKGND:     return 1;
        case WM_CHAR:           return this->OnChar(static_cast<wchar_t>(wParam));
        case WM_KEYDOWN:        return this->OnKeyDown(lParam, wParam);
        case WM_MOUSEMOVE:      return this->OnMouseMove(lParam, wParam);
        case WM_LBUTTONUP:      return this->OnLeftMouseButtonUp();
        case WM_LBUTTONDOWN:    return this->OnLeftMouseButtonDown(lParam, wParam);
        case WM_MOUSEACTIVATE:  return this->OnMouseActivate();
        case WM_SETFOCUS:       return this->OnFocusAcquired();
        case WM_KILLFOCUS:      return this->OnFocusLost();
        case WM_PAINT:          return this->OnPaint();
        case WM_HOTKEY:         return this->OnHotkey(lParam, wParam);
        case WM_TIMER:          return this->OnTimer(lParam, wParam);
        case WM_SHOWWINDOW:     return this->OnShowWindow(lParam, wParam);
        case WM_QUIT:           return this->OnQuit();
        case WM_ACTIVATE:       return this->OnActivate();

        case CommandWindow::g_showWindowMessageId:
        {
            ShowWindow();
            return 0;
        }
    }

    TrayMenuAction action = TrayMenuAction::None;
    if (tray.ProcessEvent(hwnd, msg, lParam, wParam, &action))
    {
        switch (action)
        {
            case TrayMenuAction::Show: ShowWindow(); break;
            case TrayMenuAction::Exit: Exit(); break;
            default:                 break;
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void CommandWindow::beforeCommandRun()
{
    HideWindow();
}

LRESULT WINAPI commandWindowWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CommandWindow* window = nullptr;

    if (msg == WM_CREATE)
    {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        if (cs == nullptr)  return 1;

        window = reinterpret_cast<CommandWindow*>(cs->lpCreateParams);
        if (window == nullptr)  return 1;
        
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }
    else
    {
        window = reinterpret_cast<CommandWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    return window ? window->WindowProc(hwnd, msg, lParam, wParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}

void beforeRunCallback(CommandEngine * engine, void * userdata)
{
    if (userdata != nullptr)
        ((CommandWindow*)userdata)->beforeCommandRun();
}
