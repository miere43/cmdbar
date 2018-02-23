#include <assert.h>

#include "basic_commands.h"
#include "command_loader.h"
#include "command_window.h"
#include "parse_utils.h"


Command* runApp_createCommand(Array<String>& keys, Array<String>& values);
Command* openDir_createCommand(Array<String>& keys, Array<String>& values);
Command* quit_createCommand(Array<String>& keys, Array<String>& values);


Command* openDir_createCommand(Array<String>& keys, Array<String>& values)
{
    OpenDirCommand* cmd = stdNew<OpenDirCommand>();

    for (uint32_t i = 0; i < keys.count; ++i)
    {
        String key = keys.data[i];
        String value = values.data[i];

        if (key.equals(L"path"))
        {
            cmd->dirPath = value;
            break;
        }
        else
        {
            // @TODO: print: 'unknown key'
        }
    }

    assert(!cmd->dirPath.isEmpty());
    cmd->dirPath = String::clone(cmd->dirPath);

    return cmd;
}

bool OpenDirCommand::onExecute(Array<String>& args)
{
    String folder;

    if (dirPath.isEmpty())
    {
        if (args.count == 0)
            return false;

        folder = args.data[0];
    }
    else
        folder = this->dirPath;

    if (folder.isEmpty())
        return false;

    wchar_t* data = (wchar_t*)g_standardAllocator.alloc((folder.count + 1) * sizeof(wchar_t));
    if (data == nullptr)
        return false;

    wmemcpy(data, folder.data, folder.count);
    data[folder.count] = L'\0';

    PIDLIST_ABSOLUTE itemID = ILCreateFromPathW(data);
    assert(itemID);

    HRESULT result = SHOpenFolderAndSelectItems(itemID, 1, (LPCITEMIDLIST*)&itemID, 0);
    assert(SUCCEEDED(result));

    ILFree(itemID);

    g_standardAllocator.dealloc(data);

    return true;
}

void registerBasicCommands(CommandLoader* loader)
{
    assert(loader != nullptr);

    Array<CommandInfo*>& cmds = loader->commandInfoArray;

    static CommandInfo bc[] = {
        CommandInfo(CB_STRING_LITERAL(L"open_dir"), CI_None, openDir_createCommand),
        CommandInfo(CB_STRING_LITERAL(L"run_app"), CI_None, runApp_createCommand)
    };

    for (int i = 0; i < ARRAYSIZE(bc); ++i)
        cmds.add(&bc[i]);
}

// Parse 'nCmdShow' / 'nShow' windows thing (see MSDN ShowWindow function)
bool parseShowType(const String& value, int* showType);
Command * runApp_createCommand(Array<String>& keys, Array<String>& values)
{
    RunAppCommand* cmd = stdNew<RunAppCommand>();
    if (cmd == nullptr) return nullptr;

    for (uint32_t i = 0; i < keys.count; ++i)
    {
        String key = keys.data[i];
        String value = values.data[i];

        if (key.equals(L"path"))
        {
            cmd->appPath = value;
        }
        else if (key.equals(L"shell_exec"))
        {
            bool ok = ParseUtils::stringToBool(value, &cmd->shellExec);
            assert(ok);
        }
        else if (key.equals(L"run_as_admin"))
        {
            bool ok = ParseUtils::stringToBool(value, &cmd->asAdmin);
            assert(ok);
        }
        else if (key.equals(L"args"))
        {
            cmd->appArgs = value;
        }
        else if (key.equals(L"show_type"))
        {
            bool ok = parseShowType(value, &cmd->shellExec_nShow);
            assert(ok);
        }
        else if (key.equals(L"work_dir"))
        {
            cmd->workDir = value;
        }
    }

    assert(!cmd->appPath.isEmpty());
    if (cmd->asAdmin == true)
        assert(cmd->shellExec);

    // Reallocate strings, because right now they are references to data that may be deleted or modified.
    cmd->appPath = String::clone(cmd->appPath.trimmed());
    cmd->appArgs = String::clone(cmd->appArgs.trimmed());
    cmd->workDir = String::clone(cmd->workDir.trimmed());

    return cmd;
}

static bool runProcess(const wchar_t* path, wchar_t* commandLine);
static bool shellExecute(const wchar_t* path, const wchar_t* verb, const wchar_t* params, const wchar_t* workDir, int nShow);
bool RunAppCommand::onExecute(Array<String>& args)
{
    // @TODO: test 'runProcess' with args.
    // @TODO: make workDir work with 'runProcess'.

    wchar_t* workDir = this->workDir.data;
    wchar_t* execAppParamsStr = nullptr;
    bool shouldDeallocateAppParams = false;

    if (args.count == 0 && appArgs.isEmpty())
    {
        // We don't have any params to pass.
        execAppParamsStr = nullptr;
    }
    else if (args.count == 0 && !appArgs.isEmpty())
    {
        execAppParamsStr = appArgs.data;
    }
    else
    {
        int dataSize = 1; // Null-terminator.
        if (!appArgs.isEmpty())
            dataSize += appArgs.count + 1; // Include one space character for user-defined app args.
        for (uint32_t i = 0; i < args.count; ++i)
            dataSize += args.data[i].count;
        dataSize += args.count * 3; // Include one space character and two brackets for each arg.
        dataSize *= sizeof(wchar_t);

        assert(dataSize >= 1);
        
        execAppParamsStr = static_cast<wchar_t*>(g_standardAllocator.alloc(dataSize));
        assert(execAppParamsStr);
        shouldDeallocateAppParams = true;

        int i = 0;
        if (!appArgs.isEmpty())
        {
            wmemcpy(&execAppParamsStr[i], appArgs.data, appArgs.count);
            i += appArgs.count;
            execAppParamsStr[i] = L' ';
            i += 1;
        }

        for (uint32_t argIndex = 0; argIndex < args.count; ++argIndex)
        {
            const String& arg = args.data[argIndex];

            execAppParamsStr[i] = L'"';
            i += 1;

            wmemcpy(&execAppParamsStr[i], arg.data, arg.count);
            i += arg.count;

            execAppParamsStr[i] = L'"';
            i += 1;
            execAppParamsStr[i] = L' ';
            i += 1;
        }

        execAppParamsStr[i] = L'\0';
        i += 1;

        assert(i * sizeof(wchar_t) == dataSize);
    }

    bool result = false;
    if (shellExec)
    {
        result = shellExecute(appPath.data, asAdmin ? L"runas" : nullptr, execAppParamsStr, workDir, this->shellExec_nShow);
    }
    else
    {
        assert(workDir == nullptr);
        result = runProcess(appPath.data, execAppParamsStr);
    }

    if (shouldDeallocateAppParams)
        g_standardAllocator.dealloc(execAppParamsStr);

    return result;
}

bool parseShowType(const String& value, int* showType)
{
    assert(showType != nullptr);
    
    StringComparison cmp = StringComparison::CaseInsensitive;

#define SHOWTYPECHECK(str, x) if (value.equals(str, cmp)) { *showType = x; return true; }
    SHOWTYPECHECK(L"normal", SW_SHOWNORMAL);
    SHOWTYPECHECK(L"minimized", SW_SHOWMINIMIZED);
    SHOWTYPECHECK(L"maximized", SW_SHOWMAXIMIZED);
#undef SHOWTYPECHECK

    return false;
}

static bool runProcess(const wchar_t* path, wchar_t* commandLine)
{
    assert(path);

    STARTUPINFOW startupInfo = { sizeof(startupInfo), 0 };
    PROCESS_INFORMATION processInfo = { 0 };

    bool success = 0 != CreateProcessW(
        path,
        commandLine,
        nullptr,
        nullptr,
        true,
        NORMAL_PRIORITY_CLASS,
        nullptr,
        nullptr, //currentDirectory.data,
        &startupInfo,
        &processInfo);

    if (success)
    {
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
    }

    return success;
}

static bool shellExecute(const wchar_t* path, const wchar_t* verb, const wchar_t* params, const wchar_t* workDir, int nShow)
{
    assert(path);

    SHELLEXECUTEINFOW info = { 0 };
    info.cbSize = sizeof(info);
    info.lpVerb = verb;
    info.lpFile = path;
    info.hwnd = 0;
    info.lpParameters = params;
    info.nShow = nShow;
    info.lpDirectory = workDir;

    bool success = 0 != ShellExecuteExW(&info);
    if (!success)
    {
        DWORD error = GetLastError();
        if (error == ERROR_CANCELLED)
            success = true;
    }

    return success;
}

//    assert(appPath);
//    assert(&args);
//
//    //String currentDirectory = OSUtils::getDirectoryFromFileName(String((wchar_t*)appPath));
//    //String commandLine = OSUtils::buildCommandLine(args.data, args.count);
//
//    int paramsCount = 0;
//    for (int i = 1; i < args.count; ++i)
//        paramsCount += args.data[i]->count + 1;
//
//    wchar_t* params = paramsCount > 0 ? (wchar_t*)g_standardAllocator.alloc(paramsCount * sizeof(wchar_t)) : nullptr;
//    if (params != nullptr)
//    {
//        wchar_t* paramsCurrent = params;
//        for (int i = 0; i < args.count; ++i)
//        {
//            wmemcpy(paramsCurrent, args.data[i]->data, args.data[i]->count);
//            paramsCurrent += args.data[i]->count;
//            *paramsCurrent++ = L' ';
//        }
//        params[paramsCount] = L'\0';
//    }
//
//    String fileName = clone(*args.data[0]);
//    if (fileName.isEmpty())
//        __debugbreak();
//
//    SHELLEXECUTEINFOW info = { 0 };
//    info.cbSize = sizeof(info);
//    //info.lpVerb = L"runas";
//    info.lpFile = fileName.data;//s args.data[0]->data;
//    info.hwnd = 0;
//    info.lpParameters = params;
//    info.nShow = SW_NORMAL;
//
//    bool success = 0 != ShellExecuteExW(&info);
//    g_standardAllocator.dealloc(params);
//
//    //STARTUPINFOW startupInfo = { sizeof(startupInfo), 0 };
//    //PROCESS_INFORMATION processInfo = { 0 };
//
//    //bool success = 0 != CreateProcessW(
//    //    appPath,//fileName->data,
//    //    commandLine.data,//nullptr, //commandLine.data,
//    //    nullptr,
//    //    nullptr,
//    //    true,
//    //    NORMAL_PRIORITY_CLASS,
//    //    nullptr,
//    //    nullptr, //currentDirectory.data,
//    //    &startupInfo,
//    //    &processInfo);
//
//    DWORD lastError = 0;
//    if (success)
//    {
//    }
//    else
//    {
//        lastError = GetLastError();
//        LPWSTR buffer = nullptr;
//        DWORD msg = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, lastError, 0, (LPWSTR)&buffer, 0, 0);
//
//        if (msg == 0)
//            MessageBoxW(0, L"Unable to execute application.", L"Error", MB_OK | MB_ICONERROR);
//        else
//            MessageBoxW(0, buffer, L"Error", MB_OK | MB_ICONERROR);
//
//        if (buffer != nullptr)
//            LocalFree(buffer);
//    }

Command* quit_createCommand(Array<String>& keys, Array<String>& values)
{
    return stdNew<QuitCommand>();
}

bool QuitCommand::onExecute(Array<String>& args)
{
    commandWindow->exit();
    return true;
}
