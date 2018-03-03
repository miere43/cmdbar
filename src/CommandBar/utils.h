#pragma once
#include "common.h"
#include "newstring.h"


/**
 * Displays generic error box with specified text. Always returns false.
 */
bool ShowErrorBox(HWND hwnd, const Newstring& text);
