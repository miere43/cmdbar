#include <assert.h>

#include "string_utils.h"


bool StringUtils::insertChars(
    wchar_t* target,
    int targetCount,
    int targetMaxCount,
    const wchar_t* strToInsert,
    int strToInsertCount,
    int insertPos)
{
    assert(target);
    assert(targetCount >= 0);
    assert(targetMaxCount >= targetCount);
    assert(strToInsert);
    assert(strToInsertCount >= 0);
    assert(insertPos >= 0);

    if (insertPos > targetCount)
        return false;
    if (strToInsertCount >= targetMaxCount - targetCount)
        return false;

    // copy right part
    int moveStrIndex = insertPos + strToInsertCount;
    int moveStrCount = targetCount - insertPos;

    wmemcpy(&target[moveStrIndex], &target[insertPos], moveStrCount);

    // insert
    wmemcpy(&target[insertPos], strToInsert, strToInsertCount);

    return true;
}
