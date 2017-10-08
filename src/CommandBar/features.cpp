#include <ShlObj.h>

#include "features.h"
#include "line_reader.h"
#include "os_utils.h"
#include "trace.h"


static void execCommand(Command& command, const String* args, uint32_t numArgs)
{
	String* fileName = ((String*)command.userdata);
	if (fileName == nullptr || fileName->isEmpty())
		return;

	STARTUPINFOW i;
	memset(&i, 0, sizeof(i));
	i.cb = sizeof(i);
	PROCESS_INFORMATION info;

	Array<const String*> strings;
	strings.add(fileName);
	for (uint32_t i = 0; i < numArgs; ++i) {
		strings.add(&args[i]);
	}

	String currentDirectory = OSUtils::getDirectoryFromFileName(*fileName);
	String commandLine = OSUtils::buildCommandLine(strings.data, strings.count);

	OutputDebugStringW(commandLine.data);
	OutputDebugStringW(L"\n");

	bool success = 0 != CreateProcessW(
		strings.data[0]->data,//fileName->data,
		commandLine.data,//nullptr, //commandLine.data,
		nullptr,
		nullptr,
		true,
		NORMAL_PRIORITY_CLASS, 
		nullptr,
		nullptr, //currentDirectory.data,
		&i, 
		&info);

    strings.free();

	g_standardAllocator.deallocate(currentDirectory.data);
	g_standardAllocator.deallocate(commandLine.data);

	if (success)
	{
		CloseHandle(info.hProcess); // We don't care.
		CloseHandle(info.hThread);
	}
	else
	{
		MessageBoxW(0, L"Unable to execute application.", L"Error", MB_OK);
	}
}

static Command* createExecProgramCommand(const String& commandName, const String& appPath)
{
	Command* cmd = new Command();
	cmd->name = clone(commandName);
	cmd->callback = execCommand;
	cmd->userdata = (void *)new String(clone(appPath));
	return cmd;
}

static void openDirCommand(Command& command, const String* args, uint32_t numArgs)
{
	String* dir = (String*)command.userdata;
	PIDLIST_ABSOLUTE itemID = ILCreateFromPathW(dir->data);
	//SHCreateShellItemArray(itemID, nullptr, 0, nullptr, );
	HRESULT result = SHOpenFolderAndSelectItems(itemID, 1, (LPCITEMIDLIST*)&itemID, 0);
	ILFree(itemID);
}

static Command* createOpenDirCommand(const String& commandName, const String& dirPath)
{
	Command* cmd = new Command();
	cmd->name = clone(commandName);
	cmd->callback = openDirCommand;
	cmd->userdata = (void *)new String(clone(dirPath));
	return cmd;
}

bool Features::load(CommandEngine* engine)
{
	if (engine == nullptr)
		return false;

	String s = CB_STRING_LITERAL(L"D:/vlad/cb/cmds.txt");
	
	uint32_t dataLength = 0;
	void* data = OSUtils::readFileContents(s, &dataLength);

	if (data == nullptr || dataLength == 0)
		return false;

	String cmds = clone((char*)data, dataLength);

	LineReader lines;
	lines.setSource(cmds);

	String line;

	enum {
		ExecApp,
		OpenDir,
	} state = ExecApp;

	String execApp { L"!app" };
	String openDir { L"!dir" };

	while (lines.nextLine(&line))
	{
		int index = indexOf(line, L' ');

		const String& cmd = line;
		if (cmd.data[0] == '!') {
			if (index != -1) {
				Trace::debug("invalid directive.\n");
				continue;
			}
			if (equals(cmd, execApp)) {
				state = ExecApp;
			}
			else if (equals(cmd, openDir)) {
				state = OpenDir;
			}
			else {
				Trace::debug("Unknown directive.\n");
			}
			continue;
		}

		if (index == -1) {
			Trace::debug("Invalid.\n");
			continue;
		}

		String commandName = substringRef(line, 0, index);
		String path = substringRef(line, index + 1);

		switch (state)
		{
			case ExecApp:
			{
				engine->addCommand(createExecProgramCommand(commandName, path));
				continue;
			}
			case OpenDir:
			{
				engine->addCommand(createOpenDirCommand(commandName, path));
				continue;
			}
		}
	}

	return true;
}
