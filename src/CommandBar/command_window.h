#pragma once
#define NOMINMAX
#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include "taskbar_icon.h"
#include "command_engine.h"


struct CommandWindowStyle;

struct CommandWindow
{
	bool init(HINSTANCE hInstance, int windowWidth, int windowHeight);

	void showWindow();
	void showAfterAllEventsProcessed();
	void hideWindow();
	void toggleVisibility();

	void exit();

	int enterEventLoop();
	LRESULT wndProc(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam);
	void beforeCommandRun();

	HWND hwnd = 0;

    ID2D1Factory* d2d1 = nullptr;
    IDWriteFactory* dwrite = nullptr;

	const CommandWindowStyle* style = nullptr;
	TaskbarIcon taskbarIcon;
	CommandEngine* commandEngine = nullptr;

    // Tray Menu
    HMENU trayMenu = 0;

    void dispose();

	static const wchar_t* g_className;
	static const wchar_t* g_windowName;
    static const UINT g_showWindowMessageId;
private:
    bool isInitialized = false;
	bool shouldCatchInvalidUsageErrors = false;
	float showWindowYRatio = 0.4f;

	void evaluate();

	static bool initGlobalResources(HINSTANCE hInstance);

    static HKL g_englishKeyboardLayout;
	static bool g_globalResourcesInitialized;
	static ATOM g_windowClass;
	static HICON g_appIcon;

public:
    void clearText();
    bool setText(const String& text);
    void redraw();
    void updateTextLayout(bool forced = false);
    void clearSelection();
    bool getSelectionRange(int* rangeStart, int* rangeLength);

    String textBuffer;
    uint32_t textBufferMaxLength = 512;
    uint32_t cursorPos = 0;
    Command* autocompletionCandidate = nullptr;

    int selectionInitialPos = 0;
    int selectionPos = 0;
    int selectionLength = 0;

    HKL originalKeyboardLayout = 0;

    ID2D1HwndRenderTarget* hwndRenderTarget = nullptr;
    IDWriteTextFormat* textFormat = nullptr;
    IDWriteTextLayout* textLayout = nullptr;
    ID2D1SolidColorBrush* textboxBackgroundBrush = nullptr;
    ID2D1SolidColorBrush* textForegroundBrush = nullptr;
    ID2D1SolidColorBrush* autocompletionTextForegroundBrush = nullptr;
    ID2D1SolidColorBrush* selectedTextBrush = nullptr;
    
    LRESULT onChar(wchar_t c);
    LRESULT onKeyDown(LPARAM lParam, WPARAM wParam);
    LRESULT onMouseActivate();
    LRESULT onLeftMouseButtonDown(LPARAM lParam, WPARAM wParam);
    LRESULT onLeftMouseButtonUp();
    LRESULT onMouseMove(LPARAM lParam, WPARAM wParam);
    LRESULT onFocusAcquired();
    LRESULT onFocusLost();
    LRESULT onPaint();
    LRESULT onHotkey(LPARAM lParam, WPARAM wParam);
    LRESULT onTimer(LPARAM lParam, WPARAM wParam);
    LRESULT onShowWindow(LPARAM lParam, WPARAM wParam);
    LRESULT onQuit();
    LRESULT onActivate();

    LRESULT onCursorBlinkTimerElapsed();

    void onTextChanged();
    void onUserRequestedAutocompletion();
    Command* findAutocompletionCandidate();

    void updateAutocompletion();

    inline bool isTextBufferFilled()
    {
        return textBuffer.count >= textBufferMaxLength;
    }
    inline bool isTextSelected()
    {
        return selectionLength != 0;
    }

    bool shouldDrawCursor = true;
    //void setShouldDrawCursor(bool shouldDrawCursor);
    void setCursorTimer();
    void killCursorTimer();

    bool isTextLayoutDirty = false;

    // Selection with mouse variables
    // If -1 then there were no selection.
    int selectionStartCursorPos = -1;
private:
    enum class WindowAnimation
    {
        Show = 1,
        Hide = 2,
    };

    // This function blocks!
    void animateWindow(WindowAnimation animation);
};

struct CommandWindowStyle
{
    COLORREF marginColor = 0;
    D2D1_COLOR_F borderColor;
    D2D1_COLOR_F textColor;
    D2D1_COLOR_F autocompletionTextColor;
    D2D1_COLOR_F selectedTextBackgroundColor;
    D2D1_COLOR_F textboxBackgroundColor;
    String fontFamily = { 0 };
    float fontHeight = 0.0f;
    DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_REGULAR;
    DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL;
    DWRITE_FONT_STRETCH fontStretch = DWRITE_FONT_STRETCH_NORMAL;
    float textMarginLeft = 0;
    int borderSize = 5;
    int textHeight = 0;
};
