#pragma once
#include "newstring.h"
#include "newstring_builder.h"
#define NOMINMAX
#include <Windows.h>

namespace OSUtils
{
    static Newstring FormatErrorCode(DWORD errorCode, DWORD languageID = 0, IAllocator* allocator = &g_standardAllocator);

	static Newstring GetDirectoryFromFileName(const Newstring& fileName, IAllocator* allocator = &g_standardAllocator);
	static Newstring BuildCommandLine(const Newstring* strings[], size_t stringsArrayLength, IAllocator* allocator = &g_standardAllocator);

    static void* ReadFileContents(const Newstring& fileName, uint32_t* fileSize, IAllocator* allocator = &g_standardAllocator);
    static bool WriteFileContents(const Newstring& fileName, void* contents, uint32_t contentsSize);
    
    static Newstring ReadAllText(const Newstring& fileName, Encoding encoding = Encoding::UTF8, IAllocator* allocator = &g_standardAllocator);
    static bool WriteAllText(const Newstring& fileName, const Newstring& text, Encoding encoding = Encoding::UTF8);

    static void GetApplicationDirectory(NewstringBuilder* sb);
    static void TruncateFileNameToDirectory(Newstring* fileName);

    static bool FileExists(const Newstring& fileName);
    static bool DirectoryExists(const Newstring& fileName);

	static inline Encoding normalizeEncoding(Encoding encoding)
    {
		if (encoding < (Encoding)0 || encoding >= Encoding::MAX_VALUE)
			return Encoding::Unknown;
		return encoding;
	}
};