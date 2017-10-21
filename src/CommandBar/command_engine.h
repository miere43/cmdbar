#pragma once
#include "array.h"
#include "string_type.h"

struct Command;
struct CommandWindow;
struct CommandEngine;
typedef void(*CommandCallback)(Command& command, const String* args, uint32_t numArgs);
typedef void(*CommandBeforeRunCallback)(CommandEngine* engine, void* userdata);


enum CommandInfoFlags
{
    CI_None = 0,
    //CI_NoArgSplit = 1,
    //CI_IncludeCommandNameToArgs = 2,
};


typedef Command*(*CommandInfo_CreateCommand)(Array<String>& keys, Array<String>& values);
struct CommandInfo
{
    String dataName;
    CommandInfoFlags flags = CI_None;

    CommandInfo_CreateCommand createCommand = 0;
};

struct Command
{
    CommandInfo* info = nullptr;
    CommandEngine* engine = nullptr;
    String name;

    virtual bool onExecute(Array<String>& args) = 0;
};

struct CommandEngine
{
    CommandWindow* window = nullptr;
    
    bool evaluate(const String& expression);

	void setBeforeRunCallback(CommandBeforeRunCallback callback, void* userdata);
	Command* findCommandByName(const String& name);

	CommandBeforeRunCallback beforeRunCallback = nullptr;
	void* beforeRunCallbackUserdata = nullptr;

    Array<CommandInfo*> knownCommandInfoArray;
    Array<Command*> commands;
    bool registerCommand(Command* command);
    bool registerCommandInfo(CommandInfo* info);
};