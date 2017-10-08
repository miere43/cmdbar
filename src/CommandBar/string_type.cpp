#define NOMINMAX
#include <Windows.h>
#include <assert.h>

#include "string_type.h"
#include "math_utils.h"


String String::null;

String allocateStringOfLength(int length, IAllocator * allocator)
{
	if (length <= 0 || allocator == nullptr)
		return String::null;
	
	String result;
	result.data  = (wchar_t*)allocator->allocate(sizeof(wchar_t) * (1 + length));
	result.count = length;
	
	return result;
}

String clone(const char * string, int length, IAllocator * allocator)
{
	if (string == nullptr || allocator == nullptr)
		return String::null;

	String result = allocateStringOfLength(length, allocator);

	if (result.data == nullptr)
		return String::null;

	for (int i = 0; i < length; ++i)
		result.data[i] = string[i];
	result.data[length] = '\0';
	result.count = length;

	return result;
}

String clone(const String& other, IAllocator* allocator)
{
	String result = allocateStringOfLength(other.count, allocator);
	
	if (result.data == nullptr)
		return String::null;

	memcpy_s(
		result.data,
		sizeof(wchar_t) * (other.count),
		other.data,
		sizeof(wchar_t) * (other.count));

	result.count = other.count;
	result.data[result.count] = '\0';

	return result;
}

bool equals(const String & a, const String & b)
{
	if (a.data == nullptr || b.data == nullptr || a.count != b.count)
		return false;

	return wcsncmp(a.data, b.data, math::min(a.count, b.count)) == 0;
}

bool split(const String& string, Array<String>* splits, uint32_t maxSplits)
{
	if (splits == nullptr || string.data == 0 || string.count == 0)
		return false;

	if (maxSplits == 0)
		return true;

	wchar_t*  current = string.data;
	int count  = 0;
	uint32_t numSplits = 0;

	for (int i = 0; i < string.count; ++i)
	{
		wchar_t c = string.data[i];

		if (c == ' ') {
			if (count == 0) {
				++current;
				continue;
			}

			String s;
			s.data  = current;
			s.count = count;
			
			splits->add(s);
		
			current = &string.data[i + 1];
			count = 0;
		} else {
			++count;
		}
	}

	if (count != 0) {
		String s;
		s.data = current;
		s.count = count;

		splits->add(s);
	}

	return true;
}

int indexOf(const String & string, wchar_t character)
{
	if (string.data == nullptr || string.count < 1)
		return -1;

	for (int i = 0; i < string.count; ++i)
		if (string.data[i] == character)
			return (int)i;

	return -1;
}

int indexOf(const String& string, const String& other)
{
	if (string.data == nullptr || other.data == nullptr || string.count < other.count)
		return -1;

	for (int i = 0; i <= string.count - other.count; ++i)
	{
		if (string.data[i] == other.data[0])
		{
			for (int j = i + 1; j < other.count; ++j)
			{
				if (string.data[j] != other.data[j])
				{
					goto noMatch;
				}
			}

			return i;
			noMatch: {}
		}
	}

	return -1;
}

int lastIndexOf(const String & string, wchar_t character)
{
	if (string.isEmpty())
		return -1;

	for (int i = string.count - 1; i >= 0; --i)
	{
		if (string.data[i] == character)
			return i;
	}

	return -1;
}

String substringRef(const String& string, int pos, int length)
{
	String result;

	if (string.data == nullptr || string.count < 0 || pos < 0)
		return String::null;

	result.data = string.data + pos;

	if (length == -1)
		result.count = string.count - pos;
	else
		result.count = length;

	return result;
}

bool removeCharAt(wchar_t* string, size_t stringLength, size_t pos)
{
	if (!string || !stringLength) return false;
	if (pos < 0 || pos >= stringLength) return false;

	if (pos == stringLength - 1)
	{
		return true;
	}
	else
	{
		size_t maxPos = stringLength - 1;
		for (size_t i = pos; i < maxPos; ++i)
		{
			string[i] = string[i + 1];
		}

		return true;
	}
}

bool removeRange(wchar_t * string, size_t stringLength, size_t rangeStart, size_t rangeLength)
{
    assert(string);
    assert(rangeStart + rangeLength <= stringLength);


    wchar_t* rightPartString = string + (rangeStart + rangeLength);
    size_t rightPartLength = stringLength - rangeStart - rangeLength;

    if (rightPartLength != 0)
        wmemcpy(&string[rangeStart], rightPartString, rightPartLength);
    
    return true;
}

bool insertCharAt(wchar_t* string, size_t stringLength, size_t stringMaxLength, size_t pos, wchar_t charToAdd)
{
	if (!string) return 0;
	if (stringLength >= stringMaxLength) return 0;
	if (pos < 0 || pos >= stringLength + 1) return 0;

	for (size_t i = stringLength; i > pos; --i)
	{
		string[i] = string[i - 1];
	}
	string[pos] = charToAdd;
	return 1;
}

String join(const String& seperator, const String* strings[], int stringsArrayLength, IAllocator* allocator)
{
	if (strings == nullptr || stringsArrayLength <= 0 || allocator == nullptr)
		return String::null;

	int totalCount = 0;
	for (int i = 0; i < stringsArrayLength; ++i)
	{
		const String* string = strings[i];
		if (string == nullptr || string->isEmpty())
			continue;

		totalCount += string->count + seperator.count;
	}

	String result = allocateStringOfLength(totalCount + 1, allocator); // +1 for '\0'
	if (result.data == nullptr)
		return String::null;

	wchar_t* currentData = result.data;
	size_t seperatorSize = seperator.count * sizeof(wchar_t);

	for (int i = 0; i < stringsArrayLength; ++i)
	{
		const String* string = strings[i];
		if (string == nullptr || string->isEmpty())
			continue;

		size_t offset = string->count * sizeof(wchar_t);

		memcpy(currentData, string->data, offset);
		currentData += offset;
		
		if (!seperator.isEmpty()) {
			memcpy(currentData, seperator.data, seperatorSize);
			currentData += seperatorSize;
		}
	}

	result.count = totalCount;

	return result;
}

bool charToWideChar(const char* source, size_t sourceLength, wchar_t* destination, size_t destinationLength)
{
	if (source == nullptr || destination == nullptr)
		return false;
	if (destinationLength < sourceLength)
		return false;

	size_t i = 0;
	if (sourceLength >= 4)
	{
		for (; i < sourceLength; i += 4)
		{
			*destination++ = *source++;
			*destination++ = *source++;
			*destination++ = *source++;
			*destination++ = *source++;
		}
	}

	for (; i < sourceLength; ++i)
	{
		*destination++ = *source++;
	}

	return true;
}
