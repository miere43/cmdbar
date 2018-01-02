#include <assert.h>

#include "command_engine.h"


bool CommandEngine::evaluate(const String& expression)
{
    Array<String> args;
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
                args.add(String{ expression.data + argStart, argLength });

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
                args.add(String{ expression.data + argStart, argLength });
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
        args.add(String{ expression.data + argStart, argLength });
    }

    if (args.count == 0)
        return false;

    const String& commandName = args.data[0];
    Command* command = findCommandByName(commandName);

    if (command == nullptr)
        return false;

    if (beforeRunCallback != nullptr)
        beforeRunCallback(this, beforeRunCallbackUserdata);

    Array<String> actualArgs;
    for (uint32_t i = 1; i < args.count; ++i)
        actualArgs.add(args.data[i]);

    return command->onExecute(actualArgs);
}

void CommandEngine::setBeforeRunCallback(CommandBeforeRunCallback callback, void * userdata)
{
	this->beforeRunCallback = callback;
	this->beforeRunCallbackUserdata = userdata;
}

Command* CommandEngine::findCommandByName(const String& name)
{
	for (uint32_t i = 0; i < commands.count; ++i)
	{
		Command* command = commands.data[i];

		if (name.equals(command->name))
			return command;
	}

	return nullptr;
}

bool CommandEngine::registerCommand(Command* command)
{
    assert(command);

    if (commands.add(command))
    {
        command->engine = this;
        return true;
    }

    return false;
}

bool CommandEngine::registerCommandInfo(CommandInfo* info)
{
    assert(info);

    return knownCommandInfoArray.add(info);
}

