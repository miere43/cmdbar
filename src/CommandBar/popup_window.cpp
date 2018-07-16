#include "popup_window.h"


ATOM PopupWindow::g_windowClass = 0;


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

    hwnd = CreateWindowExW(
        styleEx, 
        (LPCWSTR)g_windowClass, 
        L"Popup window",
        style,
        0, 
        0,
        300,
        300,
        parentWindow, 
        0, 
        GetModuleHandleW(0),
        this);
    if (!hwnd)
        return false;

    CreateGraphicsResources();

    return true;
}

void PopupWindow::Show(int timeoutMilliseconds)
{
    ::ShowWindow(hwnd, SW_SHOW);
    OnPaint();
}

void PopupWindow::CreateGraphicsResources()
{
    auto d2d1 = graphics->d2d1;
    auto dwrite = graphics->dwrite;

    RECT rc;
    GetClientRect(hwnd, &rc);

    HRESULT hr;

    hr = d2d1->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top)),
        &renderTarget);
    assert(SUCCEEDED(hr));

    IDWriteTextFormat* format = nullptr;
    hr = dwrite->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"en-us", &format);
    assert(SUCCEEDED(hr));

    // @TODO
    headerTextFormat = format;
    mainTextFormat = format;
    format->AddRef();
 
    hr = dwrite->CreateTextLayout(headerText.data, headerText.count, headerTextFormat, 200, 200, &headerTextLayout);
    assert(SUCCEEDED(hr));
    hr = dwrite->CreateTextLayout(mainText.data, mainText.count, mainTextFormat, 200, 200, &mainTextLayout);
    assert(SUCCEEDED(hr));

    ID2D1SolidColorBrush* brush = nullptr;
    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush);
    assert(SUCCEEDED(hr));

    // @TODO
    headerTextBrush = brush;
    mainTextBrush = brush;
    brush->AddRef();
}

void PopupWindow::DisposeGraphicsResources()
{
    // @TODO
}

LRESULT PopupWindow::OnPaint()
{
    HRESULT hr;

    renderTarget->BeginDraw();
    renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::LightBlue));

    renderTarget->DrawTextLayout(D2D1::Point2F(0, 0), headerTextLayout, headerTextBrush);
    renderTarget->DrawTextLayout(D2D1::Point2F(0, 50), mainTextLayout, mainTextBrush);

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
    wc.hCursor = LoadCursorW(GetModuleHandleW(0), IDC_ARROW);
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
