#include <assert.h>
#include <windowsx.h>

#include "command_window_textbox.h"
#include "command_window.h"
#include "math.h"


ATOM CommandWindowTextbox::g_windowClass = 0;
bool CommandWindowTextbox::g_globalResourcesInitialized = false;

LRESULT __stdcall textboxWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


bool CommandWindowTextbox::init(HINSTANCE hInstance, CommandWindow * commandWindow, int x, int y, int width, int height)
{
	if (isInitialized)
		return true;

    assert(commandWindow);
    assert(style);

	if (!initGlobalResources(hInstance))
		return false;

	englishKeyboardLayout = LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE);

	textBufferMaxLength = 512;
	textBufferLength = 0;
    textBuffer = static_cast<wchar_t*>(malloc(textBufferMaxLength * sizeof(textBuffer[0])));
	if (textBuffer == nullptr)
		return false;
	
	commandWindow = commandWindow;
	hwnd = CreateWindowExW(
        0,
		(LPCWSTR)g_windowClass,
		L"Textbox",
		WS_CHILD | WS_TABSTOP | WS_VISIBLE,
		x,
		y,
		width,
		height,
		commandWindow->hwnd,
		0,
		hInstance,
		0);

	if (hwnd == 0) {
		free(textBuffer);

		return false;
	}

	this->style = style;
	this->commandWindow = commandWindow;

	SetWindowLongW(hwnd, GWLP_USERDATA, (LONG)this); // @TODO: will not work on x64
	//data->cursorTimer = SetTimer(data->window, CB_Textbox_CursorTimerID, 500, TextboxCursorTimerProc);


	{
		// Initialize Direct2D and DirectWrite resources.
		RECT size;
		GetClientRect(hwnd, &size);

		D2D1_SIZE_U d2dSize = D2D1::SizeU(size.right - size.left, size.bottom - size.top);

        HRESULT hr = 0;

		hr = dwrite->CreateTextFormat(
			style->fontFamily.data,
			nullptr,
            style->fontWeight,
            style->fontStyle,
            style->fontStretch,
			style->fontHeight,
			L"en-us",
			&textFormat);

		if (FAILED(hr))
			return false;

		textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

		//hr = interop->CreateFontFromLOGFONT(&styleFontLogfont, &font);
		//if (FAILED(hr))
		//	return false;

		hr = d2d1->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(hwnd, d2dSize, D2D1_PRESENT_OPTIONS_IMMEDIATELY),
			&hwndRenderTarget);
		if (FAILED(hr))
			return false;
	
		hr = hwndRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &textForegroundBrush);
		if (FAILED(hr))
			return false;
	
        hr = hwndRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Aqua), &textSelectionBrush);
        if (FAILED(hr))
            return false;

    }

	isInitialized = true;
	return isInitialized;
}

bool CommandWindowTextbox::clearText()
{
	textBuffer[0] = L'\0';
	textBufferLength = 0;
	
	cursorPos = 0;

	this->redraw();

	return true;
}

bool CommandWindowTextbox::setText(const String& text)
{
	if (text.data == nullptr)
		return false;

	int length = text.count;
	if (length > textBufferLength) return false;

	wmemcpy(textBuffer, text.data, length);
	textBufferLength = length;
	cursorPos = length;
    clearSelection();

    redraw();

	return true;
}

void CommandWindowTextbox::redraw()
{
    isTextLayoutDirty = true;
	InvalidateRect(hwnd, nullptr, true);
}

bool CommandWindowTextbox::initGlobalResources(HINSTANCE hInstance)
{
	if (g_globalResourcesInitialized)
		return true;

	g_globalResourcesInitialized = true;

	WNDCLASSW wc = { 0 };
	wc.hInstance = hInstance;
	wc.lpszClassName = L"CBTextbox";
	wc.hCursor = LoadCursorW(nullptr, IDC_IBEAM);
	wc.lpfnWndProc = textboxWndProc;
    wc.style = CS_OWNDC;

	g_windowClass = RegisterClassW(&wc);
	if (g_windowClass == 0)
		return false;

	return true;
}

void CommandWindowTextbox::clearSelection()
{
    selectionPos = 0;
    selectionLength = 0;
}

bool CommandWindowTextbox::getSelectionRange(int * rangeStart, int * rangeLength)
{
    assert(rangeStart);
    assert(rangeLength);

    if (selectionLength == 0)
        return false;

    *rangeStart  = selectionPos;
    *rangeLength = selectionLength;

    return true;
}

void CommandWindowTextbox::updateTextLayout(bool forced)
{
    if (textLayout == nullptr || isTextLayoutDirty || forced)
    {
        if (textLayout)
        {
            textLayout->Release();
            textLayout = nullptr;
        }

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        float clientWidth  = static_cast<float>(clientRect.right  - clientRect.left);
        float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);

        HRESULT hr;

        hr = dwrite->CreateTextLayout(
            textBuffer,
            textBufferLength,
            textFormat,
            clientWidth,
            clientHeight,
            &this->textLayout
        );

        assert(SUCCEEDED(hr));

        textLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        isTextLayoutDirty = false;
    }
}

LRESULT CommandWindowTextbox::paint(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ID2D1HwndRenderTarget* rt = this->hwndRenderTarget;
    assert(rt);

	RECT clientRect;
	GetClientRect(hwnd, &clientRect);
    float clientWidth  = static_cast<float>(clientRect.right  - clientRect.left);
    float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);

    float marginX = style->textMarginLeft;

    updateTextLayout();

	rt->BeginDraw();
	rt->Clear(D2D1::ColorF(D2D1::ColorF::White));

    float cursorRelativeX = 0.0f, cursorRelativeY = 0.0f;

    DWRITE_HIT_TEST_METRICS metrics;
    textLayout->HitTestTextPosition(cursorPos, false, &cursorRelativeX, &cursorRelativeY, &metrics);

    // Draw selection background
    int selectionStart = 0;
    int selectionLength = 0;
    if (getSelectionRange(&selectionStart, &selectionLength))
    {
        float unused;
        float selectionRelativeStartX = 0.0f;
        float selectionRelativeEndX = 0.0f;
        
        textLayout->HitTestTextPosition(selectionStart, false, &selectionRelativeStartX, &unused, &metrics);
        textLayout->HitTestTextPosition(selectionStart + selectionLength, false, &selectionRelativeEndX, &unused, &metrics);
    
        rt->FillRectangle(D2D1::RectF(marginX + selectionRelativeStartX, 0.0f, marginX + selectionRelativeEndX, clientHeight), textSelectionBrush);
    }

    rt->DrawTextLayout(D2D1::Point2F(marginX, 0.0f), textLayout, textForegroundBrush);

    // Center cursor X at pixel center to disable anti-aliasing.
    float cursorActualX = floor(marginX + cursorRelativeX) + 0.5f;
    rt->DrawLine(D2D1::Point2F(cursorActualX, 0.0f), D2D1::Point2F(cursorActualX, clientWidth), textForegroundBrush);

	rt->EndDraw();

    ValidateRect(hwnd, nullptr);
    return 0;
}

LRESULT CommandWindowTextbox::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_PAINT:
        {
            return paint(hwnd, msg, wParam, lParam);
        }

        //case WM_SETCURSOR:
        //{
        //    SetCursor(LoadCursorW(0, IDC_IBEAM));
        //    return 1;
        //}

		case WM_ERASEBKGND:
		{
			RECT dims;
			GetClientRect(hwnd, &dims);

			FillRect((HDC)wParam, &dims, style->backgroundBrush);

			return 1;
		}

		case WM_CHAR:
		{
			wchar_t wideChar = static_cast<wchar_t>(wParam); // "The WM_CHAR message uses Unicode Transformation Format (UTF)-16."

			if (!iswprint(wideChar)) return 0;

			if (textBufferLength >= textBufferMaxLength) return 0;
			if (insertCharAt(textBuffer, textBufferLength, textBufferMaxLength, cursorPos, wideChar))
			{
				++textBufferLength;
				++cursorPos;

                redraw();
			}

			SendMessageW(commandWindow->hwnd, WMAPP_TEXTBOX_TEXT_CHANGED, 0, (LPARAM)hwnd);

			return 0;
		}

		case WM_KEYDOWN:
		{
			BOOL shiftPressed = GetKeyState(VK_LSHIFT) & 0x8000;

			if (wParam == VK_BACK) // Backspace
			{
				if (textBufferLength <= 0) return 0;
                if (selectionLength != 0)
                {
                    if (removeRange(textBuffer, textBufferLength, selectionPos, selectionLength))
                    {
                        textBufferLength -= selectionLength;
                        textBuffer[textBufferLength] = L'\0';
                        if (selectionInitialPos <= cursorPos)
                            cursorPos -= selectionLength;
                        selectionLength = 0;
                    }
                }
				else if (cursorPos != 0 && removeCharAt(textBuffer, textBufferLength, cursorPos - 1))
				{
					--textBufferLength;
                    textBuffer[textBufferLength] = L'\0';
                    --cursorPos;
				}

                redraw();
			}
			else if (wParam == VK_DELETE)
			{
				if (textBufferLength <= 0) return 0;

                if (selectionLength != 0)
                {
                    if (removeRange(textBuffer, textBufferLength, selectionPos, selectionLength))
                    {
                        textBufferLength -= selectionLength;
                        textBuffer[textBufferLength] = L'\0';
                        if (selectionInitialPos <= cursorPos)
                            cursorPos -= selectionLength;
                        selectionLength = 0;
                    }
                }
				else if (cursorPos >= textBufferLength && removeCharAt(textBuffer, textBufferLength, cursorPos))
				{
					textBuffer[textBufferLength] = '\0';
					--textBufferLength;
				}

                redraw();
			}
			else if (wParam == VK_RETURN)
			{
				SendMessageW(GetParent(hwnd), WMAPP_TEXTBOX_ENTER_PRESSED, 0, (LPARAM)hwnd);
			}
			else if (wParam == VK_RIGHT)
			{
				if (cursorPos < textBufferLength)
				{
					if (shiftPressed) // High-order bit == 1 => key down
					{
                        if (selectionLength == 0)
                        {
                            selectionInitialPos = selectionPos = cursorPos;
                            selectionLength = 1;
                        }
                        else
                        {
                            if (cursorPos <= selectionInitialPos)
                            {
                                selectionPos = cursorPos + 1;
                                --selectionLength;
                            }
                            else
                            {
                                ++selectionLength;
                            }
                        }
					}

                    ++cursorPos;
                    redraw();
				}

                if (!shiftPressed)
                {
                    if (selectionLength != 0)
                    {
                        cursorPos = selectionPos + selectionLength;
                    }

                    selectionLength = 0;
                    isTextLayoutDirty = true;
                    InvalidateRect(hwnd, nullptr, true);
                }
			}
			else if (wParam == VK_LEFT)
			{
				if (cursorPos > 0)
				{
					--cursorPos;
					
                    if (shiftPressed)
                    {
                        if (selectionLength == 0)
                        {
                            selectionInitialPos = selectionPos = cursorPos;
                            selectionLength = 1;
                        }
                        else
                        {
                            if (cursorPos >= selectionInitialPos)
                            {
                                OutputDebugStringW(L"hello!\n");
                                //selectionPos = cursorPos;
                                --selectionLength;
                            }
                            else
                            {
                                selectionPos = cursorPos;
                                ++selectionLength;
                            }
                        }
                    }
                    
                    isTextLayoutDirty = true;
                    InvalidateRect(hwnd, nullptr, true);
				}

                if (!shiftPressed)
                {
                    if (selectionLength != 0)
                    {
                        cursorPos = selectionPos;
                    }

                    selectionLength = 0;
                    isTextLayoutDirty = true;
                    InvalidateRect(hwnd, nullptr, true);
                }
			}

			return 0;
		}

        case WM_LBUTTONDOWN:
        {
            int mouseX = GET_X_LPARAM(lParam);
            int mouseY = GET_Y_LPARAM(lParam);

            updateTextLayout(false);

            BOOL isInside = false;
            BOOL isTrailingHit = false;
            DWRITE_HIT_TEST_METRICS metrics = { 0 };
            HRESULT hr = 0;

            hr = textLayout->HitTestPoint(mouseX + style->textMarginLeft, mouseY, &isTrailingHit, &isInside, &metrics);
            assert(SUCCEEDED(hr));

            //selectionPos = metrics.textPosition;
            //selectionLength = 0;

            if (isTrailingHit)
            {
                cursorPos = metrics.textPosition + 1;
            }
            else
            {
                cursorPos = metrics.textPosition;
            }

            clickX = mouseX;
            clickCursorPos = cursorPos;

            clearSelection();
            redraw();

            return 0;
        }

        case WM_MOUSEMOVE:
        {
            int isLeftMouseDown = wParam & 0x0001;

            if (isLeftMouseDown && clickX != -1)
            {
                if (!isMouseCaptured)
                {
                    SetCapture(hwnd);
                    isMouseCaptured = true;
                }

                int mouseX = GET_X_LPARAM(lParam);
                int mouseY = GET_Y_LPARAM(lParam);
            
                updateTextLayout();

                BOOL isInside = false;
                BOOL isTrailingHit = false;
                DWRITE_HIT_TEST_METRICS metrics = { 0 };
                HRESULT hr = 0;

                hr = textLayout->HitTestPoint(mouseX + style->textMarginLeft, mouseY, &isTrailingHit, &isInside, &metrics);
                assert(SUCCEEDED(hr));

                if (clickX != -1)
                {
                    int oldCursorPos = clickCursorPos;

                    if (isTrailingHit)
                    {
                        cursorPos = metrics.textPosition + 1;
                    }
                    else
                    {
                        cursorPos = metrics.textPosition;
                    }

                    if (abs(oldCursorPos - cursorPos) == 0)
                    {
                        clearSelection();
                    }
                    else
                    {
                        selectionPos = min(cursorPos, oldCursorPos);
                        selectionLength = abs(oldCursorPos - cursorPos);
                    }
                }

                redraw();
            }

            return 0;
        }

        case WM_LBUTTONUP:
        {
            clickX = -1;
            clickCursorPos = -1;

            if (isMouseCaptured)
            {
                ReleaseCapture();
                isMouseCaptured = false;
            }

            return 0;
        }

		case WM_MOUSEACTIVATE:
		{
			if (GetFocus() != hwnd)
            {
				SetFocus(hwnd);
                redraw();
            }

			return MA_ACTIVATE;
		}

		//case WM_SETFOCUS:
		//{
		//	originalKeyboardLayout = GetKeyboardLayout(GetCurrentThreadId());
		//	ActivateKeyboardLayout(englishKeyboardLayout, KLF_REORDER);

		//	OutputDebugStringA("got focus.\n");
		//	break;
		//}

		//case WM_KILLFOCUS:
		//{
		//	//ActivateKeyboardLayout(originalKeyboardLayout, KLF_REORDER);

		//	OutputDebugStringA("lost focus.\n");
		//	InvalidateRect(hwnd, nullptr, true);
		//	return 0;
		//}

		case WM_GETTEXT:
		{
			int maxCharsToCopy = ((int)wParam) - 1;
			wchar_t * buffer = (wchar_t *)lParam;

			int numCharsToCopy = min(maxCharsToCopy, textBufferLength);
			memcpy(buffer, textBuffer, sizeof(wchar_t *) * numCharsToCopy);
			buffer[numCharsToCopy] = L'\0';

			return (LRESULT)numCharsToCopy;
		}
		case WM_GETTEXTLENGTH:
		{
			return (LRESULT)textBufferLength;
		}
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT __stdcall textboxWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_CREATE)
		return 0;

	CommandWindowTextbox* textbox = (CommandWindowTextbox*)GetWindowLongW(hwnd, GWLP_USERDATA);

	if (textbox != nullptr)
		return textbox->wndProc(hwnd, msg, wParam, lParam);

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}


//#include <wchar.h>
//#include <assert.h>
//#include "strutils.h"
//
//
//void __stdcall TextboxCursorTimerProc(HWND hwnd, UINT msg, UINT_PTR idEvent, DWORD dwTime);
//
//LRESULT __stdcall TextboxWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	

//
//	return DefWindowProcW(hwnd, msg, wParam, lParam);
//}
//
//
//int CB_TextboxSetTextChangedCallback(CB_TextboxUI * textbox, CB_Textbox_TextChanged callback)
//{
//	textChangedCallback = callback;
//	return 1;
//}
//


//
//void __stdcall TextboxCursorTimerProc(HWND hwnd, UINT msg, UINT_PTR idEvent, DWORD dwTime)
//{
//	if (idEvent != 1) return;
//	CB_TextboxUI * textbox = (CB_TextboxUI *)GetWindowLongW(hwnd, GWLP_USERDATA);
//
//	if (IsWindowVisible(hwnd))
//	{
//		CB_TextboxStyle * s = style;
//		
//		HDC hdc = GetDC(hwnd);
//		RECT rect;
//		rect.left = s->textMarginLeft + prevDrawTextExtent.cx;
//		rect.right = rect.left + 1;
//		rect.top = 4; // @TODO: marg
//		rect.bottom = 16;
//
//		// @TODO: ugly
//		HBRUSH brush;
//		if (shouldDrawCursorNow)
//		{
//			brush = CreateSolidBrush(s->textColor);
//		}
//		else
//		{
//			brush = backgroundBrush;
//		}
//		FillRect(hdc, &rect, brush);
//		if (shouldDrawCursorNow)
//		{
//			DeleteObject(brush);
//		}
//
//		OutputDebugStringA("redraw cursor\n");
//	}
//
//	shouldDrawCursorNow = !shouldDrawCursorNow;
////}