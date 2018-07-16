#pragma once
#include "common.h"
#include "array.h"
#include "newstring.h"
#include <Windows.h>
#include <d2d1.h>
#include "context.h"


struct PopupWindow
{
    HWND hwnd = 0;
    
    void SetHeaderText(const Newstring& text, bool takeOwnership = false);
    void SetMainText(const Newstring& text, bool takeOwnership = false);

    /** Call Initialize after setup functions (Set...) */
    bool Initialize(HWND parentWindow);
    void Show(int timeoutMilliseconds);
private:
    ID2D1HwndRenderTarget* renderTarget = nullptr;
    IDWriteTextLayout* headerTextLayout = nullptr;
    IDWriteTextFormat* headerTextFormat = nullptr;
    IDWriteTextLayout* mainTextLayout = nullptr;
    IDWriteTextFormat* mainTextFormat = nullptr;
    ID2D1SolidColorBrush* headerTextBrush = nullptr;
    ID2D1SolidColorBrush* mainTextBrush = nullptr;

    GraphicsContext* graphics = nullptr;
    Newstring headerText;
    Newstring mainText;

    void CreateGraphicsResources();
    void DisposeGraphicsResources();

    LRESULT OnPaint();

    static ATOM g_windowClass;
    static bool InitializeStaticResources();

    LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT __stdcall StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
