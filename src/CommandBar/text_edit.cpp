#include "text_edit.h"
#include "clipboard.h"
#include "defer.h"


bool TextEdit::Initialize()
{
    return buffer.Reserve(512);
}

void TextEdit::SelectAll()
{
    selectionStartPos = buffer.count != 0 ? 0 : NoSelection;
    caretPos = buffer.count;
}

void TextEdit::Select(uint32_t start, uint32_t length)
{
    selectionStartPos = start <= buffer.count ? start : buffer.count;
    SetCaretPos(selectionStartPos);
    AddCaretPos(length);
}

void TextEdit::ClearText()
{
    buffer.count = 0;
    caretPos = 0;

    ClearSelection();
}

void TextEdit::SetText(const Newstring & text)
{
    ClearText();

    buffer.Append(text);
}   

void TextEdit::ClearSelection()
{
    selectionStartPos = NoSelection;
}

void TextEdit::ClearSelection(uint32_t newCaretPos)
{
    ClearSelection();
    SetCaretPos(newCaretPos);
}

uint32_t TextEdit::GetSelectionStart() const
{
    if (!IsTextSelected())  return 0;

    return caretPos > selectionStartPos ? selectionStartPos : selectionStartPos - GetSelectionLength();
}

uint32_t TextEdit::GetSelectionLength() const
{
    if (!IsTextSelected())  return 0;

    return caretPos > selectionStartPos ? caretPos - selectionStartPos : selectionStartPos - caretPos;
}

uint32_t TextEdit::GetSelectionEnd() const
{
    if (!IsTextSelected())  return 0;

    return GetSelectionStart() + GetSelectionLength();
}

bool TextEdit::IsTextSelected() const
{
    return selectionStartPos != NoSelection && caretPos != selectionStartPos;
}

void TextEdit::GetSelectionStartAndLength(uint32_t* start, uint32_t* length) const
{
    if (!IsTextSelected())
    {
        if (start)  *start = 0;
        if (length) *length = 0;
    }
    else
    {
        if (start)  *start  = GetSelectionStart();
        if (length) *length = GetSelectionLength();
    }
}

void TextEdit::Dispose()
{
    buffer.Dispose();
}

void TextEdit::SetCaretPos(uint32_t pos)
{
    // @TODO: Find or write clamp procedure.
    if (pos < 0)
    {
        caretPos = 0;
    }
    else if (pos > buffer.count)
    {
        caretPos = buffer.count;
    }
    else
    {
        caretPos = pos;
    }
}

void TextEdit::AddCaretPos(int offset)
{
    if (offset >= 0)
    {
        uint32_t newPos = caretPos + offset;
        caretPos = newPos >= caretPos ? newPos : UINT32_MAX;
    }
    else
    {
        uint32_t newPos = caretPos + offset;
        caretPos = newPos <= caretPos ? newPos : 0;
    }
}

void TextEdit::InsertCharacterAtCaret(wchar_t c)
{
    if (!iswprint(c))  return;

    if (IsTextSelected())
    {
        buffer.Remove(GetSelectionStart(), GetSelectionLength());
        buffer.Insert(GetSelectionStart(), c);
        
        SetCaretPos(GetSelectionStart());
        AddCaretPos(+1);

        ClearSelection();
    }
    else
    {
        buffer.Insert(caretPos, c);
        AddCaretPos(+1);
    }
}

void TextEdit::MoveCaretLeft()
{
    if (IsTextSelected())  ClearSelection();

    if (caretPos > 0)
        SetCaretPos(caretPos - 1);
}

void TextEdit::MoveCaretRight()
{
    if (IsTextSelected())  ClearSelection();

    if (caretPos < UINT32_MAX)
        SetCaretPos(caretPos + 1);
}

void TextEdit::AddNextCharacterToSelection()
{
    if (caretPos >= buffer.count)
        return;

    if (!IsTextSelected())
    {
        selectionStartPos = caretPos;
    }

    AddCaretPos(+1);
}

void TextEdit::AddPrevCharacterToSelection()
{
    if (caretPos == 0)
        return;

    if (!IsTextSelected())
    {
        selectionStartPos = caretPos;
    }

    AddCaretPos(-1);
}

void TextEdit::RemoveSelectedText()
{
    if (!IsTextSelected())  return;

    uint32_t start;
    uint32_t length;
    GetSelectionStartAndLength(&start, &length);

    ClearSelection();

    buffer.Remove(start, length);

    SetCaretPos(start);
}

void TextEdit::RemovePrevCharacter()
{
    if (caretPos != 0)
    {
        buffer.Remove(caretPos - 1, 1);
        AddCaretPos(-1);
    }
}

void TextEdit::RemoveNextCharacter()
{
    if (caretPos < buffer.count)
    {
        buffer.Remove(caretPos, 1);
    }
}

void TextEdit::CopySelectionToClipboard(HWND hwnd)
{
    if (!IsTextSelected())
        return;
    
    if (!Clipboard::Open(hwnd))
    {
        assert(false);
        return;
    }

    Newstring copyStr = buffer.string.RefSubstring(GetSelectionStart(), GetSelectionLength());
    bool copied = Clipboard::CopyText(copyStr);
    assert(copied);

    Clipboard::Close();
}

void TextEdit::PasteTextFromClipboard(HWND hwnd)
{
    if (!Clipboard::Open(hwnd))
    {
        assert(false);
        return;
    }

    Newstring textToCopy;
    bool copied = Clipboard::GetText(&textToCopy, &g_tempAllocator);
    assert(copied);

    Clipboard::Close();

    if (Newstring::IsNullOrEmpty(textToCopy))
        return;

    if (IsTextSelected())
    {
        buffer.Remove(GetSelectionStart(), GetSelectionLength());
        buffer.Insert(GetSelectionStart(), textToCopy);

        AddCaretPos(textToCopy.count);
        ClearSelection();
    }
    else
    {
        buffer.Insert(caretPos, textToCopy);
        AddCaretPos(textToCopy.count);
    }
}
