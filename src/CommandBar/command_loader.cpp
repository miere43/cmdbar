#include <assert.h>

#include "command_loader.h"
#include "os_utils.h"
#include "parse_ini.h"


Array<Command*> CommandLoader::LoadFromFile(const Newstring& filePath)
{
    // @TODO
    Newstring interopSource = OSUtils::ReadAllText(filePath, Encoding::UTF8);
    source = String { interopSource.data, interopSource.count };

    INIParser p;
    Array<Command*> cmds;
    Array<String> keys;
    Array<String> values;
    CommandInfo* currCmdInfo = nullptr;
    String currCmdName;

    p.init(source);

    while (p.next())
    {
        switch (p.type)
        {
            case INIValueType::Group:
                if (currCmdInfo != nullptr)
                {
                    Command* cmd = currCmdInfo->createCommand(keys, values);
                    assert(!currCmdName.isEmpty());
                    cmd->name = String::clone(currCmdName);
                    cmd->info = currCmdInfo;
                    assert(cmd);
                    cmds.add(cmd);
                    currCmdName = String{ 0 , 0 };
                }

                keys.clear();
                values.clear();
                currCmdInfo = findCommandInfoByName(p.group);
                assert(currCmdInfo);

                break;
            case INIValueType::KeyValuePair:
                if (p.key.equals(L"name"))
                {
                    assert(!p.value.isEmpty());
                    assert(currCmdName.isEmpty());
                    currCmdName = p.value;
                }
                else
                {
                    keys.add(p.key);
                    values.add(p.value);
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
        assert(!currCmdName.isEmpty());
        cmd->name = String::clone(currCmdName);
        cmd->info = currCmdInfo;
        assert(cmd);
        cmds.add(cmd);
        currCmdName = String{ 0 , 0 };
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
