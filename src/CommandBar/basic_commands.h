#pragma once
#include <ShlObj.h>
#include <assert.h>

#include "os_utils.h"
#include "command_engine.h"


struct CommandLoader;
void RegisterBuiltinCommands(CommandLoader* loader);


struct CommandWindow;
struct QuitCommand : public Command
{
    CommandWindow* commandWindow = nullptr;

    virtual ~QuitCommand() override;

    virtual bool Execute(ExecuteCommandState* state, Array<Newstring>& args) override;
};

struct OpenDirCommand : public Command
{
    Newstring dirPath;

    virtual ~OpenDirCommand() override;

    virtual bool Execute(ExecuteCommandState* state, Array<Newstring>& args) override;
};

struct RunAppCommand : public Command
{
    wchar_t* appPath;
    wchar_t* appArgs;
    wchar_t* workDir;

    bool shellExec = false;
    bool asAdmin = false;

    int shellExec_nShow = SW_NORMAL;

    virtual ~RunAppCommand() override;

    virtual bool Execute(ExecuteCommandState* state, Array<Newstring>& args) override;
};
