#include <windowsx.h>

#include "text_edit.h"
#include "clipboard.h"
#include "defer.h"


bool TextEdit::Initialize()
{
    return buffer.Reserve(512);
}

bool TextEdit::HandleOnCharEvent(wchar_t c)
{
    if (!iswprint(c)) return 0;

    if (IsTextSelected())
    {
        buffer.Remove(selectionStartPos, GetSelectionLength());

        buffer.Insert(selectionStartPos, c);
        caretPos = selectionStartPos + 1;
        
        ClearSelection();
    }
    else
    {
        buffer.Insert(caretPos, c);
        ++caretPos;
    }

    return true;
}

bool TextEdit::HandleOnKeyDownEvent(LPARAM lParam, uint32_t vk)
{
    bool shift = !!(GetKeyState(VK_LSHIFT) & 0x8000);
    bool control = !!(GetKeyState(VK_LCONTROL) & 0x8000);

    switch (vk)
    {
        case VK_BACK:
        case VK_DELETE:
        {
            if (IsTextSelected())  RemoveSelectedText();
            else  vk == VK_BACK ? RemovePrevCharacter() : RemoveNextCharacter();
            return true;
        }
        case VK_RIGHT:
        {
            if (shift)  AddNextCharacterToSelection();
            else  IsTextSelected() ? ClearSelection(GetSelectionEnd()) : MoveCaretRight();
            return true;
        }

        case VK_LEFT:
        {
            if (shift)  AddPrevCharacterToSelection();
            else  IsTextSelected() ? ClearSelection(GetSelectionStart()) : MoveCaretLeft();
            return true;
        }
    }

    if (control && vk == 'A')       SelectAll();
    else if (control && vk == 'C')  CopySelectionToClipboard();
    else if (control && vk == 'V')  PasteTextFromClipboard();

    return true;
}

void TextEdit::SelectAll()
{
    if (buffer.count == 0)  return;

    selectionStartPos = 0;
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
    selectionStartPos = 0xFFFFFFFF;
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
    return selectionStartPos != 0xFFFFFFFF && caretPos != selectionStartPos;
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

    OutputDebugStringW(Newstring::FormatTempCString(L"selectionStartPos %d  caretPos %d  GetSelectionLength() %d\n", selectionStartPos, caretPos, GetSelectionLength()).data);
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

    OutputDebugStringW(Newstring::FormatTempCString(L"selectionStartPos %d  caretPos %d  GetSelectionLength() %d\n", selectionStartPos, caretPos, GetSelectionLength()).data);
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

void TextEdit::CopySelectionToClipboard()
{
    if (!IsTextSelected())
        return;
    
    // @TODO: Replace 0 with hwnd.
    if (!Clipboard::Open(0))
    {
        assert(false);
        return;
    }

    Newstring copyStr = buffer.string.RefSubstring(selectionStartPos, GetSelectionLength());
    bool copied = Clipboard::CopyText(copyStr);
    assert(copied);

    Clipboard::Close();
}

void TextEdit::PasteTextFromClipboard()
{
    if (!Clipboard::Open(0))
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
        buffer.Remove(selectionStartPos, GetSelectionLength());
        buffer.Insert(selectionStartPos, textToCopy);

        AddCaretPos(selectionStartPos + textToCopy.count);
        ClearSelection();

        //OnTextChanged();
        //shouldDrawCaret = true;
        //SetCaretTimer();

        //Redraw();
    }
    else
    {
        buffer.Insert(caretPos, textToCopy);
        AddCaretPos(textToCopy.count);

        //OnTextChanged();
        //shouldDrawCaret = true;
        //SetCaretTimer();

        //Redraw();
    }
}
