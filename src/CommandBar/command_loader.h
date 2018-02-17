#pragma once
#include "string_type.h"
#include "command_engine.h"
#include "newstring.h"

struct CommandLoader
{
    String source;
    Array<CommandInfo*> commandInfoArray;

    Array<Command*> LoadFromFile(const Newstring& filePath);
private:
    CommandInfo* findCommandInfoByName(const String& name);
};