//#include "text_edit.h"
//
//bool TextEdit::Initialize(CommandWindow* window)
//{
//    assert(window);
//
//    this->window = window;
//
//    return buffer.Reserve(512);
//}
//
//void TextEdit::ClearText()
//{
//    buffer.count = 0;
//}
//
//void TextEdit::ClearSelection()
//{
//    selectionStartPos = 0xFFFFFFFF;
//}
//
//uint32_t TextEdit::GetSelectionLength() const
//{
//    if (!IsTextSelected())  return 0;
//
//    return cursorPos > selectionStartPos ? cursorPos - selectionStartPos : selectionStartPos - cursorPos;
//}
//
//bool TextEdit::IsTextSelected() const
//{
//    return selectionStartPos != 0xFFFFFFFF && cursorPos != selectionStartPos;
//}
