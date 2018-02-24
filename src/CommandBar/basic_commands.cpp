#include <assert.h>

#include "basic_commands.h"
#include "command_loader.h"
#include "command_window.h"
#include "parse_utils.h"


Command* runApp_createCommand(Array<Newstring>& keys, Array<Newstring>& values);
Command* openDir_createCommand(Array<Newstring>& keys, Array<Newstring>& values);
Command* quit_createCommand(Array<Newstring>& keys, Array<Newstring>& values);


Command* openDir_createCommand(Array<Newstring>& keys, Array<Newstring>& values)
{
    OpenDirCommand* cmd = Memnew(OpenDirCommand);

    for (uint32_t i = 0; i < keys.count; ++i)
    {
        Newstring key = keys.data[i];
        Newstring value = values.data[i];

        if (key == L"path")
        {
            cmd->dirPath = value;
            break;
        }
        else
        {
            // @TODO: print: 'unknown key'
        }
    }

    assert(!Newstring::IsNullOrEmpty(cmd->dirPath));
    cmd->dirPath = cmd->dirPath.Clone();

    return cmd;
}

bool OpenDirCommand::onExecute(Array<Newstring>& args)
{
    Newstring folder;

    if (Newstring::IsNullOrEmpty(dirPath))
    {
        if (args.count == 0)
            return false;

        folder = args.data[0];
    }
    else
        folder = this->dirPath;

    if (Newstring::IsNullOrEmpty(folder))
        return false;

    wchar_t* data = (wchar_t*)g_standardAllocator.Allocate((folder.count + 1) * sizeof(wchar_t));
    if (data == nullptr)
        return false;

    wmemcpy(data, folder.data, folder.count);
    data[folder.count] = L'\0';

    PIDLIST_ABSOLUTE itemID = ILCreateFromPathW(data);
    assert(itemID);

    HRESULT result = SHOpenFolderAndSelectItems(itemID, 1, (LPCITEMIDLIST*)&itemID, 0);
    assert(SUCCEEDED(result));

    ILFree(itemID);

    g_standardAllocator.Deallocate(data);

    return true;
}

void registerBasicCommands(CommandLoader* loader)
{
    assert(loader != nullptr);

    Array<CommandInfo*>& cmds = loader->commandInfoArray;

    static CommandInfo bc[] = {
        CommandInfo(Newstring::WrapConstWChar(L"open_dir"), CI_None, openDir_createCommand),
        CommandInfo(Newstring::WrapConstWChar(L"run_app"), CI_None, runApp_createCommand)
    };

    for (int i = 0; i < ARRAYSIZE(bc); ++i)
        cmds.add(&bc[i]);
}

// Parse 'nCmdShow' / 'nShow' windows thing (see MSDN ShowWindow function)
bool parseShowType(const Newstring& value, int* showType);
Command * runApp_createCommand(Array<Newstring>& keys, Array<Newstring>& values)
{
    RunAppCommand* cmd = Memnew(RunAppCommand);
    if (cmd == nullptr) return nullptr;

    for (uint32_t i = 0; i < keys.count; ++i)
    {
        Newstring key = keys.data[i];
        Newstring value = values.data[i];

        if (key == L"path")
        {
            cmd->appPath = value;
        }
        else if (key == L"shell_exec")
        {
            bool ok = ParseUtils::StringToBool(value, &cmd->shellExec);
            assert(ok);
        }
        else if (key == L"run_as_admin")
        {
            bool ok = ParseUtils::StringToBool(value, &cmd->asAdmin);
            assert(ok);
        }
        else if (key == L"args")
        {
            cmd->appArgs = value;
        }
        else if (key == L"show_type")
        {
            bool ok = parseShowType(value, &cmd->shellExec_nShow);
            assert(ok);
        }
        else if (key == L"work_dir")
        {
            cmd->workDir = value;
        }
    }

    assert(!Newstring::IsNullOrEmpty(cmd->appPath));
    if (cmd->asAdmin == true)
        assert(cmd->shellExec);

    // Reallocate strings, because right now they are references to data that may be deleted or modified.
    cmd->appPath = cmd->appPath.Trimmed().Clone();
    cmd->appArgs = cmd->appArgs.Trimmed().Clone();
    cmd->workDir = cmd->workDir.Trimmed().Clone();

    return cmd;
}

static bool runProcess(const wchar_t* path, wchar_t* commandLine);
static bool shellExecute(const wchar_t* path, const wchar_t* verb, const wchar_t* params, const wchar_t* workDir, int nShow);
bool RunAppCommand::onExecute(Array<Newstring>& args)
{
    // @TODO: test 'runProcess' with args.
    // @TODO: make workDir work with 'runProcess'.

    wchar_t* workDir = this->workDir.data;
    wchar_t* execAppParamsStr = nullptr;
    bool shouldDeallocateAppParams = false;

    if (args.count == 0 && Newstring::IsNullOrEmpty(appArgs))
    {
        // We don't have any params to pass.
        execAppParamsStr = nullptr;
    }
    else if (args.count == 0 && !Newstring::IsNullOrEmpty(appArgs))
    {
        execAppParamsStr = appArgs.data;
    }
    else
    {
        int dataSize = 1; // Null-terminator.
        if (!Newstring::IsNullOrEmpty(appArgs))
            dataSize += appArgs.count + 1; // Include one space character for user-defined app args.
        for (uint32_t i = 0; i < args.count; ++i)
            dataSize += args.data[i].count;
        dataSize += args.count * 3; // Include one space character and two brackets for each arg.
        dataSize *= sizeof(wchar_t);

        assert(dataSize >= 1);
        
        execAppParamsStr = static_cast<wchar_t*>(g_standardAllocator.Allocate(dataSize));
        assert(execAppParamsStr);
        shouldDeallocateAppParams = true;

        int i = 0;
        if (!Newstring::IsNullOrEmpty(appArgs))
        {
            wmemcpy(&execAppParamsStr[i], appArgs.data, appArgs.count);
            i += appArgs.count;
            execAppParamsStr[i] = L' ';
            i += 1;
        }

        for (uint32_t argIndex = 0; argIndex < args.count; ++argIndex)
        {
            const Newstring& arg = args.data[argIndex];

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
        Newstring actualAppPath = appPath;
        if (!actualAppPath.IsZeroTerminated())
        {
            actualAppPath = appPath.CloneAsCString(&g_tempAllocator);
            assert(!Newstring::IsNullOrEmpty(actualAppPath));
        }

        result = shellExecute(actualAppPath.data, asAdmin ? L"runas" : nullptr, execAppParamsStr, workDir, this->shellExec_nShow);
    }
    else
    {
        assert(workDir == nullptr);

        Newstring actualAppPath = appPath;
        if (!actualAppPath.IsZeroTerminated())
        {
            actualAppPath = appPath.CloneAsCString(&g_tempAllocator);
            assert(!Newstring::IsNullOrEmpty(actualAppPath));
        }

        result = runProcess(actualAppPath.data, execAppParamsStr);
    }

    if (shouldDeallocateAppParams)
    {
        g_standardAllocator.Deallocate(execAppParamsStr);
    }

    return result;
}

bool parseShowType(const Newstring& value, int* showType)
{
    assert(showType != nullptr);
    
    //NewstringComparison cmp = NewstringComparison::CaseInsensitive;

    // @TODO: Do case-insensitive comparison.

#define SHOWTYPECHECK(str, x) if (value == str) { *showType = x; return true; }
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

Command* quit_createCommand(Array<Newstring>& keys, Array<Newstring>& values)
{
    return Memnew(QuitCommand);
}

bool QuitCommand::onExecute(Array<Newstring>& args)
{
    commandWindow->Exit();
    return true;
}
