#pragma once
#define NOMINMAX
#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include "command_window_textbox.h"
#include "taskbar_icon.h"
#include "command_engine.h"


struct CommandWindowStyle;


struct CommandWindow
{
	bool init(HINSTANCE hInstance);

	void showWindow();
	void showAfterAllEventsProcessed();
	void hideWindow();
	void toggleVisibility();

	void exit();

	int enterEventLoop();
	LRESULT wndProc(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam);
	void beforeCommandRun();


	HWND hwnd = 0;
	HMENU trayMenu = 0;

    ID2D1Factory* d2d1 = nullptr;
    IDWriteFactory* dwrite = nullptr;

	const CommandWindowStyle* style = nullptr;
	TaskbarIcon taskbarIcon;
	CommandEngine* commandEngine = nullptr;

    int windowWidth = 400;
    int windowHeight = 40;

	static const wchar_t* g_className;
	static const wchar_t* g_windowName;
private:
    bool isInitialized = false;
	bool shouldCatchInvalidUsageErrors = false;
	float showWindowYRatio = 0.4f;

	void evaluate();
	void dispose();

	static bool initGlobalResources(HINSTANCE hInstance);

    static HKL g_englishKeyboardLayout;
	static bool g_globalResourcesInitialized;
	static ATOM g_windowClass;
	static HICON g_appIcon;

public:
    bool clearText();
    bool setText(const String& text);
    void redraw();
    void updateTextLayout(bool forced = false);
    void clearSelection();
    bool getSelectionRange(int* rangeStart, int* rangeLength);

    wchar_t* textBuffer = nullptr;
    int textBufferLength = 0;
    int textBufferMaxLength = 512;
    int cursorPos = 0;

    int selectionInitialPos = 0;
    int selectionPos = 0;
    int selectionLength = 0;

    HKL englishKeyboardLayout = 0;
    HKL originalKeyboardLayout = 0;

    ID2D1HwndRenderTarget* hwndRenderTarget = nullptr;
    IDWriteTextFormat* textFormat = nullptr;
    IDWriteTextLayout* textLayout = nullptr;
    ID2D1SolidColorBrush* textForegroundBrush = nullptr;
    ID2D1SolidColorBrush* selectedTextBrush = nullptr;

    bool isTextLayoutDirty = false;
    int clickX = -1;
    int clickCursorPos = -1;
    bool isMouseCaptured = false;
private:
    LRESULT paint(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

struct CommandWindowStyle
{
    COLORREF marginColor = 0;
    COLORREF backgroundColor = 0;
    COLORREF textColor = 0;
    COLORREF selectedTextBackgroundColor = 0;
    String fontFamily = { 0 };
    float fontHeight = 0.0f;
    DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_REGULAR;
    DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL;
    DWRITE_FONT_STRETCH fontStretch = DWRITE_FONT_STRETCH_NORMAL;
    float textMarginLeft = 0;
    int textHeight = 0;
};
