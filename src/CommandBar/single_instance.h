#pragma once
#include <Windows.h>


struct SingleInstance
{
    bool CheckOrInitInstanceLock(const wchar_t* instanceId);
    bool PostMessageToOtherInstance(UINT messageId, WPARAM wParam, LPARAM lParam);
};
