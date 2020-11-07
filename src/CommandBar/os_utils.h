#pragma once
#include "newstring.h"
#include "newstring_builder.h"
#include <Windows.h>

namespace OSUtils
{
    /** 
     * Returns string representation of specified Windows error code.
     */
    Newstring FormatErrorCode(DWORD errorCode, DWORD languageID = 0, IAllocator* allocator = &g_standardAllocator);

	Newstring GetDirectoryFromFileName(const Newstring& fileName, IAllocator* allocator = &g_standardAllocator);
	Newstring BuildCommandLine(const Newstring* strings[], size_t stringsArrayLength, IAllocator* allocator = &g_standardAllocator);

    /**
     * Reads all file contents from specified file.
     * In case of error, return value is null pointer. Call GetLastError() to get error code.
     */
    void* ReadFileContents(const Newstring& fileName, uint32_t* fileSize, IAllocator* allocator = &g_standardAllocator);

    /**
     * Writes specified content to a file.
     * If file does not exist, it will be created. If file already exists, it will be overwritten.
     * In case of error, return value is false. Call GetLastError() to get error code.
     */
    bool WriteFileContents(const Newstring& fileName, void* contents, uint32_t contentsSize);
    
    /**
     * Reads all text of specified encoding from file. Text is converted to UTF-16.
     * In case of error, return value is empty string. Call GetLastError() to get error code.
     */
    Newstring ReadAllText(const Newstring& fileName, Encoding encoding = Encoding::UTF8, IAllocator* allocator = &g_standardAllocator);

    /**
     * Writes all text to file, converting text to specified encoding.
     * In case of error, return value is false. Call GetLastError() to get error code.
     */
    bool WriteAllText(const Newstring& fileName, const Newstring& text, Encoding encoding = Encoding::UTF8);

    /**
     * Writes path to current executable directory in specified string builder.
     */
    void GetApplicationDirectory(NewstringBuilder* sb);

    void TruncateFileNameToDirectory(Newstring* fileName);

    /** Returns true if specified file exists. */
    bool FileExists(const Newstring& fileName);

    /** Returns true if specified file exists. */
    bool FileExists(const wchar_t* fileName);
    
    /** Returns true if specified directory exists. */
    bool DirectoryExists(const Newstring& fileName);

    /** Returns true if specified directory exists. */
    bool DirectoryExists(const wchar_t* fileName);

    /**
     * If specified encoding is invalid, returns Encoding::Unknown, otherwise returns specified encoding.
     */
    Encoding NormalizeEncoding(Encoding encoding);
};