#pragma once
#include <initializer_list>

#include "allocators.h"


/** Represents string encoding. */
enum class Encoding
{
    /** Unknown encoding. */
    Unknown = 0,

    /** UTF-8 encoding. */
    UTF8,

    /** ASCII encoding. */
    ASCII,

    /** Represents maximum value of Encoding enumeration. */
    MAX_VALUE
};


/** Represents comparison method between strings. */
enum class StringComparison
{
    /** String comparison is case-sensitive. */
    CaseSensitive = 0,

    /**  String comparison is case-insensitive. */
    CaseInsensitive
};


/** Represents string. */
struct Newstring
{
    /** Pointer to string characters. */
    wchar_t* data  = nullptr;

    /** String length. */
    uint32_t count = 0;

    /** Initializes empty string. */
	explicit Newstring();

    /** Initializes string with specified string data and length. */
	explicit Newstring(wchar_t* data, uint32_t count);

    /** Compares two strings using case-sensitive comparison. */
    bool operator==(const Newstring& rhs) const;

    /** Compares two strings using case-sensitive comparison. */
    bool operator!=(const Newstring& rhs) const;

    /** Compares Newstring and c-string using case-sensitive comparison. */
    bool operator==(const wchar_t* rhs) const;

    /** Compares Newstring and c-string using case-sensitive comparison. */
    bool operator!=(const wchar_t* rhs) const;

    /** Compares two strings using specified comparison method. */
    bool Equals(const Newstring& rhs, StringComparison comparison) const;
    
    /** Compares two strings using specified comparison method. */
    bool Equals(const wchar_t* rhs, StringComparison comparison) const;

    /**
     * Returns position of specified character in the string.
     * If character is not found, returns -1.
     */
    int IndexOf(wchar_t c) const;

    /**
     * Returns position of specified character in the string, searching from end of the string to the start.
     * If character is not found, returns -1.
     */
    int LastIndexOf(wchar_t c) const;

    /** Returns true if this string starts with specified string. */
    bool StartsWith(const Newstring& string, StringComparison comparison = StringComparison::CaseSensitive) const;

    /** Copies characters from this string to another.
     * 'fromIndex': index in this string which specified start index to copy characters from
     * 'copyCount': maximum number of characters to copy to destination.
     * 'destIndex': index in destination string where characters should be copied.
     * Returns number of characters copied.
     */
    uint32_t CopyTo(Newstring* dest, uint32_t fromIndex = 0, uint32_t destIndex = 0, uint32_t copyCount = UINT32_MAX) const;

    /** Creates string that references this string's data from specified position and length. */
    Newstring RefSubstring(uint32_t index, uint32_t count = UINT32_MAX) const;

    /** Creates copy of this string using specified allocator. */
    Newstring Clone(IAllocator* allocator = &g_standardAllocator) const;

    /** Creates copy of this string using specified allocator. Resulting string is zero-terminated. */
    wchar_t* CloneAsCString(IAllocator* allocator = &g_standardAllocator) const;

    /** Creates copy of this string using temporary allocator. Resulting string is zero-terminated. */
    __forceinline wchar_t* CloneAsTempCString() const {
        return CloneAsCString(&g_tempAllocator);
    }

    /** Returns string which references this string data, but without tabs and spaces at start and the end of string. */
    Newstring Trimmed() const;

    /** Returns string which references this string data, but without tabs and spaces the end of string. */
    Newstring TrimmedRight() const;

    /** Deallocates string data using specified allocator and resets string state. */
    void Dispose(IAllocator* allocator = &g_standardAllocator);

    /** Skips specified characters from start of the string. */
    Newstring SkipChar(wchar_t c) const;

    /** Returns true if string data is null or string length is zero. */
    static bool IsNullOrEmpty(const Newstring& string);

    /** Returns true if pointer to string is null or string data is null or string length is zero. */
    static bool IsNullOrEmpty(const Newstring* const string);
    
    /**
     * Allocates string with length enough to hold specified amount of characters.
     * If allocation fails, returns empty string.
     */
    static Newstring New(uint32_t count, IAllocator* allocator = &g_standardAllocator);

    /**
     * Allocates string with same data as specified c-string using specified allocator.
     * If allocation fails, returns empty string.
     */
    static Newstring NewFromWChar(const wchar_t* string, IAllocator* allocator = &g_standardAllocator);
    
    /**
     * Allocates string with same data as specified c-string using specified allocator.
     * If allocation fails, returns empty string.
     */
    static Newstring NewFromWChar(const wchar_t* string, uint32_t count, IAllocator* allocator = &g_standardAllocator);
    
    /**
     * Allocates string with same data as specified c-string using specified allocator. String is zero-terminated.
     * If allocation fails, returns empty string.
     */
    static wchar_t* NewCStringFromWChar(const wchar_t* string, IAllocator* allocator = &g_standardAllocator) noexcept;

    /**
     * Allocates string with same data as specified string using specified allocator.
     * If allocation fails, returns empty string.
     */
    static Newstring Clone(const Newstring& string, IAllocator* allocator);

    /**
     * Allocates string with same data as specified string using specified allocator.
     * If allocation fails, returns empty string.
     */
    static Newstring Clone(const Newstring* string, IAllocator* allocator);

    /** Joins several strings into one without any delimiters between strings. */
    static Newstring Join(std::initializer_list<Newstring> strings, IAllocator* allocator = &g_standardAllocator);

    /** Formats string using standard allocator. */ 
    static Newstring Format(const wchar_t* format, ...);

    /** Formats string using standard allocator. String is zero-terminated. */ 
    static wchar_t* FormatCString(const wchar_t* format, ...);

    /** Formats string using temporary allocator. */ 
    static Newstring FormatTemp(const wchar_t* format, ...);

    /** Formats string using temporary allocator. String is null-terminated. */ 
    static wchar_t* FormatTempCString(const wchar_t* format, ...);

    /** Formats string using specified allocator. */
    static Newstring FormatWithAllocator(IAllocator* allocator, const wchar_t* format, ...);

    /** Formats string using specified allocator and varargs list, and optionally appends terminating zero to the string. */
    static Newstring FormatWithAllocator(IAllocator* allocator, const wchar_t* format, va_list args, bool includeTerminatingNull);

    /** Creates new string which references specified c-string. If c-string is null, returns empty string. */
    static Newstring WrapWChar(wchar_t* string);
    
    /** Creates new string which references specified c-string. If c-string is null, returns empty string. */
    static Newstring WrapWChar(wchar_t* string, uint32_t count);

    /** Creates new string which references specified constant c-string. If c-string is null, returns empty string. */
    static const Newstring WrapConstWChar(const wchar_t* string);

    /** Creates new string which references specified constant c-string. If c-string is null, returns empty string. */
    static const Newstring WrapConstWChar(const wchar_t* string, uint32_t count);

    /**  Returns string with null string data and zero length. */
    static Newstring Empty();
};
