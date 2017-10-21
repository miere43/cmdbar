#pragma once
#include <ShlObj.h>
#include <assert.h>

#include "os_utils.h"
#include "command_engine.h"


struct CommandLoader;
void registerBasicCommands(CommandLoader* loader);
Command* quit_createCommand(Array<String>& keys, Array<String>& values);


struct CommandWindow;
struct QuitCommand : public Command
{
    CommandWindow* commandWindow = nullptr;

    virtual bool onExecute(Array<String>& args) override;
};

struct OpenDirCommand : public Command
{
    String dirPath;

    virtual bool onExecute(Array<String>& args) override;
};

struct RunAppCommand : public Command
{
    String appPath;
    String appArgs;
    String workDir;

    bool shellExec = false;
    bool asAdmin = false;

    int shellExec_nShow = SW_NORMAL;

    virtual bool onExecute(Array<String>& args) override;
};
