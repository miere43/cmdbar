#include "popup_window.h"
#include "window_management.h"
#include <Windowsx.h>


using namespace WindowManagement;


ATOM PopupWindow::g_windowClass = 0;
const UINT_PTR DismissTimerId = 123;
const int ShowAnimationFilter = 0;
const int DismissAnimationFilter = 1;


/**
 * @TODO:
 * - Fade in / fade out.
 * - Window timeout.
 * - Proper display for elements.
 * - Different styles (success, error, warning)
 * - Close button.
 */


void PopupWindow::SetHeaderText(const Newstring& text, bool takeOwnership)
{
    headerText.Dispose();
    headerText = takeOwnership ? text : text.Clone();
}

void PopupWindow::SetMainText(const Newstring & text, bool takeOwnership)
{
    mainText.Dispose();
    mainText = takeOwnership ? text : text.Clone();
}

void PopupWindow::SetPopupCorner(PopupCorner corner)
{
    popupCorner = corner;
}

bool PopupWindow::Initialize(HWND parentWindow)
{
    if (!InitializeStaticResources())
        return false;

    graphics = GetGraphicsContext();
    if (!graphics)
        return false;

    const DWORD styleEx = WS_EX_TOOLWINDOW;
    const DWORD style = WS_POPUP;

    int x = 0, y = 0;
    int width = 280, height = 150;

    RECT rc { 0, 0, width, height };
    AdjustWindowRectEx(&rc, style, false, styleEx);

    width = rc.right - rc.left;
    height = rc.bottom - rc.top;

    HMONITOR monitor = MonitorFromWindow(parentWindow, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitorInfo { sizeof(monitorInfo) };
    if (GetMonitorInfoW(monitor, &monitorInfo))
    {
        int desktopWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
        int desktopHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;

        switch (popupCorner)
        {
            case PopupCorner::TopRight:
            {
                x = desktopWidth - width - windowMargin;
                y = windowMargin;
                break;
            }
            case PopupCorner::TopLeft:
            {
                x = windowMargin;
                y = windowMargin;
                break;
            }
            case PopupCorner::BottomRight:
            {
                x = desktopWidth  - width  - windowMargin;
                y = desktopHeight - height - windowMargin;
                break;
            }
            case PopupCorner::BottomLeft:
            {
                x = windowMargin;
                y = desktopHeight - height - windowMargin;
                break;
            }
        }
    }

    hwnd = CreateWindowExW(
        styleEx, 
        (LPCWSTR)g_windowClass, 
        L"Popup window",
        style,
        x, 
        y,
        width,
        height,
        parentWindow, 
        0, 
        GetModuleHandleW(0),
        this);
    if (!hwnd)
        return false;

    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, GetWindowLongPtrW(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);

    CreateGraphicsResources();

    return true;
}

void PopupWindow::Show(int dismissMilliseconds)
{
    this->dismissMilliseconds = dismissMilliseconds;

    ShowContinue();

    OnPaint();
}

void PopupWindow::Dismiss()
{
    if (dismissing)
        return;
    OutputDebugStringW(L"Dismiss!");
    dismissing = true;

    UninstallDismissTimer();

    WindowAnimationProperties props;
    props.animationDuration = 0.5;
    props.allowToStopAnimation = true;
    props.adjustAnimationDuration = true;
    props.startAlpha = 255;
    props.endAlpha = 0;
    props.stopAnimationMessageFilter = DismissAnimationFilter;

    SendStopAnimationMessage(hwnd, ShowAnimationFilter);

    animatingDismiss = true;
    OutputDebugStringW(L"Begin dismiss animation.\n");
    if (!AnimateWindow(hwnd, props))
    {
        OutputDebugStringW(L"End dismiss animation.\n");
        animatingDismiss = false;
        OutputDebugStringW(L"Dismiss animation was stopped\n");
        // Animation was stopped by application.
        dismissing = false;
        return;
    }
    OutputDebugStringW(L"End dismiss animation.\n");
    animatingDismiss = false;

    // AnimateWindow dispatches window messages, so something could have changed.
    if (!dismissing)
        return;

    OutputDebugStringW(L"Dismiss & destroy window!\n");

    UninstallDismissTimer();
    ::ShowWindow(hwnd, SW_HIDE);
    DestroyWindow(hwnd);
    hwnd = 0;
}

void PopupWindow::ShowContinue()
{
    SetWindowPos(
        hwnd,
        HWND_TOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW
    );
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    WindowAnimationProperties props;
    props.animationDuration = 3;
    props.adjustAnimationDuration = true;
    props.allowToStopAnimation = true;
    props.startAlpha = 0;
    props.endAlpha = 255;
    props.stopAnimationMessageFilter = ShowAnimationFilter;

    //StopAnyWindowAnimation();

    SendStopAnimationMessage(hwnd, DismissAnimationFilter);

    animatingShow = true;
    OutputDebugStringW(L"Begin show animation.\n");
    if (!AnimateWindow(hwnd, props))
    {
        OutputDebugStringW(L"Show animation was stopped\n");
    }
    OutputDebugStringW(L"End show animation.\n");
    animatingShow = false;
}

void PopupWindow::InstallDismissTimer()
{
    if (hwnd != 0)
    {
        SetTimer(hwnd, DismissTimerId, dismissMilliseconds, nullptr);
    }
}

void PopupWindow::UninstallDismissTimer()
{
    if (hwnd != 0)
    {
        KillTimer(hwnd, DismissTimerId);
    }
}

void PopupWindow::OnMouseEnter()
{
    OutputDebugStringW(L"OnMouseEnter()\n");
    UninstallDismissTimer();

    if (dismissing)
    {
        SendStopAnimationMessage(hwnd, DismissAnimationFilter);
        dismissing = false;
        ShowContinue();
    }
}

void PopupWindow::OnMouseLeave()
{
    OutputDebugStringW(L"OnMouseLeave()\n");

    //if (dismissing)
    //    return;

    InstallDismissTimer();
}

void PopupWindow::OnDismissTimerExpired()
{
    POINT cursor;
    GetCursorPos(&cursor);

    RECT clientRect;
    GetWindowRect(hwnd, &clientRect);

    if (PtInRect(&clientRect, cursor))
    {
        // Don't dismiss if we have cursor on window.
        if (dismissing)
        {
            SendStopAnimationMessage(hwnd, DismissAnimationFilter);
            dismissing = false;
        }
        UninstallDismissTimer();
        return;
    }
    
    Dismiss();
}

void PopupWindow::StopAnyWindowAnimation()
{
    //if (animatingShow || animatingDismiss)
    //{
    //    OutputDebugStringW(L"Stopped some animation.\n");
    //    SendStopAnimationMessage(hwnd);
    //    animatingShow = false;
    //    animatingDismiss = false;
    //}
}

void PopupWindow::CreateGraphicsResources()
{
    auto d2d1 = graphics->d2d1;
    auto dwrite = graphics->dwrite;

    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    HRESULT hr;

    hr = d2d1->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(width, height)),
        &renderTarget);
    assert(SUCCEEDED(hr));

    hr = dwrite->CreateTextFormat(
        L"Segoe UI", 
        nullptr, 
        DWRITE_FONT_WEIGHT_NORMAL, 
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 
        22.0f,
        L"en-us", 
        &headerTextFormat);
    assert(SUCCEEDED(hr));
    hr = dwrite->CreateTextFormat(
        L"Segoe UI", 
        nullptr, 
        DWRITE_FONT_WEIGHT_NORMAL, 
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 
        14.0f,
        L"en-us", 
        &mainTextFormat);
    assert(SUCCEEDED(hr));

    auto headerTextMaxSize = D2D1::SizeF(
        width - headerTextMargin.left - headerTextMargin.right,
        height - headerTextMargin.top - headerTextMargin.bottom);

    hr = dwrite->CreateTextLayout(headerText.data, headerText.count, headerTextFormat, headerTextMaxSize.width, headerTextMaxSize.height, &headerTextLayout);
    assert(SUCCEEDED(hr));

    auto mainTextMaxSize = D2D1::SizeF(
        width - mainTextMargin.left - mainTextMargin.right,
        height - mainTextMargin.top - mainTextMargin.bottom);

    hr = dwrite->CreateTextLayout(mainText.data, mainText.count, mainTextFormat, mainTextMaxSize.width, mainTextMaxSize.height, &mainTextLayout);
    assert(SUCCEEDED(hr));

    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &headerTextBrush);
    assert(SUCCEEDED(hr));
    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &mainTextBrush);
    assert(SUCCEEDED(hr));

    // @TODO
}

void PopupWindow::DisposeGraphicsResources()
{
    // @TODO
}

LRESULT PopupWindow::OnPaint()
{
    HRESULT hr;

    renderTarget->BeginDraw();
    renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::LightGreen));

    renderTarget->DrawTextLayout(D2D1::Point2F(headerTextMargin.left, headerTextMargin.top), headerTextLayout, headerTextBrush);
    renderTarget->DrawTextLayout(D2D1::Point2F(mainTextMargin.left, mainTextMargin.top), mainTextLayout, mainTextBrush);

    hr = renderTarget->EndDraw();
    assert(SUCCEEDED(hr));
    
    ValidateRect(hwnd, nullptr);
    return 0;
}

bool PopupWindow::InitializeStaticResources()
{
    static bool initialized = false;
    if (initialized)
        return true;

    if (g_windowClass == 0)
    {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.hCursor = LoadCursorW(0, IDC_ARROW);
        wc.hIcon = LoadIconW(0, IDI_APPLICATION);
        wc.lpszClassName = L"PopupWindow";
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = StaticWindowProc;
        wc.hInstance = GetModuleHandleW(0);

        if (!(g_windowClass = RegisterClassExW(&wc)))
            return false;
    }

    initialized = true;
    return true;
}

LRESULT PopupWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            return 0;
        }
        case WM_ERASEBKGND:
        {
            return 1;
        }
        case WM_PAINT:
        {
            return OnPaint();
        }
        case WM_TIMER:
        {
            if (wParam == DismissTimerId)
            {
                OnDismissTimerExpired();
            }
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            if (GetCapture() != hwnd)
            {
                SetCapture(hwnd);
                OnMouseEnter();
            }
            else
            {
                POINT cursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                RECT clientRect;
                GetClientRect(hwnd, &clientRect);
                if (!PtInRect(&clientRect, cursor))
                {
                    ReleaseCapture();
                    OnMouseLeave();
                }
            }
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT PopupWindow::StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PopupWindow* window = nullptr;
    if (msg == WM_CREATE)
    {
        auto cs = (CREATESTRUCT*)lParam;
        if (!cs)  return 1;

        window = (PopupWindow*)cs->lpCreateParams;
        if (!window)  return 1;

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)window);
    }
    else
    {
        window = reinterpret_cast<PopupWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    return window ? window->WindowProc(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}
