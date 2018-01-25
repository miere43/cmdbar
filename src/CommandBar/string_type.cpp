#define NOMINMAX
#include <Windows.h>
#include <assert.h>

#include "string_type.h"
#include "math_utils.h"
#include "unicode.h"


String String::null;

bool split(const String& string, Array<String>* splits, uint32_t maxSplits)
{
	if (splits == nullptr || string.data == 0 || string.count == 0)
		return false;

	if (maxSplits == 0)
		return true;

	wchar_t*  current = string.data;
	int count  = 0;
	uint32_t numSplits = 0;

	for (uint32_t i = 0; i < string.count; ++i)
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

	String result = String::alloc(totalCount + 1, allocator); // +1 for '\0'
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

int stringFindCharIndex(const wchar_t* str, uint32_t count, wchar_t c)
{
    for (int i = 0; i < count; ++i)
    {
        if (str[i] == c)
            return i;
    }

    return -1;
}

bool stringStartsWith(const wchar_t * str, uint32_t strCount, const wchar_t * substr, uint32_t substrCount, bool caseSensitive)
{
    assert(str);
    assert(substr);

    if (substrCount > strCount)
        return false;

    auto compareProc = caseSensitive ? wcsncmp : _wcsnicmp;

    return compareProc(str, substr, substrCount) == 0;
}

//int stringReplaceRange(const wchar_t * str, int strCount, int strMaxCount, int replaceStart, int replaceLength, const wchar_t * replstr, int replstrCount)
//{
//    assert(str);
//    assert(strCount >= 0);
//    assert(strMaxCount >= strCount);
//    assert(replaceStart >= 0 && replaceStart < strCount);
//    assert(replaceLength >= 0);
//    assert(replstr);
//    assert(replstrCount >= 0);
//
//    int countDelta = replaceLength - replstrCount;
//    int newStrCount = strCount - replaceLength + replstrCount;
//    if (newStrCount > strMaxCount)
//        return -1;
//
//    const wchar_t* rightCopyData = &str[replaceStart + replaceLength];
//    int rightCopyCount = 
//
//    // copy part after replace dest to the right
//}

String String::trimmed() const
{
    if (isEmpty())
        return String{ data, 0 };

    uint32_t i;
    for (i = 0; i < count; ++i)
        if (!(data[i] == L' ' || data[i] == L'\t'))
            break;

    uint32_t j;
    for (j = count; j >= i; --j)
        if (!(data[j] == L' ' || data[j] == L'\t'))
            break;

    return String { data + i, j };
}

String String::substring(int startPos, int length) const
{
    assert(startPos >= 0);
    assert(length >= -1);

    String result;

    if (data == nullptr || count < 0 || startPos < 0)
        return String{ data, 0 };

    result.data  = data + startPos;
    result.count = length == -1 ? count - startPos : length;

    return result;
}

int String::indexOf(uint32_t cpToFind) const
{
    const wchar_t* curr = this->data;
    const wchar_t* next = this->data;
    const wchar_t* end  = curr + this->count;
    
    uint32_t cp;
    while ((next = unicode::decode16(curr, &cp)) < end)
    {
        if (cp == cpToFind)
            return static_cast<int>(curr - this->data);
        curr = next;
    }

    return -1;
}

int String::indexOf(const String& str) const
{
    if (this->data == nullptr || str.data == nullptr || this->count < str.count)
        return -1;

    for (uint32_t i = 0; i <= this->count - str.count; ++i)
    {
        if (this->data[i] == str.data[0])
        {
            for (uint32_t j = i + 1; j < str.count; ++j)
            {
                if (this->data[j] != str.data[j])
                {
                    goto noMatch;
                }
            }

            return (int)i;
        noMatch:
            {
            }
        }
    }

    return -1;
}

int String::lastIndexOf(uint32_t codepoint) const
{
    if (isEmpty())
        return -1;

    assert(!unicode::isSurrogatePair(codepoint));

    for (uint32_t i = count - 1; i >= 0; --i)
    {
        if (data[i] == codepoint)
            return (int)i;
    }

    return -1;
}

bool String::equals(const wchar_t* strdata, uint32_t strcount, StringComparison cmpmode) const
{
    if (this->data == nullptr || strdata == nullptr || this->count != strcount)
        return false;

    auto cmpfunc = cmpmode == StringComparison::CaseSensitive ? wcsncmp : _wcsnicmp;

    return cmpfunc(this->data, strdata, math::min(this->count, strcount)) == 0;
}

bool String::equals(const wchar_t* str, StringComparison cmpmode) const
{
    int count = str != nullptr ? static_cast<int>(wcslen(str)) : 0;
    return this->equals(str, count, cmpmode);
}

bool String::equals(const String& rhs, StringComparison cmpmode) const
{
    return this->equals(rhs.data, rhs.count, cmpmode);
}

bool String::startsWith(const String& rhs, StringComparison cmpmode) const
{
    if (rhs.isEmpty())  return false;
    if (rhs.count > this->count)  return false;

    auto compareProc = cmpmode == StringComparison::CaseSensitive ? wcsncmp : _wcsnicmp;
    return compareProc(this->data, rhs.data, rhs.count) == 0;
}

bool String::startsWith(const String& rhs, uint32_t numChars, StringComparison cmpmode) const
{
    if (rhs.isEmpty())  return false;
    if (rhs.count < numChars)  return false;
    if (numChars > this->count)  return false;

    auto compareProc = cmpmode == StringComparison::CaseSensitive ? wcsncmp : _wcsnicmp;
    return compareProc(this->data, rhs.data, numChars) == 0;
}

String String::alloc(uint32_t count, IAllocator* allocator)
{
    assert(count >= 0);
    assert(allocator);

    if (count == 0)
        return String::null;

    String result;
    result.data  = static_cast<wchar_t*>(allocator->alloc(sizeof(wchar_t) * (1 + count)));
    result.count = result.data != nullptr ? count : 0;

    return result;
}

String String::clone(const char* string, uint32_t count, IAllocator* allocator)
{
    assert(allocator != nullptr);
    if (string == nullptr)
        return String::null;

    String result = String::alloc(count, allocator);
    if (result.data == nullptr) return String::null;

    for (uint32_t i = 0; i < count; ++i)
        result.data[i] = string[i];
    result.data[count] = '\0';

    return result;
}

String String::clone(const char* string, IAllocator* allocator)
{
    assert(allocator != nullptr);
    if (string == nullptr)
        return String::null;

    return String::clone(string, static_cast<int>(strlen(string)), allocator);
}

String String::clone(const wchar_t* string, uint32_t count, IAllocator* allocator)
{
    assert(allocator != nullptr);
    if (string == nullptr)
        return String::null;

    String result = String::alloc(count, allocator);
    if (result.data == nullptr) return String::null;

    wmemcpy(result.data, string, count);
    result.data[result.count] = '\0';

    return result;
}

String String::clone(const wchar_t* string, IAllocator* allocator)
{
    assert(allocator != nullptr);
    if (string == nullptr)
        return String::null;

    return String::clone(string, static_cast<int>(wcslen(string)), allocator);
}

String String::clone(const String& string, IAllocator* allocator)
{
    assert(allocator != nullptr);
    if (string.isEmpty())
        return String::null;

    return String::clone(string.data, string.count, allocator);
}

//
//const wchar_t* Utf16::next(const wchar_t* text, uint32_t* codepoint)
//{
//    wchar_t w1 = *text++;
//    if (w1 < 0xD800 || w1 > 0xDFFF)
//    {
//        *codepoint = w1;
//        return text;
//    }
//
//    if (w1 <= 0xD800 && w1 >= 0xDBFF)
//    {
//        // Invalid codepoint.
//        *codepoint = 0;
//        return text;
//    }
//
//    wchar_t w2 = *text++;
//    if (w2 == L'\0' || !(w2 >= 0xDC00 && w2 <= 0xDFFF))
//    {
//        // Invalid codepoint.
//        *codepoint = 0;
//        return text;
//    }
//
//    *codepoint = 0x10000 + (((w1 & 0x3FF) << 10) | (w2 & 0x3FF));
//    return text;
//}
