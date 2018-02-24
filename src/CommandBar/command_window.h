#pragma once
#define NOMINMAX
#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include "command_engine.h"
#include "newstring.h"
#include "newstring_builder.h"
#include "command_window_tray.h"

struct CommandWindowStyle;

struct CommandWindow
{
	bool Initialize(int windowWidth, int windowHeight, int nCmdShow);

	void ShowWindow();
	void HideWindow();
	void ToggleVisibility();

	void Exit();

	LRESULT WindowProc(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam);
	void beforeCommandRun();

	HWND hwnd = 0;

    ID2D1Factory* d2d1 = nullptr;
    IDWriteFactory* dwrite = nullptr;

	const CommandWindowStyle* style = nullptr;
	CommandEngine* commandEngine = nullptr;

    void Dispose();

	static const wchar_t* g_className;
	static const wchar_t* g_windowName;
    static const UINT g_showWindowMessageId;
private:
    bool isInitialized = false;
	bool shouldCatchInvalidUsageErrors = false;
	float showWindowYRatio = 0.4f;

	void Evaluate();

	static bool initGlobalResources(HINSTANCE hInstance);

    static HKL g_englishKeyboardLayout;
	static bool g_globalResourcesInitialized;
	static ATOM g_windowClass;
	static HICON g_appIcon;
public:
    void ClearText();
    bool SetText(const Newstring& text);
    void Redraw();
    void UpdateTextLayout(bool forced = false);
    void ClearSelection();

    NewstringBuilder textBuffer;
    uint32_t cursorPos = 0;
    Command* autocompletionCandidate = nullptr;

    CommandWindowTray tray;

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
    
    LRESULT OnChar(wchar_t c);
    LRESULT OnKeyDown(LPARAM lParam, WPARAM wParam);
    LRESULT OnMouseActivate();
    LRESULT OnLeftMouseButtonDown(LPARAM lParam, WPARAM wParam);
    LRESULT OnLeftMouseButtonUp();
    LRESULT OnMouseMove(LPARAM lParam, WPARAM wParam);
    LRESULT OnFocusAcquired();
    LRESULT OnFocusLost();
    LRESULT OnPaint();
    LRESULT OnHotkey(LPARAM lParam, WPARAM wParam);
    LRESULT OnTimer(LPARAM lParam, WPARAM wParam);
    LRESULT OnShowWindow(LPARAM lParam, WPARAM wParam);
    LRESULT OnQuit();
    LRESULT OnActivate();

    LRESULT OnCursorBlinkTimerElapsed();

    void onTextChanged();
    void onUserRequestedAutocompletion();
    Command* findAutocompletionCandidate();

    void updateAutocompletion();

    inline bool IsTextSelected()
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
    void AnimateWindow(WindowAnimation animation);
};

struct CommandWindowStyle
{
    COLORREF marginColor = 0;
    D2D1_COLOR_F borderColor;
    D2D1_COLOR_F textColor;
    D2D1_COLOR_F autocompletionTextColor;
    D2D1_COLOR_F selectedTextBackgroundColor;
    D2D1_COLOR_F textboxBackgroundColor;
    Newstring fontFamily;
    float fontHeight = 0.0f;
    DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_REGULAR;
    DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL;
    DWRITE_FONT_STRETCH fontStretch = DWRITE_FONT_STRETCH_NORMAL;
    float textMarginLeft = 0;
    int borderSize = 5;
    int textHeight = 0;
};
