#include <assert.h>

#include "command_engine.h"


bool CommandEngine::Evaluate(const Newstring& expression)
{
    ClearExecutionState();

    Array<Newstring> args;
    bool inQuotes = false;
    uint32_t argStart = 0;
    uint32_t argLength = 0;

    for (uint32_t i = 0; i < expression.count; ++i)
    {
        wchar_t c = expression.data[i];
        if (c == '\"')
        {
            if (inQuotes)
            {
                inQuotes = false;
                args.add(Newstring(expression.data + argStart, argLength));

                argStart = i + 1;
                argLength = 0;
            }
            else
            {
                assert(argLength == 0);
                inQuotes = true;
                argStart = i + 1;
                argLength = 0;
            }

            continue;
        }
        else if (inQuotes)
        {
            ++argLength;
        }
        else if (c == L' ')
        {
            if (argLength > 0)
            {
                args.add(Newstring(expression.data + argStart, argLength));
                argStart = i + 1;
                argLength = 0;
            }
            else
            {
                continue;
            }
        }
        else
        {
            ++argLength;
        }
    }

    if (argLength > 0)
    {
        args.add(Newstring(expression.data + argStart, argLength));
    }

    if (args.count == 0)
    {
        executionState.SetErrorMessage(Newstring::WrapConstWChar(L"Invalid input.").CloneAsCString(&g_tempAllocator), &g_tempAllocator);
        return false;
    }

    const Newstring& commandName = args.data[0];
    Command* command = FindCommandByName(commandName);

    if (command == nullptr)
    {
        executionState.SetErrorMessage(
            Newstring::FormatTempCString(L"Command \"%.*s\" is not found.", commandName.count, commandName.data),
            &g_tempAllocator);
        return false;
    }

    if (beforeRunCallback != nullptr)
    {
        beforeRunCallback(this, beforeRunCallbackUserdata);
    }

    Array<Newstring> actualArgs;
    for (uint32_t i = 1; i < args.count; ++i)
        actualArgs.add(args.data[i]);
    
    return command->onExecute(&executionState, actualArgs);
}

ExecuteCommandState* CommandEngine::GetExecutionState()
{
    return &executionState;
}

void CommandEngine::SetBeforeRunCallback(CommandBeforeRunCallback callback, void * userdata)
{
	this->beforeRunCallback = callback;
	this->beforeRunCallbackUserdata = userdata;
}

Command* CommandEngine::FindCommandByName(const Newstring& name)
{
	for (uint32_t i = 0; i < commands.count; ++i)
	{
		Command* command = commands.data[i];

		if (name == command->name)
			return command;
	}

	return nullptr;
}

bool CommandEngine::RegisterCommand(Command* command)
{
    assert(command);

    if (commands.add(command))
    {
        command->engine = this;
        return true;
    }

    return false;
}

bool CommandEngine::RegisterCommandInfo(CommandInfo* info)
{
    assert(info);

    return knownCommandInfoArray.add(info);
}

void CommandEngine::Dispose()
{
    ClearExecutionState();
}

void CommandEngine::ClearExecutionState()
{
    ExecuteCommandState& e = executionState;

    if (!Newstring::IsNullOrEmpty(e.errorMessage))
    {
        assert(e.errorMessageAllocator);
        e.errorMessage.Dispose(e.errorMessageAllocator);
    }

    e = ExecuteCommandState();
}

CommandInfo::CommandInfo()
{ }

CommandInfo::CommandInfo(Newstring dataName, CommandInfoFlags flags, CommandInfo_CreateCommand command)
	: dataName(dataName)
	, flags(flags)
	, createCommand(command)
{ }

void BaseCommandState::SetErrorMessage(Newstring message, IAllocator* allocator)
{
    errorMessage = message;
    errorMessageAllocator = allocator;
}
