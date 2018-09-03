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
    cmd->dirPath = cmd->dirPath.Clone();

    return cmd;
}

OpenDirCommand::~OpenDirCommand()
{
    dirPath.Dispose();
}

bool OpenDirCommand::Execute(ExecuteCommandState* state, Array<Newstring>& args)
{
    Newstring folder;
    Newstring subfolder;

    if (Newstring::IsNullOrEmpty(dirPath))
    {
        if (args.count == 0)
            return false;

        folder = args.data[0];
    }
    else
    {
        folder = this->dirPath;
        if (args.count > 0)
        {
            subfolder = args.data[0];
        }
    }

    if (Newstring::IsNullOrEmpty(folder))
        return false;

    wchar_t* actualFolder = nullptr;

    if (!Newstring::IsNullOrEmpty(subfolder))
    {
        wchar_t* result = (wchar_t*)g_tempAllocator.Allocate(sizeof(wchar_t) * (folder.count + 1 + subfolder.count + 1));
        uint32_t now = 0;

        wmemcpy(&result[now], folder.data, folder.count);
        now += folder.count;

        if (result[now - 1] != L'\\')
        {
            result[now] = L'\\';
            now += 1;
        }

        wmemcpy(&result[now], subfolder.data, subfolder.count);
        now += subfolder.count;

        result[now++] = L'\0';

        actualFolder = result;
    }
    else
    {
        actualFolder = folder.CloneAsCString(&g_tempAllocator);
    }

    if (!OSUtils::DirectoryExists(actualFolder))
    {
        state->FormatErrorMessage(L"Folder \"%s\" does not exist.", actualFolder);

        return false;
    }

    PIDLIST_ABSOLUTE itemID = ::ILCreateFromPathW(actualFolder);
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

void RegisterBuiltinCommands(CommandLoader* loader)
{
    assert(loader != nullptr);

    Array<CommandInfo*>& cmds = loader->commandInfoArray;

    static CommandInfo bc[] = {
        CommandInfo(Newstring::WrapConstWChar(L"open_dir"), CI_None, openDir_createCommand),
        CommandInfo(Newstring::WrapConstWChar(L"run_app"),  CI_None, runApp_createCommand)
    };

    for (int i = 0; i < ARRAYSIZE(bc); ++i)
        cmds.Append(&bc[i]);
}

// Parse 'nCmdShow' / 'nShow' windows thing (see MSDN ShowWindow function)
bool parseShowType(const Newstring& value, int* showType);
Command* runApp_createCommand(CreateCommandState* state, Array<Newstring>& keys, Array<Newstring>& values)
{
    RunAppCommand* cmd = Memnew(RunAppCommand);
    if (cmd == nullptr) return nullptr;

    Newstring argAppPath;
    Newstring argAppArgs;
    Newstring argWorkDir;

    for (uint32_t i = 0; i < keys.count; ++i)
    {
        Newstring key = keys.data[i];
        Newstring value = values.data[i];

        if (key == L"path")
        {
            argAppPath = value;
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
            argAppArgs = value;
        }
        else if (key == L"show_type")
        {
            bool ok = parseShowType(value, &cmd->shellExec_nShow);
            assert(ok);
        }
        else if (key == L"work_dir")
        {
            argWorkDir = value;
        }
    }

    assert(!Newstring::IsNullOrEmpty(argAppPath));
    if (cmd->asAdmin == true)
        assert(cmd->shellExec);

    // Reallocate strings, because right now they are references to data that may be deleted or modified.
    cmd->appPath = argAppPath.Trimmed().CloneAsCString();
    cmd->appArgs = argAppArgs.Trimmed().CloneAsCString();
    cmd->workDir = argWorkDir.Trimmed().CloneAsCString();

    return cmd;
}

static bool runProcess(const wchar_t* path, wchar_t* commandLine);
static bool shellExecute(const wchar_t* path, const wchar_t* verb, const wchar_t* params, const wchar_t* workDir, int nShow);

RunAppCommand::~RunAppCommand()
{
    auto a = &g_standardAllocator;
    a->Deallocate(appPath);
    a->Deallocate(appArgs);
    a->Deallocate(workDir);
}

bool RunAppCommand::Execute(ExecuteCommandState* state, Array<Newstring>& args)
{
    // @TODO: Test 'runProcess' with args.
    // @TODO: Make workDir work with 'runProcess'.

    wchar_t* workDir = this->workDir;
    wchar_t* execAppParamsStr = nullptr;
    bool shouldDeallocateAppParams = false;

    if (args.count == 0 && (appArgs == nullptr || wcslen(appArgs) == 0))
    {
        // We don't have any params to pass.
        execAppParamsStr = nullptr;
    }
    else if (args.count == 0 && (appArgs != nullptr && wcslen(appArgs) != 0))
    {
        execAppParamsStr = appArgs;
    }
    else
    {
        // @TODO: Rewrite with NewstringBuilder.

        int dataSize = 1; // Terminating zero.
        if (appArgs != nullptr && wcslen(appArgs) != 0)
            dataSize += wcslen(appArgs) + 1; // Include one space character for user-defined app args.
        for (uint32_t i = 0; i < args.count; ++i)
            dataSize += args.data[i].count;
        dataSize += args.count * 3; // Include one space character and two brackets for each arg.
        dataSize *= sizeof(wchar_t);

        assert(dataSize >= 1);
        
        execAppParamsStr = static_cast<wchar_t*>(g_standardAllocator.Allocate(dataSize));
        assert(execAppParamsStr);
        shouldDeallocateAppParams = true;

        int i = 0;
        if (appArgs != nullptr && wcslen(appArgs) != 0)
        {
            wmemcpy(&execAppParamsStr[i], appArgs, wcslen(appArgs));
            i += wcslen(appArgs);
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
        result = shellExecute(appPath, asAdmin ? L"runas" : nullptr, execAppParamsStr, workDir, this->shellExec_nShow);
    }
    else
    {
        assert(workDir == nullptr);

        result = runProcess(appPath, execAppParamsStr);
    }

    DWORD error = GetLastError();
    if (error == ERROR_CANCELLED)
        result = true;

    if (!result)
    {
        Newstring osError = OSUtils::FormatErrorCode(error, 0, &g_tempAllocator);
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

QuitCommand::~QuitCommand()
{
    
}

bool QuitCommand::Execute(ExecuteCommandState* state, Array<Newstring>& args)
{
    commandWindow->Exit();
    return true;
}
