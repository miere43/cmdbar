#pragma once
#define NOMINMAX
#include <stdint.h>
#include <initializer_list>

template<typename T>
void SafeRelease(T*& com_ptr)
{
    if (com_ptr)
    {
        com_ptr->Release();
        com_ptr = nullptr;
    }
}
