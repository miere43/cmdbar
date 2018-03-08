#pragma once
#include "command_engine.h"
#include "array.h"


/**
 * Represents history for user entries.
 */
struct CommandHistory
{
    /**
     * Represents text entered by user.
     */
    struct Entry
    {
        Newstring text;
    };

    /**
     * Maximum number of commands to store in command history.
     * If number of commands that is stored exceeds specified amount, then the most old command is discarded.
     */
    enum { MaxEntries = 16 };

    /**
     * Saves entry to command history.
     * Previous command is set to specified command, which means next call to GetPrevEntry() will return specified command.
     */
    void SaveEntry(const Newstring& text);

    /**
     * Rethieves previous (newest) entry.
     * If no entries are stored, then returns null pointer.
     * If there is no previous entry, then entries are wrapped around and newest entry is returned.
     */
    const Newstring* GetPrevEntry();

    /**
    * Rethieves next (oldest) entry.
    * If no entries are stored, then returns null pointer.
    * If there is no next entry, then entries are wrapped around and oldest entry is returned.
    */
    const Newstring* GetNextEntry();

    /**
     * Resets current entry index so GetPrevEntry() will return newest entry on next call.
     */
    void ResetCurrentEntryIndex();

    /**
     * Disposes resources used by this command history instance.
     */
    void Dispose();
private:
    bool TryCreateEntry(const Newstring& text, Entry* entry);

    /**
     * Disposes resources used by single entry.
     */
    void DisposeEntry(Entry* entry);

    /**
     * @TODO: Use inline array instead of heap-allocated thing.
     */
    Array<Entry> history;
    
    /**
     * Current entyr index in history array.
     */
    uint32_t current = 0;
};