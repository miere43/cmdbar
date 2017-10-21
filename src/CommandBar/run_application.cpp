//#include "run_application.h"
//#include "trace.h"

//static void runApplicationCallback(Command& command, const String* args, uint32_t numArgs)
//{
//	Trace::debug("run app");
//}
//
//bool RunApplicationCommandHandler::registerApplication(CommandEngine* engine, RunApplicationInfo* info)
//{
//	if (engine == nullptr || info == nullptr)
//		return false;
//
//	Command* command = new Command();
//	command->callback = runApplicationCallback;
//	command->name = info->commandName;
//	command->userdata = info;
//
//	infos.add(info);
//
//	bool result = engine->addCommand(command);
//	if (!result)
//		delete command;
//
//	info->command = command;
//	return result;
//}
//
//bool RunApplicationCommandHandler::registerApplicationsFromFile(const String& filePath)
//{
//	return false;
//}
