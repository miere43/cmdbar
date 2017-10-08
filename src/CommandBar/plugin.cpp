//#include "plugin.h"
//#include <Windows.h>
//
//typedef int (__cdecl * CB_PluginInitFunc)(void * handle);
//
//int CB_PluginDiscover()
//{
//	WIN32_FIND_DATAW find;
//	HANDLE findHandle = FindFirstFileW(L"plugins\\*.dll", &find);
//
//	if (findHandle == INVALID_HANDLE_VALUE) return 0;
//
//	int exec = 0;
//	do
//	{
//		HMODULE module = LoadLibraryW(find.cFileName);
//		if (module == NULL) continue;
//		CB_PluginInitFunc initFunc = (CB_PluginInitFunc)GetProcAddress(module, "CB_PluginInit");
//		if (!initFunc)
//		{
//			FreeLibrary(module);
//			continue;
//		}
//
//		int j = 666;
//		initFunc((void*)&j);
//
//		++exec;
//	} while (FindNextFileW(findHandle, &find));
//	FindClose(findHandle);
//
//	return exec;
//}
