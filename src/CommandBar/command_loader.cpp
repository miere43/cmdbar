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
    Array<Newstring> keys;
    Array<Newstring> values;
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
                    cmd->name = currCmdName.Clone();
                    cmd->info = currCmdInfo;
                    assert(cmd);
                    cmds.add(cmd);
                    currCmdName = Newstring::Empty();
                }

                keys.clear();
                values.clear();
                currCmdInfo = FindCommandInfoByName(p.group);
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
        assert(!Newstring::IsNullOrEmpty(currCmdName));
        cmd->name = currCmdName.Clone();
        cmd->info = currCmdInfo;
        assert(cmd);
        cmds.add(cmd);
        currCmdName = Newstring::Empty();
    }

    return cmds;
}

CommandInfo* CommandLoader::FindCommandInfoByName(const Newstring& name)
{
    for (uint32_t i = 0; i < commandInfoArray.count; ++i)
    {
        CommandInfo* info = commandInfoArray.data[i];
        if (name == info->dataName)
            return info;
    }

    return nullptr;
}
