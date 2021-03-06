#include <assert.h>

#include "command_loader.h"
#include "parse_ini.h"
#include "os_utils.h"
#include "defer.h"

Array<Command*> CommandLoader::LoadFromFile(const Newstring& filePath)
{
    Newstring source = OSUtils::ReadAllText(filePath, Encoding::UTF8);
    defer(source.Dispose());

    if (Newstring::IsNullOrEmpty(source))
    {
        Newstring osError = OSUtils::FormatErrorCode(GetLastError(), 0, &g_tempAllocator);
        
        wchar_t* msg = Newstring::FormatTempCString(
            L"Unable to open commands declaration file \"%.*s\": %.*s",
            filePath.count, filePath.data,
            osError.count, osError.data);

        MessageBoxW(0, msg, L"Error", MB_ICONERROR);
    }

    INIParser p;
    Array<Command*> cmds;
    Array<Newstring> keys;
    Array<Newstring> values;
    CommandInfo* currCmdInfo = nullptr;
    Newstring currCmdName;

    CreateCommandState state;

    p.Initialize(source);

    while (p.Next())
    {
        switch (p.type)
        {
            case INIValueType::Group:
                if (currCmdInfo != nullptr)
                {
                    Command* cmd = currCmdInfo->createCommand(&state, keys, values);
                    assert(!Newstring::IsNullOrEmpty(currCmdName));
                    cmd->name = currCmdName.Clone();
                    cmd->info = currCmdInfo;
                    assert(cmd);
                    cmds.Append(cmd);
                    currCmdName = Newstring::Empty();
                }

                keys.Clear();
                values.Clear();
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
                    keys.Append(p.key);
                    values.Append(p.value);
                }

                break;
            case INIValueType::Error:
                assert(false);
                break;
        }
    }

    if (currCmdInfo != nullptr)
    {
        Command* cmd = currCmdInfo->createCommand(&state, keys, values);
        assert(!Newstring::IsNullOrEmpty(currCmdName));
        cmd->name = currCmdName.Clone();
        cmd->info = currCmdInfo;
        assert(cmd);
        cmds.Append(cmd);
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
