#include "context.h"
#include "defer.h"
#include "newstring.h"


static GraphicsContext graphicsContext;


GraphicsContext* GetGraphicsContext()
{
    static bool initialized = false;
    if (initialized)
        return &graphicsContext;

    auto g = &graphicsContext;
    SafeRelease(g->d2d1);
    SafeRelease(g->dwrite);

    //const auto format = Newstring::FormatTempCStringWithFallback;

    HRESULT hr = E_UNEXPECTED;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g->d2d1);
    if (FAILED(hr))
    {
        return nullptr;
        //return ShowErrorBox(hwnd, format(L"Cannot initialize Direct2D.\n\nError code was 0x%08X.", hr));
    }

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&g->dwrite));
    if (FAILED(hr))
    {
        return nullptr;
        //return ShowErrorBox(hwnd, format(L"Cannot initialize DirectWrite.\n\nError code was 0x%08X.", hr));
    }

    initialized = true;
    return g;
}
