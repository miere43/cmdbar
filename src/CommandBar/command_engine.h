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

/**
 * Represents shared data between several Command instances.
 */
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

    virtual ~Command();

    virtual bool Execute(ExecuteCommandState* state, Array<Newstring>& args) = 0;
};

struct BaseCommandState 
{
    /**
     * Use temporary allocator for error message, otherwise it wouldn't be deallocated!
     * This message can be zero-terminated.
     */
    Newstring errorMessage;

    /**
     * Formats an error message using specified format string. Result is stored in errorMessage variable.
     */
    void FormatErrorMessage(const wchar_t* format, ...);
};

struct CreateCommandState : public BaseCommandState
{
};

struct ExecuteCommandState : public BaseCommandState
{
    /** Command that was parsed from expression string. */
    Command* command = nullptr;
};

struct CommandEngine
{
    ExecuteCommandState executionState;

    /**
     * Command window associated with this command engine.
     */
    CommandWindow* window = nullptr;
    CommandBeforeRunCallback beforeRunCallback = nullptr;
    void* beforeRunCallbackUserdata = nullptr;
    Array<Command*> commands;

    /**
     * Evaluates expression, calling command with args parsed from specified expression.
     * If evaluation failed, or command execution ended with an error, returns false. To get additional information, get execution state by calling GetExecutionState().
     */
    bool Evaluate(const Newstring& expression);

    /**
     * Returns execution state for last expression evaluation using Evaluate() method.
     */
    ExecuteCommandState* GetExecutionState();

	void SetBeforeRunCallback(CommandBeforeRunCallback callback, void* userdata);

    /**
     * Returns command by it's name.
     */
	Command* FindCommandByName(const Newstring& name);

    /**
     * Registers command within command engine so it can be evaluated.
     * If command cannot be registered, returns false, otherwise true.
     *
     * Specified command should be allocated using standard allocator. It will be deallocated by command engine.
     */
    bool RegisterCommand(Command* command);

    void UnregisterAllCommands();

    /**
     * Releases resources used by command engine.
     */
    void Dispose();
private:
    void ClearExecutionState();
};

