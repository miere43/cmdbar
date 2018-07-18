#pragma once
#include "common.h"
#include "array.h"
#include "newstring.h"
#include <Windows.h>
#include <d2d1.h>
#include "context.h"


struct Margin
{
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    inline Margin() { }
    inline Margin(int left, int top, int right, int bottom) {
        this->left = left;
        this->top = top;
        this->right = right;
        this->bottom = bottom;
    }
};

enum class PopupCorner
{
    TopRight,
    TopLeft,
    BottomRight,
    BottomLeft,
};

struct PopupWindow
{
    HWND hwnd = 0;
    
    void SetHeaderText(const Newstring& text, bool takeOwnership = false);
    void SetMainText(const Newstring& text, bool takeOwnership = false);
    void SetPopupCorner(PopupCorner corner);

    /** Call Initialize after setup functions (Set...) */
    bool Initialize(HWND parentWindow);
    void Show(int dismissMilliseconds);
    void Dismiss();
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
    PopupCorner popupCorner = PopupCorner::TopRight;

    void InstallDismissTimer();
    void UninstallDismissTimer();

    void OnMouseEnter();
    void OnMouseLeave();

    int dismissMilliseconds = 3000;
    bool dismissing = false;

    int windowMargin = 15;
    Margin headerTextMargin = Margin{ 5, 5, 5, 5 };
    Margin mainTextMargin = Margin{ 5, 40, 5, 5 };
    
    void CreateGraphicsResources();
    void DisposeGraphicsResources();

    LRESULT OnPaint();

    static ATOM g_windowClass;
    static bool InitializeStaticResources();

    LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT __stdcall StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
