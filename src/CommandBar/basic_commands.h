#pragma once
#include <ShlObj.h>
#include <assert.h>

#include "os_utils.h"
#include "command_engine.h"


struct CommandLoader;
void registerBasicCommands(CommandLoader* loader);
Command* quit_createCommand(Array<Newstring>& keys, Array<Newstring>& values);


struct CommandWindow;
struct QuitCommand : public Command
{
    CommandWindow* commandWindow = nullptr;

    virtual bool onExecute(Array<Newstring>& args) override;
};

struct OpenDirCommand : public Command
{
    Newstring dirPath;

    virtual bool onExecute(Array<Newstring>& args) override;
};

struct RunAppCommand : public Command
{
    Newstring appPath;
    Newstring appArgs;
    Newstring workDir;

    bool shellExec = false;
    bool asAdmin = false;

    int shellExec_nShow = SW_NORMAL;

    virtual bool onExecute(Array<Newstring>& args) override;
};
