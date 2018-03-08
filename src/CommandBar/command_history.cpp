#include "command_history.h"


void CommandHistory::SaveEntry(const Newstring& text)
{
    history.Reserve(MaxEntries);
    
    Entry entry;
    if (!TryCreateEntry(text, &entry))
        return;

    if (history.count >= MaxEntries)
        history.Remove(0);

    if (!history.Append(entry))
        DisposeEntry(&entry);

    ResetCurrentEntryIndex();
}

const Newstring* CommandHistory::GetPrevEntry()
{
    if (history.count == 0)  return nullptr;
    assert(current >= 0 && current < history.count);

    const Entry& entry = history.data[current];
    current = current != 0 ? current - 1 : history.count - 1;

    return &entry.text;
}

const Newstring* CommandHistory::GetNextEntry()
{
    if (history.count == 0)  return nullptr;
    assert(current >= 0 && current < history.count);

    const Entry& entry = history.data[current];
    current = (current + 1) % history.count;

    return &entry.text;
}

void CommandHistory::ResetCurrentEntryIndex()
{
    current = history.count != 0 ? history.count - 1 : 0;
}

void CommandHistory::Dispose()
{
    current = 0;

    for (uint32_t i = 0; i < history.count; ++i)
    {
        DisposeEntry(&history.data[i]);
    }

    history.Dispose();
}

bool CommandHistory::TryCreateEntry(const Newstring& text, Entry* entry)
{
    assert(entry);
    if (Newstring::IsNullOrEmpty(text))  return false;

    Newstring clonedText = text.Clone();
    if (Newstring::IsNullOrEmpty(clonedText))  return false;

    entry->text = clonedText;

    return true;
}

void CommandHistory::DisposeEntry(Entry* entry)
{
    assert(entry);

    entry->text.Dispose();
}
