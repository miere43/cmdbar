#pragma once
#define NOMINMAX
#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include "command_engine.h"
#include "newstring.h"
#include "newstring_builder.h"
#include "command_window_tray.h"
#include "text_edit.h"

struct CommandWindowStyle;

struct CommandWindow
{
	bool Initialize(int windowWidth, int windowHeight, int nCmdShow);

	void ShowWindow();
	void HideWindow();
	void ToggleVisibility();

	void Exit();

	LRESULT WindowProc(HWND hwnd, UINT msg, LPARAM lParam, WPARAM wParam);
	void BeforeCommandRun();

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

    bool CreateGraphicsResources();
    void DisposeGraphicsResources();

	static bool InitializeStaticResources(HINSTANCE hInstance);

    static HKL g_englishKeyboardLayout;
	static bool g_staticResourcesInitialized;
	static ATOM g_windowClass;
	static HICON g_appIcon;
public:
    void ClearText();
    bool SetText(const Newstring& text);
    void Redraw();
    void UpdateTextLayout(bool forced = false);
    void ClearSelection();
    void TextEditChanged();

    TextEdit textEdit;

    Command* autocompletionCandidate = nullptr;

    /**
     * Indicates starting text position of mouse selection.
     * If currently there are no mouse selection, contains value of 0xFFFFFFFF.
     */
    uint32_t mouseSelectionStartPos = 0xFFFFFFFF;

    CommandWindowTray tray;

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

    void OnTextChanged();
    void OnUserRequestedAutocompletion();
    Command* FindAutocompletionCandidate();

    void UpdateAutocompletion();

    bool shouldDrawCursor = true;
    void SetCursorTimer();
    void killCursorTimer();

    bool isTextLayoutDirty = false;
private:
    /**
     * Represents animations that can be played using AnimateWindow method.
     */
    enum class WindowAnimation
    {
        /**
         * Fade in from zero opacity to full opacity.
         */
        Show = 1,
        /**
         * Fade out from full opacity to zero opacity.
         */
        Hide = 2,
    };

    /**
     * Creates new message loop and does specified window animation.
     */
    void AnimateWindow(WindowAnimation animation);
};

struct CommandWindowStyle
{
    COLORREF marginColor = 0;
    D2D1_COLOR_F borderColor = D2D1::ColorF(D2D1::ColorF::Black);
    D2D1_COLOR_F textColor = D2D1::ColorF(D2D1::ColorF::Black);
    D2D1_COLOR_F autocompletionTextColor = D2D1::ColorF(D2D1::ColorF::Gray);
    D2D1_COLOR_F selectedTextBackgroundColor = D2D1::ColorF(D2D1::ColorF::Aqua);
    D2D1_COLOR_F textboxBackgroundColor = D2D1::ColorF(D2D1::ColorF::White);
    Newstring fontFamily = Newstring::WrapConstWChar(L"Segoe UI");
    float fontHeight = 22.0f;
    DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_REGULAR;
    DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL;
    DWRITE_FONT_STRETCH fontStretch = DWRITE_FONT_STRETCH_NORMAL;
    float textMarginLeft = 4.0f;
    int borderSize = 5;
    int textHeight = 0;
};
