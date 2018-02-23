#pragma once
#include <Windows.h>
#include "allocators.h"
#include "string_type.h"


struct HintWindow
{
    enum Style
    {
        Information,
        Error,
        Warning,
    };

    bool initialize(int x, int y, int w, int h);
    void setStyle(Style style);
    void setParentWindow(HWND hwnd);
    void setText(const String& text);
    void setTimeout(ULONGLONG timeoutMSecs);
    //void setDestroyAfterDismissed(bool destroyAfterDismissed);
    bool hasTimeout() const;

    // If 'takeOwnership' is true, then HintWindow will call DeleteObject(font) when it wants.
    void setFont(HFONT font, bool takeOwnership);

    void show();
    void dismiss();

    void destroy();
private:
    HWND hwnd = 0;
    HWND parentHwnd = 0;
    Style style = Style::Information;
    String text;
    ULONGLONG targetTimeoutMSecs = 0ULL;
    UINT_PTR timerId = 0;
    HFONT font = 0;
    HBRUSH styleBrush = 0;
    int flags = 0;

    enum
    {
        OwnsFont = (1 << 1),
        Showing = (1 << 2),
        Dismissing = (1 << 3),
        DestroyAfterDismissed = (1 << 4),
    };

    void paintWindow();
    void updateWindowAnimation();
    void updateTimeout();
    void destroyFont();
    void setTimer(bool enabled);

    LRESULT wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK staticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static ATOM g_classAtom;
};