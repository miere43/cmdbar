#pragma once
#include <d2d1.h>
#include <dwrite.h>


struct GraphicsContext
{
    ID2D1Factory* d2d1 = nullptr;
    IDWriteFactory* dwrite = nullptr;
};

GraphicsContext* GetGraphicsContext();
