#include <assert.h>
#include <stdarg.h>

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
                args.Append(Newstring(expression.data + argStart, argLength));

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
                args.Append(Newstring(expression.data + argStart, argLength));
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
        args.Append(Newstring(expression.data + argStart, argLength));
    }

    if (args.count == 0)
    {
        executionState.errorMessage = Newstring::WrapConstWChar(L"Invalid input.").Clone(&g_tempAllocator);
        return false;
    }

    const Newstring& commandName = args.data[0];
    Command* command = FindCommandByName(commandName);
    executionState.command = command;

    if (command == nullptr)
    {
        executionState.FormatErrorMessage(L"Command \"%.*s\" is not found.", commandName.count, commandName.data);
        return false;
    }

    if (beforeRunCallback != nullptr)
    {
        beforeRunCallback(this, beforeRunCallbackUserdata);
    }

    Array<Newstring> actualArgs;
    for (uint32_t i = 1; i < args.count; ++i)
        actualArgs.Append(args.data[i]);
    
    return command->Execute(&executionState, actualArgs);
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

		if (name.Equals(command->name, StringComparison::CaseInsensitive))
			return command;
	}

	return nullptr;
}

bool CommandEngine::RegisterCommand(Command* command)
{
    assert(command);

    if (commands.Append(command))
    {
        command->engine = this;
        return true;
    }

    return false;
}

void CommandEngine::UnregisterAllCommands()
{
    for (uint32_t i = 0; i < commands.count; ++i)
        Memdelete(commands.data[i]);
    commands.Clear();    
}

void CommandEngine::Dispose()
{
    ClearExecutionState();
}

void CommandEngine::ClearExecutionState()
{
    ExecuteCommandState& e = executionState;

    // Cleanup here if necessary.

    e = ExecuteCommandState();
}

CommandInfo::CommandInfo()
{ }

CommandInfo::CommandInfo(Newstring dataName, CommandInfoFlags flags, CommandInfo_CreateCommand command)
	: dataName(dataName)
	, flags(flags)
	, createCommand(command)
{ }

void BaseCommandState::FormatErrorMessage(const wchar_t* format, ...)
{
    assert(format);

    va_list args;
    va_start(args, format);
    
    errorMessage = Newstring::FormatWithAllocator(&g_tempAllocator, format, args, true);
    
    va_end(args);

    if (Newstring::IsNullOrEmpty(errorMessage))
    {
        // @TODO: log error.
        errorMessage = Newstring::WrapConstWChar(L"Unknown error.").Clone(&g_tempAllocator);
        assert(!Newstring::IsNullOrEmpty(errorMessage));
    }
}

Command::~Command()
{
    name.Dispose();
}
