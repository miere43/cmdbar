#pragma once
#include "string_type.h"
#include "command_window.h"
#include "newstring.h"

struct CommandWindowStyleLoader
{
    static bool LoadFromFile(const Newstring& filePath, CommandWindowStyle* style);
};