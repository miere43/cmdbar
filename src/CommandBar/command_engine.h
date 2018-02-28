#pragma once
#include "array.h"
#include "newstring.h"


struct Command;
struct CommandWindow;
struct CommandEngine;
struct BaseCommandState;
struct CreateCommandState;
struct ExecuteCommandState;

typedef void(*CommandCallback)(Command& command, const Newstring* args, uint32_t numArgs);
typedef void(*CommandBeforeRunCallback)(CommandEngine* engine, void* userdata);
typedef Command*(*CommandInfo_CreateCommand)(CreateCommandState* state, Array<Newstring>& keys, Array<Newstring>& values);

enum CommandInfoFlags
{
    CI_None = 0,
    //CI_NoArgSplit = 1,
    //CI_IncludeCommandNameToArgs = 2,
};

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

    virtual bool onExecute(ExecuteCommandState* state, Array<Newstring>& args) = 0;
};

struct BaseCommandState 
{
    Newstring errorMessage;
    IAllocator* errorMessageAllocator;

    void SetErrorMessage(Newstring message, IAllocator* allocator);
};

struct CreateCommandState : public BaseCommandState
{
};

struct ExecuteCommandState : public BaseCommandState
{
};

struct CommandEngine
{
    ExecuteCommandState executionState;

    CommandWindow* window = nullptr;
    
    bool Evaluate(const Newstring& expression);
    ExecuteCommandState* GetExecutionState();

	void SetBeforeRunCallback(CommandBeforeRunCallback callback, void* userdata);
	Command* FindCommandByName(const Newstring& name);

	CommandBeforeRunCallback beforeRunCallback = nullptr;
	void* beforeRunCallbackUserdata = nullptr;

    Array<CommandInfo*> knownCommandInfoArray;
    Array<Command*> commands;
    bool RegisterCommand(Command* command);
    bool RegisterCommandInfo(CommandInfo* info);

    void Dispose();

private:
    void ClearExecutionState();
};

