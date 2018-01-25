#include <assert.h>
#include <windowsx.h>

#include "command_window.h"
#include "CommandBar.h"
#include "math_utils.h"
#include "os_utils.h"
#include "basic_commands.h"
#include "clipboard.h"
#include "string_utils.h"
#include "string_builder.h"
#include "edit_commands_window.h"


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

void CommandWindow::clearText()
{
    textBuffer.data[0] = L'\0';
    textBuffer.count = 0;

    cursorPos = 0;
    clearSelection();

    autocompletionCandidate = nullptr;

    redraw();
}

bool CommandWindow::setText(const String & text)
{
    if (text.data == nullptr)
        return false;

    uint32_t length = text.count;
    if (length > textBuffer.count) return false;

    wmemcpy(textBuffer.data, text.data, length);
    textBuffer.count = length;
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

        int spaceIndex = textBuffer.indexOf(L' ');

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

LRESULT CommandWindow::onChar(wchar_t c)
{
    // Skip if not printable.
    if (!iswprint(c)) return 0;
    if (isTextBufferFilled()) return 0;

    if (isTextSelected())
    {
        bool s = removeRange(textBuffer.data, textBuffer.count, selectionPos, selectionLength);
        if (s)
            textBuffer.count -= selectionLength;
        else
            return 0;

        s = insertCharAt(textBuffer.data, textBuffer.count, textBufferMaxLength, selectionPos, c);
        if (s)
            ++textBuffer.count;
        else
            return 0;

        cursorPos = selectionPos + 1;
        clearSelection();
    
        onTextChanged();
        shouldDrawCursor = true;
        setCursorTimer();

        redraw();
    }
    else
    {
        if (!insertCharAt(textBuffer.data, textBuffer.count, textBufferMaxLength, cursorPos, c))
            return 0;

        ++textBuffer.count;
        ++cursorPos;

        onTextChanged();
        shouldDrawCursor = true;
        setCursorTimer();

        redraw();
    }

    return 0;
}

LRESULT CommandWindow::onKeyDown(LPARAM lParam, WPARAM vk)
{
    BOOL shiftPressed = GetKeyState(VK_LSHIFT) & 0x8000;
    BOOL ctrlPressed = GetKeyState(VK_LCONTROL) & 0x8000;

    switch (vk)
    {
        case VK_ESCAPE:
        {
            if (IsWindowVisible(hwnd))
            {
                hideWindow();
            }

            break;
        }
        case VK_TAB:
        {
            onUserRequestedAutocompletion();
            break;
        }
        // Backspace
        case VK_BACK:
        {
            shouldDrawCursor = true;
            setCursorTimer();

            if (selectionLength != 0)
            {
                if (removeRange(textBuffer.data, textBuffer.count, selectionPos, selectionLength))
                {
                    textBuffer.count -= selectionLength;
                    textBuffer.data[textBuffer.count] = L'\0';
                    if (selectionInitialPos <= cursorPos)
                        cursorPos -= selectionLength;
                    selectionLength = 0;
                }
            }
            else if (cursorPos != 0 && removeCharAt(textBuffer.data, textBuffer.count, cursorPos - 1))
            {
                --textBuffer.count;
                textBuffer.data[textBuffer.count] = L'\0';
                --cursorPos;
            }

            onTextChanged();
            redraw();

            break;
        }

        case VK_DELETE:
        {
            shouldDrawCursor = true;
            setCursorTimer();

            if (selectionLength != 0)
            {
                if (removeRange(textBuffer.data, textBuffer.count, selectionPos, selectionLength))
                {
                    textBuffer.count -= selectionLength;
                    textBuffer.data[textBuffer.count] = L'\0';
                    if (selectionInitialPos <= cursorPos)
                        cursorPos -= selectionLength;
                    selectionLength = 0;
                }
            }
            else if (removeCharAt(textBuffer.data, textBuffer.count, cursorPos))
            {
                textBuffer.data[textBuffer.count] = '\0';
                --textBuffer.count;
            }

            onTextChanged();
            redraw();

            break;
        }

        case VK_RETURN:
        {
            evaluate();

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

                redraw();
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

                redraw();
            }

            if (!shiftPressed)
            {
                if (selectionLength != 0)
                {
                    cursorPos = selectionPos;
                }

                selectionLength = 0;

                redraw();
            }
        }

        break;
    }

    if (ctrlPressed && vk == L'C')
    {
        if (!isTextSelected())
            return 0;
        if (!Clipboard::open(hwnd))
            assert(false);

        String copyStr = textBuffer.substring(selectionPos, selectionLength);
        bool copied = Clipboard::copyText(copyStr.data, copyStr.count);
        assert(copied);

        Clipboard::close();

        onTextChanged();
        shouldDrawCursor = true;
        setCursorTimer();

        redraw();
    }
    else if (ctrlPressed && vk == L'V')
    {
        if (!Clipboard::open(hwnd))
            assert(false);

        String textToCopy = { 0, 0 };
        bool copied = Clipboard::getText(&textToCopy);
        assert(copied);

        Clipboard::close();

        if (textToCopy.isEmpty())
        {
            g_standardAllocator.dealloc(textToCopy.data);
            return 0;
        }

        if (isTextSelected())
        {
            bool s = removeRange(textBuffer.data, textBuffer.count, selectionPos, selectionLength);
            assert(s);

            textBuffer.count -= selectionLength;

            s = StringUtils::insertChars(textBuffer.data, textBuffer.count, textBufferMaxLength, textToCopy.data, textToCopy.count, selectionPos);
            assert(s);

            textBuffer.count += textToCopy.count;

            cursorPos = selectionPos + textToCopy.count;
            clearSelection();

            onTextChanged();
            shouldDrawCursor = true;
            setCursorTimer();

            redraw();
        }
        else
        {
            bool s = StringUtils::insertChars(textBuffer.data, textBuffer.count, textBufferMaxLength, textToCopy.data, textToCopy.count, cursorPos);
            assert(s);

            textBuffer.count += textToCopy.count;
            
            cursorPos += textToCopy.count;

            onTextChanged();
            shouldDrawCursor = true;
            setCursorTimer();

            redraw();
        }

        g_standardAllocator.dealloc(textToCopy.data);
        return 0;
    }

    return 0;
}

LRESULT CommandWindow::onMouseActivate()
{
    if (GetFocus() != hwnd)
    {
        SetFocus(hwnd);
        redraw();
    }

    return MA_ACTIVATE;
}

LRESULT CommandWindow::onLeftMouseButtonDown(LPARAM lParam, WPARAM wParam)
{
    float mouseX = static_cast<float>(GET_X_LPARAM(lParam));
    float mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
    const float borderSize = static_cast<float>(style->borderSize);

    updateTextLayout();
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

    clearSelection();
    redraw();

    return 0;
}

LRESULT CommandWindow::onLeftMouseButtonUp()
{
    selectionStartCursorPos = -1;

    if (GetCapture() == hwnd)
        ReleaseCapture();

    return 0;
}

LRESULT CommandWindow::onFocusAcquired()
{
    //originalKeyboardLayout = GetKeyboardLayout(GetCurrentThreadId());
    
    ActivateKeyboardLayout(g_englishKeyboardLayout, KLF_REORDER);

    shouldDrawCursor = true;
    setCursorTimer();

    return 0;
}

LRESULT CommandWindow::onFocusLost()
{
    //ActivateKeyboardLayout(originalKeyboardLayout, KLF_REORDER);
    //OutputDebugStringA("lost focus.\n");
    //InvalidateRect(hwnd, nullptr, true);
    hideWindow();
    
    // @TODO: use built-in CreateCaret/ShowCaret/etc. stuff!

    return 0;
}

void CommandWindow::onTextChanged()
{
    shouldDrawCursor = true;
    setCursorTimer();

    updateAutocompletion();
}

LRESULT CommandWindow::onMouseMove(LPARAM lParam, WPARAM wParam)
{
    bool isLeftMouseDown = static_cast<bool>(wParam & 0x0001);

    if (!isLeftMouseDown || selectionStartCursorPos == -1)
        return 0;

    SetCapture(hwnd);

    float mouseX = static_cast<float>(GET_X_LPARAM(lParam));
    float mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
    const float borderSize = static_cast<float>(style->borderSize);

    updateTextLayout();

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
            clearSelection();
        }
        else
        {
            selectionInitialPos = selectionStartCursorPos;
            selectionPos = math::min((int)cursorPos, oldCursorPos);
            selectionLength = calcSelectionLength;
        }
    }

    redraw();

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

    clearSelection();
    cursorPos = textBuffer.count;

    redraw();

    return;
}

Command* CommandWindow::findAutocompletionCandidate()
{
    if (textBuffer.count == 0)
        return nullptr;

    int index = stringFindCharIndex(textBuffer.data, textBuffer.count, L' ');

    String command;
    command.data  = textBuffer.data;
    command.count = index == -1 ? textBuffer.count : index;

    if (command.count == 0)
        return nullptr;

    for (uint32_t i = 0; i < commandEngine->commands.count; ++i)
    {
        Command* candidate = commandEngine->commands.data[i];
        bool isMatchingCandidate = stringStartsWith(
            candidate->name.data,
            candidate->name.count,
            command.data,
            command.count,
            false
        );

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
        redraw();
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

void CommandWindow::animateWindow(WindowAnimation animation)
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

LRESULT CommandWindow::onPaint()
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

    updateTextLayout();

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
    if (isTextSelected())
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

    if (shouldDrawCursor && !isTextSelected())
    {
        // Center cursor X at pixel center to disable anti-aliasing.
        float cursorActualX = floorf(marginX + cursorRelativeX) + 0.5f;
        rt->DrawLine(D2D1::Point2F(cursorActualX, cursorUpY), D2D1::Point2F(cursorActualX, cursorBottomY), textForegroundBrush);
    }

    rt->EndDraw();

    ValidateRect(hwnd, nullptr);
    return 0;
}

LRESULT CommandWindow::onHotkey(LPARAM lParam, WPARAM wParam)
{
    if (wParam == SHOW_APP_WINDOW_HOTKEY_ID)
    {
        if (IsWindowVisible(hwnd))
        {
            hideWindow();
        }
        else
        {
            showWindow();
        }
    }

    return 0;
}

LRESULT CommandWindow::onTimer(LPARAM lParam, WPARAM timerID)
{
    switch (timerID)
    {
        case CURSOR_BLINK_TIMER_ID: return onCursorBlinkTimerElapsed();
        default:
            __debugbreak(); // Unknown timer.
    }
    
    return 0;
}

LRESULT CommandWindow::onShowWindow(LPARAM lParam, WPARAM wParam)
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

LRESULT CommandWindow::onQuit()
{
    DestroyWindow(hwnd);

    return 0;
}

LRESULT CommandWindow::onActivate()
{
    return 0;
}

LRESULT CommandWindow::onCursorBlinkTimerElapsed()
{
    shouldDrawCursor = !shouldDrawCursor;
    redraw();

    return 0;
}

bool CommandWindow::init(HINSTANCE hInstance, int windowWidth, int windowHeight)
{
    if (isInitialized)
        return true;

    assert(windowWidth  > 0);
    assert(windowHeight > 0);
    assert(style);
    assert(commandEngine);

    isInitialized = false;
    if (!initGlobalResources(hInstance))
        return false;

    // Initialize Direct2D and DirectWrite
    this->d2d1 = nullptr;
    this->dwrite = nullptr;
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
            return false;
        }
    }

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

    if (!taskbarIcon.enable(hwnd, g_appIcon, 1, WM_USER + 15))
    {
        // We can live without it.
    }

    if (!RegisterHotKey(hwnd, SHOW_APP_WINDOW_HOTKEY_ID, MOD_ALT | MOD_NOREPEAT, 0x51))
        MessageBoxW(hwnd, L"Hotkey Alt+Q is already claimed.", L"Command Bar", MB_ICONINFORMATION);

    if ((trayMenu = CreatePopupMenu()) != 0)
    {
        AppendMenuW(trayMenu, MF_STRING, (UINT_PTR)TrayMenuItem::Show, L"Show");
        AppendMenuW(trayMenu, MF_SEPARATOR, (UINT_PTR)TrayMenuItem::None, nullptr);
        AppendMenuW(trayMenu, MF_STRING, (UINT_PTR)TrayMenuItem::Exit, L"Exit");
    }

    commandEngine->setBeforeRunCallback(beforeRunCallback, this);

    textBufferMaxLength = 512;
    textBuffer.count = 0;
    textBuffer.data = static_cast<wchar_t*>(malloc(textBufferMaxLength * sizeof(textBuffer.data[0])));
    if (textBuffer.data == nullptr)
        return false;

    setCursorTimer();

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

        hr = hwndRenderTarget->CreateSolidColorBrush(style->autocompletionTextColor, &autocompletionTextForegroundBrush);
        assert(SUCCEEDED(hr));

        hr = hwndRenderTarget->CreateSolidColorBrush(style->selectedTextBackgroundColor, &selectedTextBrush);
        assert(SUCCEEDED(hr));

        hr = hwndRenderTarget->CreateSolidColorBrush(style->textboxBackgroundColor, &textboxBackgroundBrush);
        assert(SUCCEEDED(hr));
    }

    QuitCommand* quitcmd = stdNew<QuitCommand>();
    quitcmd->name = String::clone(L"quit");
    quitcmd->info = nullptr;
    quitcmd->commandWindow = this;
    commandEngine->registerCommand(quitcmd);

    //EditCommandsWindow* edit = new EditCommandsWindow();
    //edit->init(hwnd, commandEngine);

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

    animateWindow(WindowAnimation::Show);
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
    animateWindow(WindowAnimation::Hide);

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
                g_standardAllocator.dealloc(error.data);
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

    return static_cast<int>(msg.wParam);
}

void CommandWindow::evaluate()
{
    if (!commandEngine->evaluate(textBuffer))
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
    taskbarIcon.disable();

    DeleteMenu(trayMenu, 0, 0);
    trayMenu = 0;
}

LRESULT CommandWindow::wndProc(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam)
{
    switch (msg)
    {
        case WM_ERASEBKGND:     return 1;
        case WM_CHAR:           return this->onChar(static_cast<wchar_t>(wParam));
        case WM_KEYDOWN:        return this->onKeyDown(lParam, wParam);
        case WM_MOUSEMOVE:      return this->onMouseMove(lParam, wParam);
        case WM_LBUTTONUP:      return this->onLeftMouseButtonUp();
        case WM_LBUTTONDOWN:    return this->onLeftMouseButtonDown(lParam, wParam);
        case WM_MOUSEACTIVATE:  return this->onMouseActivate();
        case WM_SETFOCUS:       return this->onFocusAcquired();
        case WM_KILLFOCUS:      return this->onFocusLost();
        case WM_PAINT:          return this->onPaint();
        case WM_HOTKEY:         return this->onHotkey(lParam, wParam);
        case WM_TIMER:          return this->onTimer(lParam, wParam);
        case WM_SHOWWINDOW:     return this->onShowWindow(lParam, wParam);
        case WM_QUIT:           return this->onQuit();
        case WM_ACTIVATE:       return this->onActivate();

        case (UINT)CustomMessage::ShowAfterAllEventsProcessed:
        {
            showWindow();
            return 0;
        }

        case CommandWindow::g_showWindowMessageId:
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
            default:                 break;
        }

        return 0;
    }
    else if (taskbarIcon.isClicked(hwnd, msg, lParam, wParam))
    {
        OutputDebugStringW(L"clicked\n");
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

    return window ? window->wndProc(hwnd, msg, lParam, wParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}

void beforeRunCallback(CommandEngine * engine, void * userdata)
{
    if (userdata != nullptr)
        ((CommandWindow*)userdata)->beforeCommandRun();
}
