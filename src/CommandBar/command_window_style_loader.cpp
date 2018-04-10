#include "command_window_style_loader.h"
#include "parse_ini.h"
#include "os_utils.h"
#include "string_utils.h"
#include "newstring.h"


bool CommandWindowStyleLoader::LoadFromFile(const Newstring& filePath, CommandWindowStyle* style)
{
    if (Newstring::IsNullOrEmpty(filePath) || style == nullptr)
        return false;

    auto text = OSUtils::ReadAllText(filePath, Encoding::UTF8, &g_tempAllocator);
    if (Newstring::IsNullOrEmpty(text))
        return false;

    Newstring fontFamily;

    auto p = INIParser(text);
    while (p.Next())
    {
        switch (p.type)
        {
            case INIValueType::KeyValuePair:
            {
                if (p.key == L"border_size")
                {
                    int borderSize;
                    if (!StringUtils::ParseInt32(p.value, &borderSize, 0))
                        return false;
                    style->borderSize = borderSize;
                }
                else if (p.key == L"font_family")
                {
                    fontFamily.Dispose();
                    style->fontFamily = fontFamily = p.value.CloneAsCString();
                }
            }
        }
    }

    return true;
}
