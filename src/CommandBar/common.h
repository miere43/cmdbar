#pragma once
#define NOMINMAX
#include <stdint.h>
#include <Windows.h>

template<typename T>
void SafeRelease(T*& com_ptr)
{
    if (com_ptr)
    {
        com_ptr->Release();
        com_ptr = nullptr;
    }
}

