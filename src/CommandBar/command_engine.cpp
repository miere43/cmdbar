#include "command_engine.h"

bool CommandEngine::evaluate(const String& expression)
{
	Array<String> args;
	split(expression, &args);

	if (args.count == 0)
		return false;

	const String& commandName = args.data[0];
	Command* command = findCommandByName(commandName);
	
	if (command == nullptr || command->callback == nullptr)
		return false;

	if (beforeRunCallback != nullptr)
		beforeRunCallback(this, beforeRunCallbackUserdata);

	command->callback(*command, args.count == 1 ? nullptr : args.data + 1, args.count - 1);

	return true;
}

bool CommandEngine::addCommand(Command* command)
{
	return commands.add(command);
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

		if (equals(command->name, name))
			return command;
	}

	return nullptr;
}
