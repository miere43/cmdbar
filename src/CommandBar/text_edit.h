//#pragma once
//#include "newstring.h"
//#include "newstring_builder.h"
//
//struct CommandWindow;
//
//struct TextEdit
//{
//    NewstringBuilder buffer;
//    uint32_t cursorPos = 0;
//    uint32_t selectionStartPos = 0xFFFFFFFF;
//
//    bool Initialize(CommandWindow* window);
//
//    void HandleOnCharEvent(wchar_t c);
//    void HandleOnKeyDownEvent(uint32_t vk);
//
//    void ClearText();
//
//    void ClearSelection();
//    uint32_t GetSelectionLength() const;
//    bool IsTextSelected() const;
//private:
//    CommandWindow* window = nullptr;
//};