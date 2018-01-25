#pragma once
#include "string_type.h"
#include "command_window.h"

struct CommandWindowStyleLoader
{
    static bool loadFromFile(const String& filePath, CommandWindowStyle* style);
};