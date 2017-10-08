#pragma once
#include "allocators.h"
#include "array.h"

struct String
{
	wchar_t* data  = nullptr;
	int      count = 0;

	inline String() { }
	inline String(wchar_t* data) {
		this->data  = data;
        this->count = data ? wcslen(data) : 0;
	}
	inline String(wchar_t* data, int count) {
		this->data  = data;
        this->count = data ? count : 0;
	}

	inline bool isEmpty() const { return data == nullptr || count <= 0; }

	static String null;
};

String allocateStringOfLength(int length, IAllocator* allocator = &g_standardAllocator);
String clone(const char* string, int length, IAllocator* allocator = &g_standardAllocator);
inline String clone(const char* string, IAllocator* allocator = &g_standardAllocator) {
	return string == nullptr ? String::null : clone(string, strlen(string), allocator);
}
String clone(const String& other, IAllocator* allocator = &g_standardAllocator);
bool equals(const String& a, const String& b);
bool split(const String& string, Array<String>* splits, uint32_t maxSplits = SIZE_MAX);
int indexOf(const String& string, wchar_t character);
int indexOf(const String& string, const String& other);
int lastIndexOf(const String& string, wchar_t character);
String substringRef(const String& string, int pos, int length = -1);
bool removeCharAt(wchar_t* string, size_t stringLength, size_t pos);
bool removeRange(wchar_t* string, size_t stringLength, size_t rangeStart, size_t rangeLength);
bool insertCharAt(wchar_t* string, size_t stringLength, size_t stringMaxLength, size_t pos, wchar_t charToAdd);
String join(const String& seperator, const String* strings[], int stringsArrayLength, IAllocator* allocator = &g_standardAllocator);
bool charToWideChar(const char* source, size_t sourceLength, wchar_t* destination, size_t destinationLength);

#define CB_STRING_LITERAL(s) String(s, (sizeof(s) / sizeof(s[0]) - 1))