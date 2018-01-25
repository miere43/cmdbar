#pragma once
#include <Windows.h>


struct SingleInstance
{
    bool checkOrInitInstanceLock(const wchar_t* instanceId);
    bool postMessageToOtherInstance(UINT messageId, WPARAM wParam, LPARAM lParam);
};
