#include <Windows.h>
#include <windowsx.h>
#include <algorithm>
#include <assert.h>
#include <math.h>

#include "CommandBar.h"
#include "edit_commands_window.h"
#include "command_window_tray.h"
#include "window_management.h"
#include "command_window.h"
#include "command_loader.h"
#include "basic_commands.h"
#include "popup_window.h"
#include "string_utils.h"
#include "debug_utils.h"
#include "clipboard.h"
#include "parse_ini.h"
#include "os_utils.h"
#include "context.h"
#include "defer.h"
#include "utils.h"


void beforeRunCallback(CommandEngine* engine, void* userdata);

HICON CommandWindow::g_appIcon = 0;
ATOM CommandWindow::g_windowClass = 0;
HKL CommandWindow::g_englishKeyboardLayout = 0;
UINT CommandWindow::g_taskbarCreatedMessageId = 0;
const wchar_t* CommandWindow::g_windowName = L"Command Bar";
const wchar_t* CommandWindow::g_className = L"CommandWindow";
const UINT CommandWindow::g_showWindowMessageId = WM_USER + 64;

enum
{
    SHOW_APP_WINDOW_HOTKEY_ID = 0,
    CURSOR_BLINK_TIMER_ID = 0
};


using namespace WindowManagement;


bool CommandWindow::InitializeStaticResources(HINSTANCE hInstance)
{
    if (g_appIcon == 0)
    {
        g_appIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APPTRAYICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
        if (g_appIcon == 0)
            g_appIcon = LoadIconW(0, IDI_APPLICATION);
    }
    
    if (g_windowClass == 0)
    {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpszClassName = g_className;
        wc.hInstance = hInstance;
        wc.lpfnWndProc = StaticWindowProc;
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.hCursor = LoadCursorW(0, IDC_IBEAM);
        wc.hIcon = g_appIcon;

        g_windowClass = RegisterClassExW(&wc);
        if (g_windowClass == 0)
            return false;
    }

    if (g_englishKeyboardLayout == 0)
        g_englishKeyboardLayout = LoadKeyboardLayoutW(L"00000409", 0);

    if (g_taskbarCreatedMessageId == 0)
        g_taskbarCreatedMessageId = RegisterWindowMessageW(L"TaskbarCreated");

    return true;
}

void CommandWindow::ClearText()
{
    textEdit.ClearText();

    autocompletionCandidate = nullptr;

    Redraw();
}

bool CommandWindow::SetText(const Newstring& text)
{
    textEdit.SetText(text);

    autocompletionCandidate = nullptr;

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
        SafeRelease(textLayout);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        float clientWidth  = static_cast<float>(clientRect.right - clientRect.left);
        float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);

        HRESULT hr = dwrite->CreateTextLayout(
            textEdit.buffer.data,
            textEdit.buffer.count,
            textFormat,
            clientWidth,
            clientHeight,
            &this->textLayout);

        assert(SUCCEEDED(hr));

        textLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        int spaceIndex = textEdit.buffer.string.IndexOf(L' ');

        DWRITE_TEXT_RANGE range;
        if (spaceIndex != -1)
            range = { 0, static_cast<UINT32>(spaceIndex) };
        else
            range = { 0, static_cast<UINT32>(textEdit.buffer.count) };

        textLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, range);

        isTextLayoutDirty = false;
    }
}

void CommandWindow::UpdateAutocompletionLayout()
{
    SafeRelease(autocompletionLayout);

    auto ac = autocompletionCandidate;
    if (ac == nullptr)
        return;

    float clientWidth, clientHeight;
    GetClientSizeF(&clientWidth, &clientHeight);
    
    enum
    {
        MAX_AUTOCOMPLETE = 128
    };

    Newstring autocomplText;
    auto commandText = textEdit.buffer.string;

    if (commandText.count < MAX_AUTOCOMPLETE)
    {
        wchar_t buffer[MAX_AUTOCOMPLETE];

        const size_t ncopy = std::min(commandText.count, ac->name.count);
        wmemcpy(buffer, commandText.data, commandText.count);

        if (ac->name.count > ncopy)
        {
            wmemcpy_s(buffer + ncopy, sizeof(wchar_t) * (MAX_AUTOCOMPLETE - ncopy),
                ac->name.data + ncopy, ac->name.count - ncopy);
        }

        autocomplText.data  = buffer;
        autocomplText.count = ac->name.count;
    }
    else
    {
        autocomplText = commandText;
    }

    HRESULT hr = dwrite->CreateTextLayout(
        autocomplText.data,
        autocomplText.count,
        textFormat,
        clientWidth,
        clientHeight,
        &this->autocompletionLayout
    );
    assert(SUCCEEDED(hr));

    autocompletionLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    DWRITE_TEXT_RANGE range ={ 0, ac->name.count };
    assert(SUCCEEDED(autocompletionLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, range)));
}

void CommandWindow::TextEditChanged()
{
    Redraw();

    shouldDrawCaret = true;
    SetCaretTimer();
}

bool CommandWindow::ShouldDrawAutocompletion() const
{
    return autocompletionCandidate != nullptr;
}

LRESULT CommandWindow::OnChar(wchar_t c)
{
    textEdit.InsertCharacterAtCaret(c);
    isTextLayoutDirty = true;

    return 0;
}

LRESULT CommandWindow::OnKeyDown(LPARAM lParam, WPARAM vk)
{
    bool shift = !!(GetKeyState(VK_LSHIFT) & 0x8000);
    bool control = !!(GetKeyState(VK_LCONTROL) & 0x8000);

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
        case VK_RETURN:
        {
            Evaluate();
            break;
        }
        case VK_UP:
        case VK_DOWN:
        {
            const Newstring* entry = vk == VK_UP ? history.GetPrevEntry() : history.GetNextEntry();
            if (entry == nullptr)  break;

            textEdit.SetText(*entry);
            textEdit.SetCaretPos(entry->count);

            break;
        }
        case VK_BACK:
        case VK_DELETE:
        {
            if (textEdit.IsTextSelected())  textEdit.RemoveSelectedText();
            else  vk == VK_BACK ? textEdit.RemovePrevCharacter() : textEdit.RemoveNextCharacter();
            break;
        }
        case VK_RIGHT:
        {
            if (shift)  textEdit.AddNextCharacterToSelection();
            else  textEdit.IsTextSelected() ? textEdit.ClearSelection(textEdit.GetSelectionEnd()) : textEdit.MoveCaretRight();
            break;
        }
        case VK_LEFT:
        {
            if (shift)  textEdit.AddPrevCharacterToSelection();
            else  textEdit.IsTextSelected() ? textEdit.ClearSelection(textEdit.GetSelectionStart()) : textEdit.MoveCaretLeft();
            break;
        }
        case VK_TAB:
        {
            OnUserRequestedAutocompletion();
            break;
        }
        default:
        {
            if (control && vk == 'A')       textEdit.SelectAll();
            else if (control && vk == 'L')  textEdit.SelectAll();
            else if (control && vk == 'X')
            {
                textEdit.CopySelectionToClipboard(hwnd);
                textEdit.RemoveSelectedText();
            }
            else if (control && vk == 'C')  textEdit.CopySelectionToClipboard(hwnd);
            else if (control && vk == 'V')  textEdit.PasteTextFromClipboard(hwnd);
            break;
        }
    }

    TextEditChanged();

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
    const float mouseX = static_cast<float>(GET_X_LPARAM(lParam));
    const float mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
    const float borderSize = static_cast<float>(style->borderSize);

    UpdateTextLayout();

    BOOL isInside = false;
    BOOL isTrailingHit = false;
    DWRITE_HIT_TEST_METRICS metrics = { 0 };
    HRESULT hr = 0;

    hr = textLayout->HitTestPoint(mouseX - style->textMarginLeft - borderSize, mouseY, &isTrailingHit, &isInside, &metrics);
    assert(SUCCEEDED(hr));

    textEdit.ClearSelection();
    textEdit.SetCaretPos(metrics.textPosition + (isTrailingHit ? 1 : 0));

    mouseSelectionStartPos = textEdit.caretPos;

    Redraw();

    ::SetCapture(hwnd);

    return 0;
}

LRESULT CommandWindow::OnLeftMouseButtonUp()
{
    if (::GetCapture() == hwnd)  ReleaseCapture();
    mouseSelectionStartPos = 0xFFFFFFFF;

    return 0;
}

LRESULT CommandWindow::OnMouseMove(LPARAM lParam, WPARAM wParam)
{
    if (mouseSelectionStartPos == 0xFFFFFFFF)
        return 0;

    bool isLeftMouseDown = !!(wParam & 0x0001);

    if (!isLeftMouseDown)
        return true;

    const float mouseX = static_cast<float>(GET_X_LPARAM(lParam));
    const float mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
    const float borderSize = static_cast<float>(style->borderSize);

    UpdateTextLayout();

    BOOL isInside = false;
    BOOL isTrailingHit = false;
    DWRITE_HIT_TEST_METRICS metrics = { 0 };
    HRESULT hr = 0;

    hr = textLayout->HitTestPoint(mouseX - style->textMarginLeft - borderSize, mouseY, &isTrailingHit, &isInside, &metrics);
    assert(SUCCEEDED(hr));

    uint32_t targetPos = metrics.textPosition + (isTrailingHit ? 1 : 0);
    
    uint32_t start = mouseSelectionStartPos;
    uint32_t length;
    if (targetPos < mouseSelectionStartPos)
    {
        start = targetPos;
        length = mouseSelectionStartPos - targetPos;
    }
    else
    {
        start = mouseSelectionStartPos;
        length = targetPos - mouseSelectionStartPos;
    }
    
    textEdit.Select(start, length);

    Redraw();

    return 0;
}

LRESULT CommandWindow::OnFocusAcquired()
{
    SetForegroundWindow(hwnd);
    ActivateKeyboardLayout(g_englishKeyboardLayout, 0);

    shouldDrawCaret = true;
    SetCaretTimer();

    return 0;
}

LRESULT CommandWindow::OnFocusLost()
{
    HideWindow();
    
    return 0;
}

void CommandWindow::OnTextChanged()
{
    shouldDrawCaret = true;
    SetCaretTimer();

    UpdateAutocompletion();
}

void CommandWindow::OnUserRequestedAutocompletion()
{
    UpdateAutocompletion();
    if (autocompletionCandidate == nullptr)
        return;

    textEdit.SetText(autocompletionCandidate->name);
    textEdit.SetCaretPos(autocompletionCandidate->name.count);
    textEdit.InsertCharacterAtCaret(L' ');

    Redraw();

    return;
}

Command* CommandWindow::FindAutocompletionCandidate()
{
    if (textEdit.buffer.count == 0)
        return nullptr;

    const Newstring& buffer = textEdit.buffer.string;

    int index = buffer.IndexOf(L' ');

    Newstring command;
    command.data  = buffer.data;
    command.count = index == -1 ? buffer.count : index;

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

void CommandWindow::UpdateAutocompletion()
{
    if (showPreviousCommandAutocompletion)
    {
        if (textEdit.buffer.count == 0)
        {
            autocompletionCandidate = showPreviousCommandAutocompletion_command;
            return;
        }
    }
    
    Command* newCandidate = FindAutocompletionCandidate();
    if (autocompletionCandidate != newCandidate)
    {
        autocompletionCandidate = newCandidate;

        Redraw();
    }
}

void CommandWindow::SetCaretTimer()
{
    SetLastError(NO_ERROR);

    uintptr_t timerID = SetTimer(hwnd, CURSOR_BLINK_TIMER_ID, 600, nullptr);
    
    if (timerID == 0 && GetLastError() != NO_ERROR)
    {
        __debugbreak();
    }
}
void CommandWindow::KillCaretTimer()
{
    KillTimer(hwnd, CURSOR_BLINK_TIMER_ID);
}

LRESULT CommandWindow::OnPaint()
{
    ID2D1HwndRenderTarget* rt = this->hwndRenderTarget;
    assert(rt);

    float clientWidth, clientHeight;
    GetClientSizeF(&clientWidth, &clientHeight);
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
    textLayout->HitTestTextPosition(textEdit.caretPos, false, &cursorRelativeX, &cursorRelativeY, &metrics);

    // Draw selection background
    if (textEdit.IsTextSelected())
    {
        uint32_t selectionStart = 0;
        uint32_t selectionLength = 0;
        textEdit.GetSelectionStartAndLength(&selectionStart, &selectionLength);

        float unused;
        float selectionRelativeStartX = 0.0f;
        float selectionRelativeEndX = 0.0f;

        textLayout->HitTestTextPosition(selectionStart, false, &selectionRelativeStartX, &unused, &metrics);
        textLayout->HitTestTextPosition(selectionStart + selectionLength, false, &selectionRelativeEndX, &unused, &metrics);

        rt->FillRectangle(
            D2D1::RectF(
                marginX + selectionRelativeStartX,
                borderSize, 
                marginX + selectionRelativeEndX,
                clientHeight - borderSize), 
            selectedTextBrush);
    }

    float textDrawY = borderSize;

    UpdateAutocompletion();
    if (ShouldDrawAutocompletion())
    {
        UpdateAutocompletionLayout();

        DWRITE_HIT_TEST_METRICS acMetrics;
        float acDrawRelativeX = 0.0f;
        float acDrawRelativeY = 0.0f;
        assert(SUCCEEDED(
            autocompletionLayout->HitTestTextPosition(textEdit.buffer.count, false, &acDrawRelativeX, &acDrawRelativeY, &acMetrics)));

        D2D1_RECT_F acClip = D2D1::RectF(
            marginX + acDrawRelativeX,
            borderSize + acDrawRelativeY,
            clientWidth,
            clientHeight);

        rt->PushAxisAlignedClip(acClip, D2D1_ANTIALIAS_MODE_ALIASED);

        rt->DrawTextLayout(
            D2D1::Point2F(marginX, textDrawY),
            autocompletionLayout,
            autocompletionTextForegroundBrush,
            D2D1_DRAW_TEXT_OPTIONS_CLIP);

        rt->PopAxisAlignedClip();
    }

    // Draw actual user text
    rt->DrawTextLayout(D2D1::Point2F(marginX, textDrawY), textLayout, textForegroundBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

    if (shouldDrawCaret && !textEdit.IsTextSelected())
    {
        // Center cursor X at pixel center to disable anti-aliasing.
        float cursorActualX = floorf(marginX + cursorRelativeX) + 0.5f;
        rt->DrawLine(D2D1::Point2F(cursorActualX, cursorUpY), D2D1::Point2F(cursorActualX, cursorBottomY), textForegroundBrush);
    }

    HRESULT hr = rt->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        DiscardGraphicsResources();
        CreateGraphicsResources();
    } 
    else
    {
        ValidateRect(hwnd, nullptr);
    }

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
        SetCaretTimer();
        __debugbreak();
    }
    else 
        KillCaretTimer();

    return 0;
}

LRESULT CommandWindow::OnQuit()
{
    DestroyWindow(hwnd);

    return 0;
}

LRESULT CommandWindow::OnActivate(uint32_t activateState)
{
    if (activateState != WA_INACTIVE)
    {
        history.ResetCurrentEntryIndex();
    }

    return 0;
}

LRESULT CommandWindow::OnCursorBlinkTimerElapsed()
{
    shouldDrawCaret = !shouldDrawCaret;
    Redraw();

    return 0;
}

bool CommandWindow::Initialize(CommandEngine* engine, CommandWindowStyle* style, int nCmdShow)
{
    if (isInitialized)
        return true;

    assert(style);
    assert(engine);

    this->commandEngine = engine;
    this->style = style;

    HINSTANCE hInstance = GetModuleHandleW(0);

    isInitialized = false;
    if (!InitializeStaticResources(hInstance))
        return false;

    int windowWidth = style->windowWidth;
    int windowHeight = style->windowHeight;//(int)style->fontHeight + style->borderSize * 2;

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
    tray.hwnd = hwnd;
    tray.icon = g_appIcon;
    tray.Initialize(&trayFailureReason);

    if (!Newstring::IsNullOrEmpty(trayFailureReason))
    {
        MessageBoxW(hwnd, trayFailureReason.CloneAsTempCString(), L"Error", MB_ICONERROR);
    } 

    if (!CreateGraphicsResources())
        return false;

    commandEngine->SetBeforeRunCallback(beforeRunCallback, this);
    if (!textEdit.Initialize())
        return false;

    SetCaretTimer();

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
    
    //if (nCmdShow != 0) ShowWindow();

    // @TODO
    //PopupWindow* window = new PopupWindow();
    //window->SetHeaderText(Newstring::Format(L"Holy shit."), true);
    //window->SetMainText(Newstring::Format(L"There were errors when reading your commands file.\n\nClick here to do something."), true);
    //window->SetPopupCorner(PopupCorner::BottomRight);
    //window->Initialize(hwnd);
    //window->Show(3000);

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

    if (showPreviousCommandAutocompletion && showPreviousCommandAutocompletion_command != nullptr)
    {
        autocompletionCandidate = showPreviousCommandAutocompletion_command;
    }

    WindowAnimationProperties props;
    props.startAlpha = 0;
    props.endAlpha = 255;
    AnimateWindow(hwnd, props);
}

void CommandWindow::HideWindow()
{
    WindowAnimationProperties props;
    props.startAlpha = 255;
    props.endAlpha = 0;
    AnimateWindow(hwnd, props);

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
    const Newstring& input = textEdit.buffer.string;
    history.SaveEntry(input);

    bool success = commandEngine->Evaluate(input);

    if (!success)
    {
        auto state = commandEngine->GetExecutionState();
        Newstring message = state->errorMessage;

        if (Newstring::IsNullOrEmpty(message))
        {
            message = Newstring::WrapConstWChar(L"Unknown error.");
        }

        MessageBoxW(hwnd, message.CloneAsCString(&g_tempAllocator), L"Error", MB_OK | MB_ICONERROR);
        SetFocus(hwnd);  // We lose focus after MessageBox

        Redraw();
    }
    else
    {
        if (showPreviousCommandAutocompletion)
        {
            auto state = commandEngine->GetExecutionState();
            showPreviousCommandAutocompletion_command = state->command;
        }

        ClearText();
        HideWindow();
    }
}

void CommandWindow::GetClientSizeF(float* width, float* height)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    if (width)  *width  = (float)(rc.right - rc.left);
    if (height) *height = (float)(rc.bottom - rc.top);
}

bool CommandWindow::CreateGraphicsResources()
{
    const auto format = Newstring::FormatTempCString;

    HRESULT hr = E_UNEXPECTED;
    defer(
        if (FAILED(hr))
        {
            DiscardGraphicsResources();
        }
    );
    
    auto g = GetGraphicsContext();
    if (g)
    {
        d2d1 = g->d2d1;
        dwrite = g->dwrite;
    }
    else
    {
        return ShowErrorBox(hwnd, L"Unable to initialize graphics.");
    }

    hr = dwrite->CreateTextFormat(
        style->fontFamily.CloneAsCString(&g_tempAllocator),
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
        return ShowErrorBox(hwnd, format(L"Cannot create text format.\n\nError code was 0x%08X.", hr));
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
        return ShowErrorBox(hwnd, format(L"Cannot create window render target.\n\nError code was 0x%08X.", hr));
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
        return ShowErrorBox(hwnd, format(L"Cannot create brushes.\n\nError code was 0x%08X.", hr));
    }

    return true;
}

void CommandWindow::DiscardGraphicsResources()
{
    SafeRelease(textForegroundBrush);
    SafeRelease(autocompletionTextForegroundBrush);
    SafeRelease(selectedTextBrush);
    SafeRelease(textboxBackgroundBrush);
    SafeRelease(textFormat);
    SafeRelease(hwndRenderTarget);
}

void CommandWindow::Dispose()
{
    tray.Dispose();
    DiscardGraphicsResources();
    textEdit.Dispose();
    history.Dispose();

    if (hwnd != 0)
    {
        DestroyWindow(hwnd);
        hwnd = 0;
    }
}

LRESULT CommandWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
        case WM_ACTIVATE:       return this->OnActivate((uint32_t)LOWORD(wParam));

        case CommandWindow::g_showWindowMessageId:
        {
            ShowWindow();
            return 0;
        }
    }

    if (msg == CommandWindow::g_taskbarCreatedMessageId && CommandWindow::g_taskbarCreatedMessageId != 0)
        tray.Initialize(nullptr);

    TrayMenuAction action = TrayMenuAction::None;
    if (tray.ProcessEvent(hwnd, msg, lParam, wParam, &action))
    {
        switch (action)
        {
            case TrayMenuAction::Show: ShowWindow(); break;
            case TrayMenuAction::ReloadCommandsFile: ReloadCommandsFile(); break;
            case TrayMenuAction::OpenCommandsFile: OpenCommandsFile(); break;
            case TrayMenuAction::Exit: Exit(); break;
            default:                 break;
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT __stdcall CommandWindow::StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

    return window ? window->WindowProc(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}

void CommandWindow::BeforeCommandRun()
{
    HideWindow();
}

Newstring AskUserForCmdsFilePath()
{
    HRESULT hr = E_UNEXPECTED;
    defer(
        if (FAILED(hr))
        {
            MessageBoxW(0, Newstring::FormatCString(L"Error: 0x%08X.", hr), L"Error", MB_ICONERROR);
        }
    );

    Newstring result;

    IFileOpenDialog* dialog = nullptr;
    defer(SafeRelease(dialog));

    hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_ALL,
        IID_IFileOpenDialog,
        reinterpret_cast<void**>(&dialog));
    if (FAILED(hr))  return result;

    dialog->SetTitle(L"Select cmds.ini");

    hr = dialog->Show(0); // @TODO: Handle user cancellation case.
    if (FAILED(hr))  return result;

    IShellItem* selection = nullptr;
    defer(SafeRelease(selection));

    hr = dialog->GetResult(&selection);  
    if (FAILED(hr))  return result;

    wchar_t* selectionTitle = nullptr;
    defer(
        if (selectionTitle)
        {
            CoTaskMemFree(selectionTitle);
            selectionTitle = nullptr;
        }
    );

    hr = selection->GetDisplayName(SIGDN_FILESYSPATH, &selectionTitle);
    if (FAILED(hr) || !selectionTitle)  return result;

    result = Newstring::WrapConstWChar(selectionTitle);
    result.count += 1; // Count terminating zero.

    if (!OSUtils::FileExists(result))
        return result;

    result = result.Clone();
    return result;
}

Newstring GetCommandsFilePath()
{
#ifdef _DEBUG
    static Newstring SettingsPath = Newstring::WrapConstWChar(L"settings_debug.ini");
#else
    static Newstring SettingsPath = Newstring::WrapConstWChar(L"settings.ini");
#endif

    NewstringBuilder settingsPath;
    defer(settingsPath.Dispose());

    OSUtils::GetApplicationDirectory(&settingsPath);
    settingsPath.Append(SettingsPath);
    settingsPath.ZeroTerminate();

    Newstring cmdsFile;
    Newstring settingsText = OSUtils::ReadAllText(settingsPath.string);
    defer(settingsText.Dispose());

    if (Newstring::IsNullOrEmpty(settingsText))
    {
        cmdsFile = AskUserForCmdsFilePath();
        if (Newstring::IsNullOrEmpty(cmdsFile))
        {
            settingsPath.Dispose();
            return Newstring::Empty();
        }

        Newstring settingsNewText = Newstring::Join({
            Newstring::WrapConstWChar(L"cmds_path="),
            Newstring(cmdsFile.data, cmdsFile.count - 1) // Don't count terminating zero. 
        }); 
        defer(settingsNewText.Dispose());
        assert(!Newstring::IsNullOrEmpty(settingsNewText));

        bool result = OSUtils::WriteAllText(settingsPath.string, settingsNewText);
        assert(result);
    }
    else
    {
        INIParser ps;
        ps.Initialize(settingsText);

        while (ps.Next())
        {
            if (ps.type == INIValueType::KeyValuePair && ps.key == L"cmds_path")
            {
                cmdsFile = Newstring::NewFromWChar(ps.value.data, ps.value.count);
                break;
            }
        }
    }

    return cmdsFile;
}

void CommandWindow::ReloadCommandsFile()
{
    // @TODO: We have a memory leak here, but who cares?

    autocompletionCandidate = nullptr;
    showPreviousCommandAutocompletion_command = nullptr;

    commandEngine->UnregisterAllCommands();

    CommandLoader commandLoader;
    RegisterBuiltinCommands(&commandLoader);

    Array<Command*> commands = commandLoader.LoadFromFile(GetCommandsFilePath());
    for (uint32_t i = 0; i < commands.count; ++i)
        commandEngine->RegisterCommand(commands.data[i]);
}

void CommandWindow::OpenCommandsFile()
{
    auto path = GetCommandsFilePath();

    int result = (int)ShellExecuteW(
        0,
        L"open",
        path.CloneAsTempCString(),
        nullptr,
        nullptr,
        SW_SHOW);

    if (result <= 32) {
        MessageBoxW(0, L"Failed to launch text editor", L"Error", MB_ICONERROR);
    }

    Memdelete(path.data);
}

void beforeRunCallback(CommandEngine* engine, void* userdata)
{
    if (userdata != nullptr)
        ((CommandWindow*)userdata)->BeforeCommandRun();
}
