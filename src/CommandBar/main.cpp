#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include "trace.h"
#include "command_window.h"
#include "os_utils.h"
#include "allocators.h"
#include <CommCtrl.h>
#include "command_engine.h"
#include "features.h"
#include "one_instance.h"
#include <assert.h>


CommandWindow* g_commandWindow = nullptr;


void quitCommand(Command& command, const String* args, uint32_t numArgs)
{
    assert(g_commandWindow);
	g_commandWindow->exit();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* lpCmdLine, int nCmdShow)
{
#ifndef _DEBUG
	if (!checkOrInitInstanceLock()) {
		if (!postMessageToOtherInstance(g_oneInstanceMessage, 0, 0)) {
			MessageBoxW(0, L"Already running.", L"Command Bar", MB_OK);
		}

		return 0;
	}
#endif

	Trace::init();
	
	InitCommonControls();
	CoInitialize(nullptr);

	CommandEngine commandEngine;
	Command* quitCmd = new Command();
	quitCmd->name = clone("quit");
	quitCmd->callback = quitCommand;

	commandEngine.addCommand(quitCmd);

	Features features;
	features.load(&commandEngine);

	CommandWindowStyle windowStyle;
	windowStyle.backgroundColor = RGB(128, 128, 128);
	windowStyle.textMarginLeft = 2;
	windowStyle.textHeight = 26;
	windowStyle.textColor = GetSysColor(COLOR_WINDOWTEXT);//RGB(64, 64, 64);
	windowStyle.selectedTextBackgroundColor = RGB(0, 0, 0);
    windowStyle.backgroundColor = RGB(255, 255, 255); // GetSysColor(COLOR_WINDOW);//RGB(230, 230, 230);
	windowStyle.fontFamily = String(L"Segoe UI");
    windowStyle.fontHeight = 22.0f;
    windowStyle.fontStyle = DWRITE_FONT_STYLE_NORMAL;
    windowStyle.fontStretch = DWRITE_FONT_STRETCH_NORMAL;
    windowStyle.fontWeight = DWRITE_FONT_WEIGHT_REGULAR;

    ID2D1Factory* d2d1 = nullptr;
    IDWriteFactory* dwrite = nullptr;

    {
        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d1)))
        {
            // @TODO: better error message
            MessageBoxW(0, L"Cannot initialize Direct2D.", L"Error", MB_OK | MB_ICONERROR);
            return 1;
        }
        
        if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&dwrite))))
        {
            // @TODO: better error message
            MessageBoxW(0, L"Cannot initialize DirectWrite.", L"Error", MB_OK | MB_ICONERROR);
            return 1;
        }
    }

	CommandWindow commandWindow;
    commandWindow.windowWidth = 390;
    commandWindow.windowHeight = 30;
    commandWindow.style = &windowStyle;
    commandWindow.commandEngine = &commandEngine;
    commandWindow.d2d1 = d2d1;
    commandWindow.dwrite = dwrite;

	if (!commandWindow.init(hInstance))
	{
		__debugbreak();
		return 1;
	}

	g_commandWindow = &commandWindow;

	String commandLine { lpCmdLine };
	if (indexOf(commandLine, String(L"/noshow")) == -1)
		commandWindow.showAfterAllEventsProcessed(); // Make sure all controls are initialized.

	return commandWindow.enterEventLoop();
}



//#include "allocators.h"
//#include "context.h"
//#include "mainui.h"
//#include "common.h"
//#include "plugin.h"
//#include "trace.h"
//#include "strutils.h"
//
//#define CB_TEMP_ALLOCATOR_BUFFER_SIZE 4096
//#define CB_COPYDATA_BRING_TO_FRONT_ID 1
//static const wchar_t mutexHandleString[] = L"Global\\{947d8adb-d10b-4e50-96d3-4f24ffe4cc4e}";
//static const wchar_t mappingHandleString[] = L"{6bc44f0b-28ed-4a65-b49d-51b7c51bf7e8}";
//
//void debugTraceCallback(TraceLevel level, const char* msg, const char* fileName, int line, void* userdata)
//{
//	OutputDebugStringA(msg);
//	OutputDebugStringA("\n");
//}
//
//using namespace strutils;
//
//int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t * lpCmdLine, int nCmdShow)
//{
//	HANDLE instanceMutex = NULL;// = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, mutexHandleString);
//	HANDLE fileMapping = NULL;
//	//if (!instanceMutex)
//	//{
//	//	instanceMutex = CreateMutexW(NULL, TRUE, mutexHandleString);
//	//	fileMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL,
//	//		PAGE_READWRITE, 0, 4096, mappingHandleString);
//	//	if (!fileMapping)
//	//	{
//	//		int e = GetLastError();
//	//		MessageBoxW(NULL, L"Unable to create file mapping.", L"Error", MB_OK);
//	//	}
//	//}
//	//else 
//	//{
//	//	fileMapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mappingHandleString);
//	//	if (!fileMapping)
//	//	{
//	//		int e = GetLastError();
//	//		MessageBoxW(NULL, L"Unable to open file mapping.", L"Error", MB_OK);
//	//	}
//	//	else
//	//	{
//	//		void* data = MapViewOfFile(fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 32);
//	//		if (!data)
//	//		{
//	//			CloseHandle(fileMapping);
//	//			MessageBoxW(NULL, L"Unable to map file mapping.", L"Error", MB_OK);
//	//			return 0;
//	//		}
//	//		HWND target = *((HWND*)data);
//	//		if (!IsWindow(target)) {
//	//			UnmapViewOfFile(data);
//	//			CloseHandle(fileMapping);
//	//			MessageBoxW(NULL, L"Invalid target.", L"Error", MB_OK);
//	//			return 0;
//	//		}
//	//		COPYDATASTRUCT copyData;
//	//		copyData.dwData = CB_COPYDATA_BRING_TO_FRONT_ID;
//	//		copyData.cbData = sizeof(target);
//	//		copyData.lpData = &target;
//	//		SendMessageW(target, WM_COPYDATA, (WPARAM)0, &copyData);
//	//		UnmapViewOfFile(data);
//	//		CloseHandle(fileMapping);
//	//		return 0;
//	//	}
//	//	MessageBoxW(NULL, L"Sending WM_COPYDATA.", L"Error", MB_OK);
//	//	return 0;
//	//}
//
//	QueryPerformanceFrequency(&g_mainTickFrequency);
//	QueryPerformanceCounter(&g_mainStartupTick);
//
//	CB_InitAllocators(CB_TEMP_ALLOCATOR_BUFFER_SIZE);
//
//	Trace::init();
//	Trace::registerCallback(debugTraceCallback, nullptr);
//
//	CB_InitCoreFunctionality();
//
//	CB_Context context;
//	if (!CB_InitContext(&context)) {
//		return 1;
//	}
//
//	CB_LoadCommands(&context);
//
//	int v = CB_PluginDiscover();
//	
//	int result = CB_EnterMainUILoop(&context, hInstance, lpCmdLine, nCmdShow, fileMapping);
//	//CloseHandle(instanceMutex);
//	return result;
//}