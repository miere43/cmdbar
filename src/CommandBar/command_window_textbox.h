#pragma once
#define NOMINMAX
#include <Windows.h>
#include "string_type.h"
#include <d2d1.h>
#include <dwrite.h>


struct CommandWindow;
struct CommandWindowTextboxStyle;

#define WMAPP_TEXTBOX_ENTER_PRESSED (WM_USER + 1)
#define WMAPP_TEXTBOX_TEXT_CHANGED  (WM_USER + 2)

struct CommandWindowTextbox
{
	bool init(HINSTANCE hInstance, CommandWindow* commandWindow, int x, int y, int width, int height);

	bool clearText();
	bool setText(const String& text);
	void redraw();
    void updateTextLayout(bool forced = false);
    void clearSelection();
    bool getSelectionRange(int* rangeStart, int* rangeLength);

	LRESULT wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND hwnd = 0;
	CommandWindow* commandWindow = nullptr;
	const CommandWindowTextboxStyle* style = nullptr;

	wchar_t* textBuffer = nullptr;
	int textBufferLength = 0;
	int textBufferMaxLength = 512;
	int cursorPos = 0;
	
    int selectionInitialPos = 0;
    int selectionPos = 0;
    int selectionLength = 0;

    HKL englishKeyboardLayout = 0;
	HKL originalKeyboardLayout = 0;

    ID2D1Factory* d2d1 = nullptr;
    IDWriteFactory* dwrite = nullptr;

	ID2D1HwndRenderTarget* hwndRenderTarget = nullptr;
	IDWriteTextFormat* textFormat = nullptr;
    IDWriteTextLayout* textLayout = nullptr;
	ID2D1SolidColorBrush* textForegroundBrush = nullptr;
    ID2D1SolidColorBrush* textSelectionBrush = nullptr;

    bool isTextLayoutDirty = false;
    int clickX = -1;
    int clickCursorPos = -1;
    bool isMouseCaptured = false;
private:
	bool isInitialized = false;

	LRESULT paint(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static bool initGlobalResources(HINSTANCE hInstance);

	static bool g_globalResourcesInitialized;
	static ATOM g_windowClass;
};

struct CommandWindowTextboxStyle
{
	HBRUSH backgroundBrush = 0;
	COLORREF backgroundColor = 0;
	COLORREF textColor = 0;
	COLORREF textSelectedColor = 0;
	COLORREF textSelectedBackgroundColor = 0;
	HBRUSH borderBrush = 0;
	HBRUSH borderFocusedBrush = 0;
    String fontFamily = { 0 };
    float fontHeight = 0.0f;
    DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_REGULAR;
    DWRITE_FONT_STYLE fontStyle = DWRITE_FONT_STYLE_NORMAL;
    DWRITE_FONT_STRETCH fontStretch = DWRITE_FONT_STRETCH_NORMAL;
	float textMarginLeft = 0;
	int textHeight = 0;
};

//#include "common.h"
//#include <Windows.h>
//

//
////enum CB_Textbox_TextChangeKind
////{
////	CB_TextChange_CharAdded,
////	CB_TextChange_CharRemoved,
////};
//
//enum {
//	CB_Textbox_CursorTimerID = 1,
//};
//
//struct CB_TextboxUI;
//typedef void(*CB_Textbox_TextChanged)(struct CB_TextboxUI * textbox, wchar_t changedChar);
//
//struct CB_TextboxStyle
//{
//	COLORREF backgroundColor;
//	COLORREF textColor;
//	COLORREF textSelectedColor;
//	COLORREF textSelectedBackgroundColor;
//	HFONT    textFont;
//	int      textMarginLeft;
//	int      textFontHeight;
//	COLORREF borderColor;
//	COLORREF borderFocusedColor;
//};
//
//struct CB_TextboxUI
//{
//	HWND window;
//	HWND parent;
//	UINT_PTR cursorTimer;
//	bool shouldDrawCursorNow;
//
//	CB_TextboxStyle * style;
//	HBRUSH textBrush;
//	HBRUSH backgroundBrush;
//	HBRUSH borderBrush;
//	HBRUSH borderFocusedBrush;
//
//	wchar_t * textBuffer;
//	size_t    textBufferLength;
//	size_t    textBufferMaxLength;
//	size_t    cursorPos;
//	size_t    selectionBase;
//
//	int    *  textExtentBuffer;
//
//	CB_Textbox_TextChanged textChangedCallback;
//};
//
//bool CB_TextboxRegisterClass(HINSTANCE hInstance);
////void    CB_TextboxSetStyle(CB_Textbox * data, CB_TextboxStyle * style);
//int     CB_TextboxCreateWindow(CB_TextboxUI * data, HWND parent, HINSTANCE hInstance, int x, int y, int width, int height);
//int     CB_TextboxSetStyle(CB_TextboxUI * textbox, CB_TextboxStyle * style);
//int     CB_TextboxSetTextChangedCallback(CB_TextboxUI * textbox, CB_Textbox_TextChanged callback);
//bool CB_TextboxSetText(CB_TextboxUI * textbox, wchar_t * text);
//
