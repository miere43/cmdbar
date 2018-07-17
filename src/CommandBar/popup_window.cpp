#include "popup_window.h"
#include "window_management.h"


using namespace WindowManagement;


ATOM PopupWindow::g_windowClass = 0;


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
        x = (monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left) - width - windowMargin;
        y = windowMargin;
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
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    CreateGraphicsResources();

    return true;
}

void PopupWindow::Show(int timeoutMilliseconds)
{
    // @TODO: timeoutMilliseconds unused!

    //SetLayeredWindowAttributes(hwnd, 0, 128, LWA_ALPHA);
    //SetForegroundWindow(hwnd);
    //SetFocus(hwnd);

    ::ShowWindow(hwnd, SW_SHOW);
    //AnimateWindow(hwnd, WindowAnimation::Hide);

    OnPaint();
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

    WNDCLASSEXW wc {};
    wc.cbSize = sizeof(wc);
    wc.hCursor = LoadCursorW(0, IDC_ARROW);
    wc.hIcon = LoadIconW(0, IDI_APPLICATION);
    wc.lpszClassName = L"PopupWindow";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = StaticWindowProc;

    if (!(g_windowClass = RegisterClassExW(&wc)))
        return false;

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
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT PopupWindow::StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PopupWindow* window = nullptr;
    if (msg == WM_CREATE)
    {
        auto cs = (CREATESTRUCT*)lParam;
        if (cs)
        {
            window = (PopupWindow*)cs->lpCreateParams;
            if (window)
            {
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)window);
            }
        }
    }
    return window ? window->WindowProc(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}
