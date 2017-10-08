#pragma once
#include "array.h"
#include "string_type.h"

struct Command;
struct CommandEngine;
typedef void(*CommandCallback)(Command& command, const String* args, uint32_t numArgs);
typedef void(*CommandBeforeRunCallback)(CommandEngine* engine, void* userdata);

struct Command
{
	String name;

	CommandCallback callback = nullptr;
	void* userdata = nullptr;
};

struct CommandEngine
{
	bool evaluate(const String& expression);
	bool addCommand(Command* command);

	void setBeforeRunCallback(CommandBeforeRunCallback callback, void* userdata);
	Command* findCommandByName(const String& name);

	Array<Command*> commands;
	CommandBeforeRunCallback beforeRunCallback = nullptr;
	void* beforeRunCallbackUserdata = nullptr;
};