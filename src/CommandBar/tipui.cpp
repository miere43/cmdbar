//#include "tipui.h"
//
//
//LRESULT __stdcall CB_TipWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
//{
//	switch (Msg)
//	{
//		case WM_CREATE: {
//			return 0;
//		}
//
//		case WM_PAINT: {
//			PAINTSTRUCT ps;
//			HDC hdc = BeginPaint(hWnd, &ps);
//
//			CB_TipUI * tip = (CB_TipUI *)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
//			if (!tip) {
//				__debugbreak();
//			}
//			
//			CB_TipUI_AutocompletionStyle * s = tip->autocompletionStyle;
//		
//			HFONT hPrevFont = (HFONT)SelectObject(hdc, s->itemDrawFont);
//
//			SetTextColor(hdc, s->textColor);
//			SetBkColor(hdc, s->backgroundColor);
//
//			int x = s->itemMarginX;
//			int y = s->itemMarginY;
//			for (int i = 0; i < tip->numAutocompletionResults; ++i)
//			{
//				struct CB_TipUI_AutocompletionResult * result = &tip->autocompletionResults[i];
//				
//				wchar_t* str = result->command->name;
//				size_t   len = wcslen(str);
//				
//				TextOutW(hdc, x, y, str, len);
//				y += tip->acStyleItemHeight;
//			}
//
//			SelectObject(hdc, hPrevFont);
//
//			EndPaint(hWnd, &ps);
//			return 0;
//		}
//	}
//	return DefWindowProcW(hWnd, Msg, wParam, lParam);
//}
//
//bool CB_TipRegisterClass(HINSTANCE hInstance)
//{
//	WNDCLASSW wc;
//	memset(&wc, 0, sizeof(wc));
//
//	wc.hInstance = hInstance;
//	wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(64, 64, 64));
//	wc.lpszClassName = L"CBTooltipWindow";
//	wc.style = CS_VREDRAW | CS_HREDRAW;
//	wc.lpfnWndProc = CB_TipWndProc;
//
//
//	return RegisterClassW(&wc) != 0;
//}
//
//void CB__TipShouldRedraw(CB_TipUI * data)
//{
//	if (!data) return;
//
//	RECT rect;
//	GetWindowRect(data->window, &rect);
//
//	RECT parentRect;
//	GetWindowRect(data->parent, &parentRect);
//
//	if (data->numAutocompletionResults <= 0) {
//		CB_TipHideWindow(data);
//	} else {
//		int height = data->numAutocompletionResults * data->acStyleItemHeight + data->autocompletionStyle->marginBottom;
//		SetWindowPos(data->window, HWND_TOPMOST, parentRect.left + data->showOffset.x, parentRect.top + data->showOffset.y, rect.right - rect.left, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
//		//InvalidateRect(data->window, NULL, TRUE);
//	}
//}
//
//bool CB_TipCreateWindow(CB_TipUI* data, HWND parent, HINSTANCE hInstance, int width, int height)
//{
//	if (!data) return 0;
//
//	memset(data, 0, sizeof(*data));
//	data->parent = parent;
//	data->window = CreateWindowExW(WS_EX_TOPMOST, L"CBTooltipWindow", L"", WS_POPUP, 0, 0, width, height, parent, 0, GetModuleHandle(0), 0);
//	if (!data->window) return 0;
//	SetWindowLongPtrW(data->window, GWLP_USERDATA, (LONG_PTR)data);
//
//	UpdateWindow(data->window);
//
//	return 1;
//}
//
//void CB_TipSetAutocompletionStyle(CB_TipUI * data, CB_TipUI_AutocompletionStyle * style)
//{
//	data->autocompletionStyle = style;
//	
//	HDC hdc = GetDC(data->window);
//	HFONT hPrevFont = (HFONT)SelectObject(hdc, (HGDIOBJ)style->itemDrawFont);
//
//	GetTextMetricsW(hdc, &data->acStyleItemTextMetric);
//	data->acStyleItemHeight = data->acStyleItemTextMetric.tmHeight
//		+
//		style->itemMarginY;
//}
//
//void CB_TipShowWindow(CB_TipUI* data)
//{
//	if (!data) return;
//
//	RECT parentRect;
//	GetWindowRect(data->parent, &parentRect);
//
//	SetWindowPos(data->window, data->parent, parentRect.left + data->showOffset.x, parentRect.top + data->showOffset.y, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
//}
//
//void CB_TipHideWindow(CB_TipUI * data)
//{
//	if (!data) return;
//
//	ShowWindow(data->window, SW_HIDE);
//}
//
//void CB_TipSetCommandDefVector(CB_TipUI * data, std::vector<CB_CommandDef>* commands)
//{
//	if (!data) return;
//
//	data->commands = commands;
//	CB_TipUpdateAutocompletion(data);
//}
//
//void CB_TipSetInputText(CB_TipUI * data, wchar_t * text, size_t textLength)
//{
//	if (!data) return;
//
//	free(data->currCommandText);
//
//	if (text == NULL || textLength == 0)
//	{
//		data->currCommandText = NULL;
//		data->currCommandLength = 0;
//	}
//	else
//	{
//		data->currCommandText = (wchar_t*)::malloc(sizeof(wchar_t) * (textLength + 1));
//		memcpy(data->currCommandText, text, sizeof(wchar_t) * textLength);
//		data->currCommandText[textLength] = L'\0';
//		data->currCommandLength = textLength;
//	}
//	
//	CB_TipUpdateAutocompletion(data);
//}
//
//void CB__TipAddAutocompletionResult(CB_TipUI * data, const CB_CommandDef * def)
//{
//	if (data->numAutocompletionResults >= CB_MAX_AUTOCOMPLETION_RESULTS) return;
//	data->autocompletionResults[data->numAutocompletionResults++].command = def;
//}
//
//void CB_TipUpdateAutocompletion(CB_TipUI * data)
//{
//	if (!data) return;
//
//	data->numAutocompletionResults = 0;
//	if (data->currCommandText == NULL || data->currCommandLength == 0) {
//		CB__TipShouldRedraw(data);
//		return;
//	};
//
//	for (int i = 0; i < data->commands->size(); ++i)
//	{
//		const CB_CommandDef& command = data->commands->operator[](i);
//
//		if (wcsstr(command.name, data->currCommandText) != NULL)
//		{
//			CB__TipAddAutocompletionResult(data, &command);
//		};
//	}
//
//	CB__TipShouldRedraw(data);
//}
//
