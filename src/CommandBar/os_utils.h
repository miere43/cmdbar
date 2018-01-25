#pragma once
#include "string_type.h"
#define NOMINMAX
#include <Windows.h>


struct OSUtils
{
	static String formatErrorCode(DWORD errorCode, DWORD languageID = 0, IAllocator* allocator = &g_standardAllocator);
    // 'fileName' must be null-terminated
	static void*  readFileContents(const String& fileName, uint32_t* fileSize, IAllocator* allocator = &g_standardAllocator);
    // 'fileName' must be null-terminated
	static String readAllText(const String& fileName, Encoding encoding = Encoding::UTF8, IAllocator* allocator = &g_standardAllocator);

	static String getDirectoryFromFileName(const String& fileName, IAllocator* allocator = &g_standardAllocator);
	static String buildCommandLine(const String* strings[], size_t stringsArrayLength, IAllocator* allocator = &g_standardAllocator);

    static bool fileExists(const String& fileName);

	static inline Encoding normalizeEncoding(Encoding encoding) {
		if (encoding < (Encoding)0 || encoding >= Encoding::MAX_VALUE)
			return Encoding::Unknown;
		return encoding;
	}
};