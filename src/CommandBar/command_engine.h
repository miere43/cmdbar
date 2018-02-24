#pragma once
#include "array.h"
#include "newstring.h"

struct Command;
struct CommandWindow;
struct CommandEngine;
typedef void(*CommandCallback)(Command& command, const Newstring* args, uint32_t numArgs);
typedef void(*CommandBeforeRunCallback)(CommandEngine* engine, void* userdata);


enum CommandInfoFlags
{
    CI_None = 0,
    //CI_NoArgSplit = 1,
    //CI_IncludeCommandNameToArgs = 2,
};


typedef Command*(*CommandInfo_CreateCommand)(Array<Newstring>& keys, Array<Newstring>& values);
struct CommandInfo
{
    Newstring dataName;
    CommandInfoFlags flags = CI_None;

    CommandInfo_CreateCommand createCommand = 0;

	CommandInfo();
	CommandInfo(Newstring dataName, CommandInfoFlags flags, CommandInfo_CreateCommand command);
};

struct Command
{
    CommandInfo* info = nullptr;
    CommandEngine* engine = nullptr;
    Newstring name;

    virtual bool onExecute(Array<Newstring>& args) = 0;
};

struct CommandEngine
{
    CommandWindow* window = nullptr;
    
    bool Evaluate(const Newstring& expression);

	void SetBeforeRunCallback(CommandBeforeRunCallback callback, void* userdata);
	Command* FindCommandByName(const Newstring& name);

	CommandBeforeRunCallback beforeRunCallback = nullptr;
	void* beforeRunCallbackUserdata = nullptr;

    Array<CommandInfo*> knownCommandInfoArray;
    Array<Command*> commands;
    bool RegisterCommand(Command* command);
    bool RegisterCommandInfo(CommandInfo* info);
};