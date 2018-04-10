#include <assert.h>
#include <Windows.h>
#include <CommCtrl.h>
#include <d2d1.h>
#include <dwrite.h>

#include "command_window.h"
#include "os_utils.h"
#include "allocators.h"
#include "command_engine.h"
#include "single_instance.h"
#include "basic_commands.h"
#include "command_loader.h"
#include "unicode.h"
#include "hint_window.h"
#include "newstring.h"
#include "newstring_builder.h"
#include "command_window_style_loader.h"
#include "defer.h"
#include "parse_ini.h"
#include "common.h"
#include "utils.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


SingleInstance g_singleInstanceGuard;

bool InitializeAllocators();
bool InitializeLibraries();
void DisposeAllocators();
void DisposeLibraries();

Newstring GetCommandsFilePath();
Newstring AskUserForCmdsFilePath();

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* lpCmdLine, int nCmdShow)
{
    if (!InitializeAllocators())
        return 1;
    defer(DisposeAllocators());

    if (!InitializeLibraries())
        return 1;
    defer(DisposeLibraries());

#ifndef _DEBUG
	if (!g_singleInstanceGuard.CheckOrInitInstanceLock(L"CommandBarSingleInstanceGuard"))
    {
		if (!g_singleInstanceGuard.PostMessageToOtherInstance(CommandWindow::g_showWindowMessageId, 0, 0))
        {
			MessageBoxW(0, L"Already running.", L"Command Bar", MB_OK);
		}

		return 0;
	}
#endif

	CommandEngine commandEngine;
    defer(commandEngine.Dispose());

	CommandWindowStyle windowStyle;

    Newstring styleFilePath = Newstring::WrapConstWChar(L"D:/Vlad/cb/style.ini");
    if (OSUtils::FileExists(styleFilePath))
    {
        if (!CommandWindowStyleLoader::LoadFromFile(styleFilePath, &windowStyle))
        {
            windowStyle = CommandWindowStyle();
            MessageBoxW(0, L"Failed to load style.", L"Error", MB_OK | MB_ICONERROR);
        }
    }

	CommandWindow commandWindow;
    defer(commandWindow.Dispose());

    CommandLoader commandLoader;
    RegisterBasicCommands(&commandLoader);

    Array<Command*> commands = commandLoader.LoadFromFile(GetCommandsFilePath());
    for (uint32_t i = 0; i < commandLoader.commandInfoArray.count; ++i)
        commandEngine.RegisterCommandInfo(commandLoader.commandInfoArray.data[i]);
    for (uint32_t i = 0; i < commands.count; ++i)
        commandEngine.RegisterCommand(commands.data[i]);

    nCmdShow = wcscmp(lpCmdLine, L"/noshow") == 0 ? 0 : SW_SHOW;

    bool initialized = commandWindow.Initialize(&commandEngine, &windowStyle, nCmdShow); // width=400, height=40
    if (!initialized)  return 1;

    MSG msg;
    uint32_t ret;

    while ((ret = GetMessageW(&msg, 0, 0, 0)) != 0)
    {
        if (ret == -1)
        {
            continue;
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);

        g_tempAllocator.Reset();
    }

    return static_cast<int>(msg.wParam);
}

Newstring AskUserForCmdsFilePath()
{
    HRESULT hr = E_UNEXPECTED;
    defer(
        if (FAILED(hr))
        {
            auto msg = Newstring::FormatCString(L"Error: 0x%08X.", hr);
            MessageBoxW(0, msg.data, L"Error", MB_ICONERROR);
        }
    );

    Newstring result;

    IFileOpenDialog* dialog = nullptr;
    defer(SafeRelease(dialog));

    hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_ALL,
        IID_IFileOpenDialog,
        reinterpret_cast<void**>(&dialog));
    if (FAILED(hr))  return result;

    dialog->SetTitle(L"Select cmds.ini");

    hr = dialog->Show(0); // @TODO: Handle user cancellation case.
    if (FAILED(hr))  return result;

    IShellItem* selection = nullptr;
    defer(SafeRelease(selection));

    hr = dialog->GetResult(&selection);  
    if (FAILED(hr))  return result;

    wchar_t* selectionTitle = nullptr;
    defer(
        if (selectionTitle)
        {
            CoTaskMemFree(selectionTitle);
            selectionTitle = nullptr;
        }
    );

    hr = selection->GetDisplayName(SIGDN_FILESYSPATH, &selectionTitle);
    if (FAILED(hr) || !selectionTitle)  return result;

    result = Newstring::WrapConstWChar(selectionTitle);
    result.count += 1; // Count terminating zero.

    if (!OSUtils::FileExists(result))
        return result;

    result = result.Clone();
    return result;
}

Newstring GetCommandsFilePath()
{
#ifdef _DEBUG
    static Newstring SettingsPath = Newstring::WrapConstWChar(L"settings_debug.ini");
#else
    static Newstring SettingsPath = Newstring::WrapConstWChar(L"settings.ini");
#endif

    NewstringBuilder settingsPath;
    defer(settingsPath.Dispose());

    OSUtils::GetApplicationDirectory(&settingsPath);
    settingsPath.Append(SettingsPath);
    settingsPath.ZeroTerminate();

    Newstring cmdsFile;
    Newstring settingsText = OSUtils::ReadAllText(settingsPath.string);
    defer(settingsText.Dispose());

    if (Newstring::IsNullOrEmpty(settingsText))
    {
        cmdsFile = AskUserForCmdsFilePath();
        if (Newstring::IsNullOrEmpty(cmdsFile))
        {
            settingsPath.Dispose();
            return Newstring::Empty();
        }

        Newstring settingsNewText = Newstring::Join({
            Newstring::WrapConstWChar(L"cmds_path="),
            Newstring(cmdsFile.data, cmdsFile.count - 1) // Don't count terminating zero. 
        }); 
        defer(settingsNewText.Dispose());
        assert(!Newstring::IsNullOrEmpty(settingsNewText));

        bool result = OSUtils::WriteAllText(settingsPath.string, settingsNewText);
        assert(result);
    }
    else
    {
        INIParser ps;
        ps.Initialize(settingsText);

        while (ps.Next())
        {
            if (ps.type == INIValueType::KeyValuePair && ps.key == L"cmds_path")
            {
                cmdsFile = Newstring::NewFromWChar(ps.value.data, ps.value.count);
                break;
            }
        }
    }

    return cmdsFile;
}

bool InitializeLibraries()
{
    HRESULT hr;

    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    if (!InitCommonControlsEx(&icc))
    {
        return ShowErrorBox(0, Newstring::FormatTempCStringWithFallback(
            L"Failed to initialize common controls.\n\nError code was 0x%08X.", GetLastError()));
    }

    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);
    if (FAILED(hr))
    {
        return ShowErrorBox(0, Newstring::FormatTempCStringWithFallback(
            L"Failed to initialize COM.\n\nError code was 0x%08X.", GetLastError()));
    }

    return true;
}

void DisposeAllocators()
{
    OutputDebugStringW(Newstring::FormatTempCString(L"std allocator leftover: %d\n", (int)g_standardAllocator.allocated).data);

    g_tempAllocator.Dispose();
}

void DisposeLibraries()
{
    CoUninitialize();
}

bool InitializeAllocators()
{
    if (!g_tempAllocator.SetSize(4096))
    {
        return ShowErrorBox(0, Newstring::WrapConstWChar(L"Unable to initialize temporary allocator."));
    }

    return true;
}