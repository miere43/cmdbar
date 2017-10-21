#pragma once


struct Trace
{
	static void init();
	static void debug(const char* text);
	static void error(const char* text);

private:
	static bool g_isInitialized;
};
