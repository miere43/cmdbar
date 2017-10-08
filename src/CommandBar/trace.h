#pragma once

struct Trace
{
	static void init();
	static void debug(const char* text);
	static void error(const char* text);

private:
	static bool g_isInitialized;
};

//#include "common.h"
//
//#define CB_WARN(m_msg) CB_Warn(m_msg, __FILE__, __LINE__)
//
//enum class TraceLevel
//{
//	Debug = 0,
//	Warning = 1,
//	Error = 2
//};
//
//typedef void (*CB_TraceHandler)(TraceLevel level, const char* msg, const char* fileName, int line, void* userdata);
//
//namespace Trace
//{
//	struct TraceHandlerList
//	{
//		CB_TraceHandler callback;
//		void* userdata;
//
//		struct TraceHandlerList* next;
//	};
//
//	void init();
//	void trace(TraceLevel level, const char* msg, const char* fileName, int line);
//	void registerCallback(CB_TraceHandler callback, void* userdata);
//}
