#include "command_window_style_loader.h"
#include "parse_ini.h"
#include "os_utils.h"
#include "string_utils.h"
#include "newstring.h"

bool CommandWindowStyleLoader::LoadFromFile(const Newstring& filePath, CommandWindowStyle* style)
{
    if (Newstring::IsNullOrEmpty(filePath) || style == nullptr)
        return false;

    Newstring text = OSUtils::ReadAllText(filePath);
    if (Newstring::IsNullOrEmpty(text))
        return false;

    INIParser p;
    p.Initialize(text);

    while (p.Next())
    {
        switch (p.type)
        {
            case INIValueType::KeyValuePair:
            {
                if (p.key == L"borderSize")
                {
                    int borderSize;
                    if (!StringUtils::ParseInt32(p.value, &borderSize, 0))
                        return false;
                    style->borderSize = borderSize;
                }
            }
        }
    }

    return true;
}
