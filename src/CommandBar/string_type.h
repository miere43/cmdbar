#pragma once
#include <stdint.h>

#include "allocators.h"
#include "array.h"


#define CB_STRING_LITERAL(s) String(s, (sizeof(s) / sizeof(s[0]) - 1))

enum class StringComparison
{
    CaseSensitive,
    CaseInsensitive
};

struct String
{
	wchar_t* data  = nullptr;
	int      count = 0;

	inline String() { }
	inline String(wchar_t* data) {
		this->data  = data;
        this->count = data ? static_cast<int>(wcslen(data)) : 0;
	}
	inline String(wchar_t* data, int count) {
		this->data  = data;
        this->count = data ? count : 0;
	}

	inline bool isEmpty() const { return data == nullptr || count <= 0; }

    String trimmed() const;
    String substring(int startPos, int length = -1) const;
    int indexOf(uint32_t codepoint) const;
    int indexOf(const String& str) const;
    int lastIndexOf(uint32_t codepoint) const;
    bool equals(const wchar_t* str, int count, StringComparison cmpmode = StringComparison::CaseSensitive) const;
    bool equals(const wchar_t* str, StringComparison cmpmode = StringComparison::CaseSensitive) const;
    bool equals(const String& rhs, StringComparison cmpmode = StringComparison::CaseSensitive) const;

    static String alloc(int count, IAllocator* allocator = &g_standardAllocator);
    static String clone(const char* string, int count, IAllocator* allocator = &g_standardAllocator);
    static String clone(const char* string, IAllocator* allocator = &g_standardAllocator);
    static String clone(const wchar_t* string, int count, IAllocator* allocator = &g_standardAllocator);
    static String clone(const wchar_t* string, IAllocator* allocator = &g_standardAllocator);
    static String clone(const String& string, IAllocator* allocator = &g_standardAllocator);

	static String null;
};


bool split(const String& string, Array<String>* splits, uint32_t maxSplits = UINT32_MAX);
bool removeCharAt(wchar_t* string, size_t stringLength, size_t pos);
bool removeRange(wchar_t* string, size_t stringLength, size_t rangeStart, size_t rangeLength);
bool insertCharAt(wchar_t* string, size_t stringLength, size_t stringMaxLength, size_t pos, wchar_t charToAdd);
String join(const String& seperator, const String* strings[], int stringsArrayLength, IAllocator* allocator = &g_standardAllocator);
bool charToWideChar(const char* source, size_t sourceLength, wchar_t* destination, size_t destinationLength);


int stringFindCharIndex(const wchar_t* str, int count, wchar_t c);
bool stringStartsWith(const wchar_t* str, int strCount, const wchar_t* substr, int substrCount, bool caseSensitive);
//int stringReplaceRange(const wchar_t* str, int strCount, int strMaxCount, int replaceStart, int replaceLength, const wchar_t* replstr, int replstrCount);

enum class Encoding
{
    Unknown = 0,
    UTF8,
    ASCII,
    MAX_VALUE
};
