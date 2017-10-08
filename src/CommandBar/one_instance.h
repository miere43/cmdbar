#pragma once
#include <Windows.h>

const UINT g_oneInstanceMessage = WM_USER + 64;

bool checkOrInitInstanceLock();
bool postMessageToOtherInstance(UINT message, WPARAM wParam, LPARAM lParam);
