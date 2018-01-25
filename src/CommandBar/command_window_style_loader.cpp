#include "command_window_style_loader.h"
#include "parse_ini.h"
#include "os_utils.h"
#include "string_utils.h"

bool CommandWindowStyleLoader::loadFromFile(const String& filePath, CommandWindowStyle* style)
{
    if (filePath.isEmpty() || style == nullptr)
        return false;

    String text = OSUtils::readAllText(filePath);
    if (text.isEmpty())
        return false;

    INIParser p;
    p.init(text);

    while (p.next())
    {
        switch (p.type)
        {
            case INIValueType::KeyValuePair:
            {
                if (p.key.equals(L"borderSize", StringComparison::CaseInsensitive))
                {
                    int borderSize;
                    if (!StringUtils::parseInt(p.value, &borderSize, 0))
                        return false;
                    style->borderSize = borderSize;
                }
            }
        }
    }

    return true;
}
