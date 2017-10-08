#pragma once
#include <assert.h>

#include "command_window.h"


static void quitCommand(Command& command, const String* args, uint32_t numArgs)
{
    CommandWindow* window = static_cast<CommandWindow*>(command.userdata);
    assert(window);

    window->exit();
}

static void loadBasicCommands(CommandWindow* window)
{
    assert(window);
    assert(window->commandEngine);

    CommandEngine* e = window->commandEngine;
    Command* quit = new Command();
    quit->name = clone("quit");
    quit->callback = quitCommand;
    quit->userdata = window;
    e->addCommand(quit);
}
