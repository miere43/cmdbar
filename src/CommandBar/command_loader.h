#pragma once
#include "command_engine.h"
#include "newstring.h"


struct CommandLoader
{
    Array<CommandInfo*> commandInfoArray;

    Array<Command*> LoadFromFile(const Newstring& filePath);
private:
    CommandInfo* FindCommandInfoByName(const Newstring& name);
};