#pragma once
#include "string_type.h"
#include "command_engine.h"


struct CommandLoader
{
    String source;
    Array<CommandInfo*> commandInfoArray;

    Array<Command*> loadFromFile(String filePath);
private:
    CommandInfo* findCommandInfoByName(const String& name);
};