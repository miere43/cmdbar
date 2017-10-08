//#pragma once
//#include "common.h"
//#include <Windows.h>
//#include "context.h"
//#include <vector>
//
//#define CB_MAX_AUTOCOMPLETION_RESULTS 4
//
//struct CB_TipUI_AutocompletionResult
//{
//	const CB_CommandDef* command;
//};
//
//struct CB_TipUI_AutocompletionStyle
//{
//	COLORREF backgroundColor;
//	COLORREF textColor;
//	
//	HFONT itemDrawFont;
//	int itemMarginX;
//	int itemMarginY;
//	int itemMaxHeight;
//
//	int marginBottom;
//};
//
//struct CB_TipUI
//{
//	HWND window;
//	HWND parent;
//	POINT showOffset;
//
//	std::vector<CB_CommandDef>* commands;
//
//	wchar_t * currCommandText;
//	size_t    currCommandLength;
//
//	int numAutocompletionResults;
//	CB_TipUI_AutocompletionResult autocompletionResults[CB_MAX_AUTOCOMPLETION_RESULTS];
//
//	CB_TipUI_AutocompletionStyle * autocompletionStyle;
//	TEXTMETRICW acStyleItemTextMetric;
//	int acStyleItemHeight;
//};
//
//bool CB_TipRegisterClass(HINSTANCE hInstance);
//bool CB_TipCreateWindow(CB_TipUI * data, HWND parent, HINSTANCE hInstance, int width, int height);
//void CB_TipShowWindow(CB_TipUI * data);
//void CB_TipHideWindow(CB_TipUI * data);
//void CB_TipSetCommandDefVector(CB_TipUI * data, std::vector<CB_CommandDef>* commands);
//void CB_TipSetInputText(CB_TipUI * data, wchar_t * text, size_t textLength);
//void CB_TipUpdateAutocompletion(CB_TipUI * data);
//void CB_TipSetAutocompletionStyle(CB_TipUI * data, CB_TipUI_AutocompletionStyle * style);
//
//
//inline void CB_TipSetShowOffset(CB_TipUI * data, int x, int y)
//{
//	data->showOffset.x = x;
//	data->showOffset.y = y;
//}
