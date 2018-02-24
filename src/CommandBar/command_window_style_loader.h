#pragma once
#include "command_window.h"
#include "newstring.h"

struct CommandWindowStyleLoader
{
    static bool LoadFromFile(const Newstring& filePath, CommandWindowStyle* style);
};