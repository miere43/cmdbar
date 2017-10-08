//#include "uiopts.h"
//
//void CB_LoadUIOptions()
//{
//	g_uiOptions.cursorBlinkRate = 500; // Default Windows cursor blink time.
//
//	HKEY hkey;
//	if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hkey))
//	{
//		// for some weird reason cursorblinkrate is stored as string :/
//		wchar_t maxCursorBlinkDigits[10];
//		DWORD size = sizeof(maxCursorBlinkDigits);
//		if (ERROR_SUCCESS == RegGetValueW(hkey, NULL, L"CursorBlinkRate", RRF_RT_REG_SZ, NULL, maxCursorBlinkDigits, &size))
//		{
//			maxCursorBlinkDigits[sizeof(maxCursorBlinkDigits) / sizeof(maxCursorBlinkDigits[0]) - 1] = L'\0';
//			int rate = _wtoi(maxCursorBlinkDigits);
//			if (rate >= 0)
//			{
//				g_uiOptions.cursorBlinkRate = rate;
//			}
//		}
//
//		RegCloseKey(hkey);
//	}
//}
//
//struct CB_UIOptions g_uiOptions;
