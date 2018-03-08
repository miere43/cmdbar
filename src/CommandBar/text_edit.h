#pragma once
#include <Windows.h>

#include "newstring.h"
#include "newstring_builder.h"


/**
 * Represents logic of single-line text editor control.
 */
struct TextEdit
{
    /**
     * Represents a value which indicates that text is not selected.
     */
    enum : uint32_t { NoSelection = 0xFFFFFFFF };

    /**
     * Resizeable string buffer to store control text.
     * Don't change any text of this buffer to avoid breaking control internal state, use
     * SetText(), ClearText() instead.
     */
    NewstringBuilder buffer;

    /**
     * Current caret position.
     * Use SetCaretPos() or AddCaretPos() to set caret position without breaking control internal state.
     */
    uint32_t caretPos = 0;

    /**
     * Current selection starting position. If text is not selected, value of this variable is NoSelection.
     * Use Select() or SelectAll() to set selection range without breaking control internal state.
     */
    uint32_t selectionStartPos = NoSelection;

    /**
     * Initializes resources for this instance.
     */
    bool Initialize();

    /**
     * Disposes all associated resources.
     */
    void Dispose();

    /**
     * Selects all text.
     */
    void SelectAll();

    /**
     * Select text enclosed in specified range.
     */
    void Select(uint32_t start, uint32_t length);

    /**
     * Removes all text from text buffer.
     */
    void ClearText();

    /**
     * Copies text from specified string to text buffer.
     */
    void SetText(const Newstring& text);

    /**
     * Clears currently selected text.
     */
    void ClearSelection();

    /**
     * Clears currently selected text and sets new caret position.
     */
    void ClearSelection(uint32_t newCaretPos);

    /**
     * Returns starting index of text selection.
     * If text is not selected, returns 0.
     */
    uint32_t GetSelectionStart() const;
    
    /**
     * Returns length of selected text.
     * If text is not selected, returns 0.
     */
    uint32_t GetSelectionLength() const;

    /**
     * Returns ending index of text selection.
     * If text is not selected, returns 0.
     */
    uint32_t GetSelectionEnd() const;

    /**
     * Returns true if text is selected, false otherwise.
     */
    bool IsTextSelected() const;

    /**
     * Returns starting index of text selecting and it's length.
     */
    void GetSelectionStartAndLength(uint32_t* start, uint32_t* length) const;

    /**
     * Sets caret position.
     * If new caret position is out of range, position is clamped to allowed range.
     */
    void SetCaretPos(uint32_t pos);

    /**
     * Adds specified offset to caret position, correctly handling underflows and overflows.
     */
    void AddCaretPos(int offset);

    /**
     * If text is not selected, adds specified character at caret position.
     * If text is selected, then replaces selected text with specified character.
     * If character is not printable, does nothing.
     */
    void InsertCharacterAtCaret(wchar_t c);

    /**
     * Moves caret one character to the left.
     * If caret position is 0, then does nothing.
     */
    void MoveCaretLeft();

    /**
     * Moves caret one character to the right.
     * If caret position equals to number of character in text buffer, then does nothing.
     */
    void MoveCaretRight();

    /**
     * Copies selected text to the clipboard.
     * If text is not selected, then does nothing.
     */
    void CopySelectionToClipboard(HWND hwnd);

    /**
     * Pastes text that is currently loaded in clipboard at caret position.
     * If there is no text in clipboard, then does nothing.
     */
    void PasteTextFromClipboard(HWND hwnd);

    /**
     * Adds character that is next to the caret to current selection.
     * If there is no character next to the caret, then does nothing.c
     */
    void AddNextCharacterToSelection();
    
    /**
     * Adds character that is previous to the caret to current selection.
     * If there is no character previous to the caret, then does nothing.c
     */
    void AddPrevCharacterToSelection();
    
    /**
     * Removes selected text and clears selection.
     */
    void RemoveSelectedText();
    
    /**
     * Removes character that is previous to the caret.
     */
    void RemovePrevCharacter();
    
    /**
     * Removes character that is next to the caret.
     */
    void RemoveNextCharacter();
};