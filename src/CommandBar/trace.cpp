#include "trace.h"
#include <Windows.h>

bool Trace::g_isInitialized = false;

void Trace::init()
{
	g_isInitialized = true;
}

void Trace::debug(const char* text)
{
	OutputDebugStringA(text);
}

void Trace::error(const char* text)
{
	OutputDebugStringA(text);
	__debugbreak();
}
