#include "hint_window.h"


ATOM HintWindow::g_classAtom = 0;


bool HintWindow::initialize(int x, int y, int w, int h)
{
    HINSTANCE hInstance = GetModuleHandleW(0);

    if (g_classAtom == 0)
    {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.hInstance = hInstance;
        wc.hbrBackground = 0;
        wc.lpfnWndProc = HintWindow::staticWndProc;
        wc.lpszClassName = L"HintWindow";
        wc.hCursor = LoadCursorW(0, IDC_ARROW);

        g_classAtom = RegisterClassExW(&wc);
        if (g_classAtom == 0)
        {
            return false;
        }
    }

    HWND hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        (LPCWSTR)g_classAtom,
        L"Hint",
        WS_POPUP,
        x,
        y,
        w,
        h,
        0,
        0,
        hInstance,
        this
    );

    if (!hwnd)
    {
        return false;
    }

    this->hwnd = hwnd;
    return true;
}

void HintWindow::setStyle(Style style)
{
    this->style = style;

    styleBrush = CreateSolidBrush(RGB(180, 0, 0));
}

void HintWindow::setParentWindow(HWND parentHwnd)
{
    this->parentHwnd = parentHwnd;
    //SetParent(this->hwnd, this->parentHwnd);
}

void HintWindow::setText(const String& text)
{
    if (this->text.data != nullptr)
    {
        g_standardAllocator.dealloc(this->text.data);
        this->text.data  = nullptr;
        this->text.count = 0;
    }

    this->text = String::clone(text, &g_standardAllocator);
    InvalidateRect(hwnd, nullptr, false);
}

void HintWindow::setTimeout(ULONGLONG timeoutMSecs)
{
    if (flags & Dismissing)
        return;

    if (timeoutMSecs == 0ULL)
    {
        targetTimeoutMSecs = 0ULL;
        if (!(flags & Dismissing) || !(flags & Showing))
            setTimer(false);
    }
    else
    {
        targetTimeoutMSecs = GetTickCount64() + timeoutMSecs;
        setTimer(true);
    }
}
//
//void HintWindow::setDestroyAfterDismissed(bool destroyAfterDismissed)
//{
//    
//}

bool HintWindow::hasTimeout() const
{
    return targetTimeoutMSecs != 0ULL;
}

void HintWindow::setFont(HFONT font, bool takeOwnership)
{
    destroyFont();
    this->font = font;

    if (font != 0)
    {
        if (takeOwnership)
        {
            flags = flags & OwnsFont;
        }
        else
        {
            flags = flags & ~OwnsFont;
        }
    }

    InvalidateRect(hwnd, nullptr, false);
}

void HintWindow::show()
{
    if ((flags & Showing) || (flags & Dismissing))
        return;
    flags = flags | Showing;

    setTimer(true);

    SetWindowLongW(hwnd, GWL_EXSTYLE, GetWindowLongW(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);

    SetWindowPos(
        hwnd,
        HWND_TOPMOST,
        0,
        0,
        0,
        0,
        SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE
    );
}

void HintWindow::dismiss()
{
    if ((flags & Showing))
        flags = flags & ~Showing;
    flags = flags | Dismissing;

    targetTimeoutMSecs = 0ULL;
    setTimer(true);

    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
}

void HintWindow::destroy()
{
    if (hwnd != 0)
    {
        DestroyWindow(hwnd);
        hwnd = 0;
    }

    parentHwnd = 0;

    if (styleBrush != 0)
    {
        DeleteObject(styleBrush);
        styleBrush = 0;
    }

    style = Style::Information;

    if (text.data != nullptr)
    {
        g_standardAllocator.dealloc(text.data);
        text.data = nullptr;
        text.count = 0;
    }

    setTimer(false);
    destroyFont();

    flags = 0;
}

void HintWindow::paintWindow()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rc;
    GetClientRect(hwnd, &rc);

    SelectObject(hdc, styleBrush);
    FillRect(hdc, &rc, styleBrush);

    if (!text.isEmpty())
    {
        SelectObject(hdc, font);
        SetBkColor(hdc, RGB(180, 0, 0));
        SetTextColor(hdc, RGB(255, 255, 255));

        DrawTextExW(
            hdc,
            text.data,
            text.count,
            &rc,
            DT_CENTER | DT_HIDEPREFIX | DT_WORD_ELLIPSIS | DT_SINGLELINE | DT_VCENTER,
            nullptr
        );
    }

    EndPaint(hwnd, &ps);
}

void HintWindow::updateWindowAnimation()
{
    if (flags & Showing)
    {
        BYTE opacity = 0;
        GetLayeredWindowAttributes(hwnd, nullptr, &opacity, nullptr);

        const int step = 20;
        if (opacity > 255 - step)
        {
            SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
            if (!hasTimeout())
                setTimer(false);

            flags = flags & ~Showing;
        }
        else
        {
            opacity += step;
            SetLayeredWindowAttributes(hwnd, 0, opacity, LWA_ALPHA);
        }

        InvalidateRect(hwnd, nullptr, false);
    }
    else if (flags & Dismissing)
    {
        BYTE opacity = 0;
        GetLayeredWindowAttributes(hwnd, nullptr, &opacity, nullptr);

        const int step = 20;
        if ((int)opacity - step < 0)
        {
            SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
            setTimer(false);
            ShowWindow(hwnd, SW_HIDE);

            flags = flags & ~Dismissing;
        }
        else
        {
            opacity -= step;
            SetLayeredWindowAttributes(hwnd, 0, opacity, LWA_ALPHA);
        }

        InvalidateRect(hwnd, nullptr, false);
    }
}

void HintWindow::updateTimeout()
{
    if (targetTimeoutMSecs != 0ULL)
    {
        ULONGLONG currentTimeoutMSecs = GetTickCount64();

        if (currentTimeoutMSecs >= targetTimeoutMSecs)
        {
            targetTimeoutMSecs = 0ULL;
            dismiss();
        }
    }
}

void HintWindow::destroyFont()
{
    if ((flags & OwnsFont) && font != 0)
    {
        DeleteObject(font);
        font = 0;
        flags = flags & ~OwnsFont;
    }
}

void HintWindow::setTimer(bool enabled)
{
    if (enabled)
    {
        if (timerId == 0)
        {
            timerId = (UINT_PTR)hwnd;
            SetTimer(hwnd, timerId, 10, nullptr);
        }
    }
    else
    {
        if (timerId != 0)
        {
            KillTimer(hwnd, timerId);
            timerId = 0;
        }
    }
}

LRESULT HintWindow::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            return 0;
        }
        case WM_PAINT:
        {
            paintWindow();
            return 0;
        }
        case WM_TIMER:
        {
            if ((UINT_PTR)wParam == timerId)
            {
                updateWindowAnimation();
                updateTimeout();
            }
            return 0;
        }
        case WM_ERASEBKGND:
        {
            return 1;
        }
        case WM_LBUTTONDOWN:
        {
            dismiss();
            return 0;
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT HintWindow::staticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HintWindow* window = nullptr;

    if (msg == WM_CREATE)
    {
        CREATESTRUCTW* createStruct = (CREATESTRUCTW*)lParam;
        if (createStruct == nullptr)
            return 1;

        window = (HintWindow*)createStruct->lpCreateParams;
        if (window == nullptr)
            return 1;

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)window);
    }
    else
    {
        window = (HintWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }

    return window ? window->wndProc(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}


