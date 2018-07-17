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
#include "command_history.h"

struct CommandWindowStyle;

struct CommandWindow
{
	bool Initialize(CommandEngine* engine, CommandWindowStyle* style, int nCmdShow);

	void ShowWindow();
	void HideWindow();
	void ToggleVisibility();

	void Exit();

	LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND hwnd = 0;

    ID2D1Factory* d2d1 = nullptr;
    IDWriteFactory* dwrite = nullptr;

	const CommandWindowStyle* style = nullptr;
	CommandEngine* commandEngine = nullptr;

    /** When user opens command bar, it will show previous command as autocompletion suggestion. */
    bool showPreviousCommandAutocompletion = true;
    Command* showPreviousCommandAutocompletion_command = nullptr;

    void Dispose();

	static const wchar_t* g_className;
	static const wchar_t* g_windowName;
    static const UINT g_showWindowMessageId;
private:
    bool isInitialized = false;
	bool shouldCatchInvalidUsageErrors = false;
	float showWindowYRatio = 0.4f;

	void Evaluate();

    /** Returns size of client window part as two floats. */
    void GetClientSizeF(float* width, float* height);

    bool CreateGraphicsResources();
    void DisposeGraphicsResources();
	static bool InitializeStaticResources(HINSTANCE hInstance);

    static HKL g_englishKeyboardLayout;
	static ATOM g_windowClass;
	static HICON g_appIcon;
    static UINT g_taskbarCreatedMessageId;
public:
    void ClearText();
    bool SetText(const Newstring& text);
    void Redraw();
    void UpdateTextLayout(bool forced = false);
    void UpdateAutocompletionLayout();
    void TextEditChanged();
    bool ShouldDrawAutocompletion() const;
    void BeforeCommandRun();
    //void AfterCommandRun();

    /** Reloads commands from cmds.ini file. */
    void ReloadCommandsFile();

    TextEdit textEdit;
    CommandHistory history;

    Command* autocompletionCandidate = nullptr;

    /**
     * Indicates starting text position of mouse selection.
     * If currently there are no mouse selection, contains value of 0xFFFFFFFF.
     */
    uint32_t mouseSelectionStartPos = 0xFFFFFFFF;

    CommandWindowTray tray;

    ID2D1HwndRenderTarget* hwndRenderTarget = nullptr;
    IDWriteTextFormat* textFormat = nullptr;
    IDWriteTextLayout* textLayout = nullptr;
    ID2D1SolidColorBrush* textboxBackgroundBrush = nullptr;
    ID2D1SolidColorBrush* textForegroundBrush = nullptr;
    ID2D1SolidColorBrush* autocompletionTextForegroundBrush = nullptr;
    ID2D1SolidColorBrush* selectedTextBrush = nullptr;
    IDWriteTextLayout* autocompletionLayout = nullptr;

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
    LRESULT OnActivate(uint32_t activateState);

    LRESULT OnCursorBlinkTimerElapsed();

    void OnTextChanged();
    void OnUserRequestedAutocompletion();
    Command* FindAutocompletionCandidate();

    void UpdateAutocompletion();

    bool shouldDrawCaret = true;
    void SetCaretTimer();
    void KillCaretTimer();

    bool isTextLayoutDirty = false;
};

/**
 * Defines styling options for command window such as window color, font face, text color and so on.
 */
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
    int windowWidth = 400;
    int windowHeight = 40;
};
