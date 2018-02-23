#include <assert.h>

#include "command_loader.h"
#include "os_utils.h"
#include "parse_ini.h"


Array<Command*> CommandLoader::LoadFromFile(const Newstring& filePath)
{
    // @TODO
    Newstring source = OSUtils::ReadAllText(filePath, Encoding::UTF8);
    if (Newstring::IsNullOrEmpty(source))
    {
        MessageBoxW(0, L"Unable to load source file.", L"Error", MB_ICONERROR);
    }

    INIParser p;
    Array<Command*> cmds;
    Array<String> keys;
    Array<String> values;
    CommandInfo* currCmdInfo = nullptr;
    Newstring currCmdName;

    p.Initialize(source);

    while (p.Next())
    {
        switch (p.type)
        {
            case INIValueType::Group:
                if (currCmdInfo != nullptr)
                {
                    Command* cmd = currCmdInfo->createCommand(keys, values);
                    assert(!Newstring::IsNullOrEmpty(currCmdName));
                    cmd->name = String::clone(String(currCmdName.data, currCmdName.count));
                    cmd->info = currCmdInfo;
                    assert(cmd);
                    cmds.add(cmd);
                    currCmdName = Newstring::Empty();
                }

                keys.clear();
                values.clear();
                currCmdInfo = findCommandInfoByName(String(p.group.data, p.group.count));
                assert(currCmdInfo);

                break;
            case INIValueType::KeyValuePair:
                if (p.key == L"name")
                {
                    assert(!Newstring::IsNullOrEmpty(p.value));
                    assert(Newstring::IsNullOrEmpty(currCmdName));
                    currCmdName = p.value;
                }
                else
                {
                    keys.add(String(p.key.data, p.key.count));
                    values.add(String(p.value.data, p.value.count));
                }

                break;
            case INIValueType::Error:
                assert(false);
                break;
        }
    }

    if (currCmdInfo != nullptr)
    {
        Command* cmd = currCmdInfo->createCommand(keys, values);
        assert(!Newstring::IsNullOrEmpty(currCmdName));
        cmd->name = String::clone(String(currCmdName.data, currCmdName.count));
        cmd->info = currCmdInfo;
        assert(cmd);
        cmds.add(cmd);
        currCmdName = Newstring::Empty();
    }

    return cmds;
}

CommandInfo* CommandLoader::findCommandInfoByName(const String& name)
{
    for (uint32_t i = 0; i < commandInfoArray.count; ++i)
    {
        CommandInfo* info = commandInfoArray.data[i];
        if (name.equals(info->dataName))
            return info;
    }

    return nullptr;
}
