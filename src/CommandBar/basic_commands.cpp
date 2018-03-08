#include <assert.h>

#include "basic_commands.h"
#include "command_loader.h"
#include "command_window.h"
#include "parse_utils.h"
#include "defer.h"


Command* runApp_createCommand(CreateCommandState* state, Array<Newstring>& keys, Array<Newstring>& values);
Command* openDir_createCommand(CreateCommandState* state, Array<Newstring>& keys, Array<Newstring>& values);
Command* quit_createCommand(CreateCommandState* state, Array<Newstring>& keys, Array<Newstring>& values);


Command* openDir_createCommand(CreateCommandState* state, Array<Newstring>& keys, Array<Newstring>& values)
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
    cmd->dirPath = cmd->dirPath.CloneAsCString();

    return cmd;
}

bool OpenDirCommand::Execute(ExecuteCommandState* state, Array<Newstring>& args)
{
    Newstring folder;
    if (Newstring::IsNullOrEmpty(dirPath))
    {
        if (args.count == 0)
            return false;

        folder = args.data[0];
    }
    else
    {
        folder = this->dirPath;
    }

    if (Newstring::IsNullOrEmpty(folder))
        return false;

    Newstring actualFolder = folder;
    if (!actualFolder.IsZeroTerminated())
    {
        actualFolder = folder.CloneAsCString(&g_tempAllocator);
        assert(!Newstring::IsNullOrEmpty(actualFolder));
    }

    if (!OSUtils::DirectoryExists(actualFolder))
    {
        actualFolder.RemoveZeroTermination();
        state->FormatErrorMessage(L"Folder \"%.*s\" does not exist.", actualFolder.count, actualFolder.data);

        return false;
    }

    PIDLIST_ABSOLUTE itemID = ::ILCreateFromPathW(actualFolder.data);
    if (itemID == nullptr)
    {
        Newstring osError = OSUtils::FormatErrorCode(GetLastError(), 0, &g_tempAllocator);
        state->FormatErrorMessage(L"Cannot get directory identifier: %.*s", osError.count, osError.data);

        return false;
    }
    defer(ILFree(itemID));

    HRESULT result = ::SHOpenFolderAndSelectItems(itemID, 1, (LPCITEMIDLIST*)&itemID, 0);
    if (FAILED(result))
    {
        state->FormatErrorMessage(L"Cannot select directory, error code was 0x%08X.", result);

        return false;
    }

    return true;
}

void RegisterBasicCommands(CommandLoader* loader)
{
    assert(loader != nullptr);

    Array<CommandInfo*>& cmds = loader->commandInfoArray;

    static CommandInfo bc[] = {
        CommandInfo(Newstring::WrapConstWChar(L"open_dir"), CI_None, openDir_createCommand),
        CommandInfo(Newstring::WrapConstWChar(L"run_app"), CI_None, runApp_createCommand)
    };

    for (int i = 0; i < ARRAYSIZE(bc); ++i)
        cmds.Append(&bc[i]);
}

// Parse 'nCmdShow' / 'nShow' windows thing (see MSDN ShowWindow function)
bool parseShowType(const Newstring& value, int* showType);
Command * runApp_createCommand(CreateCommandState* state, Array<Newstring>& keys, Array<Newstring>& values)
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
    cmd->appPath = cmd->appPath.Trimmed().CloneAsCString();
    cmd->appPath.count -= 1;
    cmd->appArgs = cmd->appArgs.Trimmed().CloneAsCString();
    cmd->appArgs.count -= 1;
    cmd->workDir = cmd->workDir.Trimmed().CloneAsCString();
    cmd->workDir.count -= 1;

    return cmd;
}

static bool runProcess(const wchar_t* path, wchar_t* commandLine);
static bool shellExecute(const wchar_t* path, const wchar_t* verb, const wchar_t* params, const wchar_t* workDir, int nShow);
bool RunAppCommand::Execute(ExecuteCommandState* state, Array<Newstring>& args)
{
    // @TODO: Test 'runProcess' with args.
    // @TODO: Make workDir work with 'runProcess'.

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
        // @TODO: Rewrite with NewstringBuilder.

        int dataSize = 1; // Terminating zero.
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
        result = shellExecute(appPath.data, asAdmin ? L"runas" : nullptr, execAppParamsStr, workDir, this->shellExec_nShow);
    }
    else
    {
        assert(workDir == nullptr);

        result = runProcess(appPath.data, execAppParamsStr);
    }

    if (!result)
    {
        Newstring osError = OSUtils::FormatErrorCode(GetLastError(), 0, &g_tempAllocator);
        state->FormatErrorMessage(L"Unable to run application: %.*s", osError.count, osError.data);
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
    
#define SHOWTYPECHECK(str, x) if (value.Equals(str, StringComparison::CaseInsensitive)) { *showType = x; return true; }
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

    SetLastError(0);
    bool success = 0 != CreateProcessW(
        path,
        commandLine,
        nullptr,
        nullptr,
        true,
        NORMAL_PRIORITY_CLASS,
        nullptr,
        nullptr,
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

    SetLastError(0);
    return !!ShellExecuteExW(&info);
}

Command* quit_createCommand(CreateCommandState* state, Array<Newstring>& keys, Array<Newstring>& values)
{
    return Memnew(QuitCommand);
}

bool QuitCommand::Execute(ExecuteCommandState* state, Array<Newstring>& args)
{
    commandWindow->Exit();
    return true;
}
