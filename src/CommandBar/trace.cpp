#include "trace.h"
#include <Windows.h>

bool Trace::g_isInitialized = false;

void Trace::init()
{
	g_isInitialized = true;
}

void Trace::debug(const char * text)
{
	OutputDebugStringA(text);
}

void Trace::error(const char * text)
{
	OutputDebugStringA(text);
	__debugbreak();
}


//#include "trace.h"
//#include <Windows.h>
//
//
//namespace Trace {
//
//	TraceHandlerList* g_traceHandlerFirst = NULL;
//
//	void Trace::init()
//	{
//	}
//
//	void Trace::trace(TraceLevel level, const char * msg, const char * fileName, int line)
//	{
//		TraceHandlerList* current = g_traceHandlerFirst;
//		while (current)
//		{
//			current->callback(level, msg, fileName, line, current->userdata);
//			current = current->next;
//		}
//	}
//
//	void Trace::registerCallback(CB_TraceHandler callback, void * userdata)
//	{
//		TraceHandlerList* trace = (TraceHandlerList*)malloc(sizeof(struct TraceHandlerList));
//		trace->callback = callback;
//		trace->userdata = userdata;
//		trace->next = NULL;
//
//		if (g_traceHandlerFirst == NULL) {
//			g_traceHandlerFirst = trace;
//		} else {
//			TraceHandlerList* current = g_traceHandlerFirst;
//			while (current->next) continue;
//			current->next = trace;
//		}
//	}
//
//}