#pragma once
#include "command_window.h"
#include "newstring.h"


struct CommandWindowStyleLoader
{
    /**
     * Loads style settings from file.
     * If loading fails, then method returns false and 'style' parameter content remains in unconsistent state.
     */
    static bool LoadFromFile(const Newstring& filePath, CommandWindowStyle* style);
};