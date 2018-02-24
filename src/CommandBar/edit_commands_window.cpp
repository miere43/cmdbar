#include "edit_commands_window.h"
#include <CommCtrl.h>


bool EditCommandsWindow::g_globalResourcesInitialized = false;
ATOM EditCommandsWindow::g_windowClass = 0;


enum ControlId {
    idList = 1,
    idLabel,
};

bool EditCommandsWindow::init(HWND parent, CommandEngine* commandEngine)
{
    if (isInitialized)
        return true;

    if (parent == 0)
        return false;
    if (commandEngine == nullptr)
        return false;

    if (!initGlobalResources())
        return false;

    this->commandEngine = commandEngine;
    HWND tempHwnd = CreateWindowExW(
        0,
        (LPCWSTR)g_windowClass,
        L"Edit commands",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        640,
        480,
        parent,
        0,
        GetModuleHandleW(0),
        this
    );

    if (tempHwnd == 0) {
        hwnd = 0;
        this->commandEngine = nullptr;
        return false;
    }

    hwnd = tempHwnd;

    isInitialized = true;
    return true;
}

LRESULT EditCommandsWindow::onCreate()
{
    HINSTANCE hInstance = GetModuleHandleW(0);

    LOGFONTW lf = {};
    lf.lfHeight = 18;
    lstrcpyW(lf.lfFaceName, L"Segoe UI");

    font = CreateFontIndirectW(&lf);

    listHwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WC_LISTBOXW,
        L"help",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_OWNERDRAWFIXED | LBS_NOINTEGRALHEIGHT | LBS_DISABLENOSCROLL | LBS_NOTIFY,
        8, 8,
        150,
        400,
        hwnd,
        (HMENU)idList,
        hInstance,
        0);
    if (listHwnd == 0)
        return -1;

    SendMessageW(listHwnd, WM_SETFONT, (WPARAM)font, 0);

    for (int i = 0; i < commandEngine->commands.count; ++i)
    {
        Command* command = commandEngine->commands.data[i];
        SendMessageW(listHwnd, LB_ADDSTRING, 0, (LPARAM)command);
    }

    labelHwnd = CreateWindowExW(
        0,
        WC_STATICW,
        L"Label: ",
        WS_CHILD | WS_VISIBLE,
        150 + 16, 8,
        160,
        40,
        hwnd,
        (HMENU)idLabel,
        hInstance,
        0);
    if (labelHwnd == 0)
        return -1;
    
    SendMessageW(labelHwnd, WM_SETFONT, (WPARAM)font, 0);

    return 0;
}

LRESULT EditCommandsWindow::onSize(WPARAM type, int newWidth, int newHeight)
{
    const int numWindows = 1;
    HDWP pos = BeginDeferWindowPos(numWindows);

    int w = newWidth;
    int h = newHeight;

    DeferWindowPos(pos, listHwnd, 0, 8, 8, 150, h - 16, SWP_NOZORDER);

    EndDeferWindowPos(pos);

    return 0;
}

bool EditCommandsWindow::initGlobalResources()
{
    if (g_globalResourcesInitialized)
        return true;

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.lpszClassName = L"EditCommandsWindow";
    wc.hInstance = GetModuleHandleW(0);
    wc.lpfnWndProc = staticWndProc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hCursor = LoadCursorW(0, IDC_ARROW);
    wc.hIcon = LoadIconW(0, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);

    g_windowClass = RegisterClassExW(&wc);
    if (g_windowClass == 0)
        return false;

    g_globalResourcesInitialized = true;
    return true;
}

LRESULT WINAPI EditCommandsWindow::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            return onCreate();
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case idList:
                {
                    switch (HIWORD(wParam))
                    {
                        case LBN_SELCHANGE:
                        {
                            int selection = (int)SendMessageW(listHwnd, LB_GETCURSEL, 0, 0);
                            if (selection == -1)
                                return 0;
                            return 0; // @TODO
                        }
                    }
                }
            }
            break;
        }
        case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT* item = (MEASUREITEMSTRUCT*)lParam;
            if (item == nullptr)
                return 1;
            switch (item->CtlID)
            {
                case idList:
                {
                    item->itemWidth = 100;
                    item->itemHeight = 20;
                    break;
                }
            }
            return 1;
        }
        case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT* item = (DRAWITEMSTRUCT*)lParam;
            if (item == nullptr)
                return 1;
            switch (item->CtlID)
            {
                case idList:
                {
                    if (item->itemState & ODS_SELECTED)
                    {
                        FillRect(item->hDC, &item->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
                        SetBkColor(item->hDC, RGB(0,0,0));
                    }
                    else {
                        FillRect(item->hDC, &item->rcItem, (HBRUSH)GetStockObject(WHITE_BRUSH));
                    }

                    
                    Command* command = (Command*)item->itemData;
                    wchar_t* text = command ? command->name.data : L"null";
                    int stringLength = command ? command->name.count : ARRAYSIZE(L"null");
                    DrawTextW(item->hDC, text, stringLength, &item->rcItem, 0);

                    
                    return 1;
                }
            }
            return 1;
        }
        case WM_SIZE:
        {
            return onSize(wParam, LOWORD(lParam), HIWORD(lParam));
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT WINAPI EditCommandsWindow::staticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    EditCommandsWindow* window = nullptr;

    if (msg == WM_CREATE)
    {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        if (cs == nullptr)  return 1;

        window = reinterpret_cast<EditCommandsWindow*>(cs->lpCreateParams);
        if (window == nullptr)  return 1;

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->hwnd = hwnd;
    }
    else
    {
        window = reinterpret_cast<EditCommandsWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    return window ? window->wndProc(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}

