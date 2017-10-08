#include "line_reader.h"

int lineBreakLength(const wchar_t* string, size_t stringLength)
{
	if (!string || stringLength == 0) return 0;
	if (stringLength > 0 && string[0] == '\n') return 1;
	if (stringLength > 1 && string[0] == '\r' && string[1] == '\n') return 2;
	if (stringLength > 0 && string[0] == '\r') return 1;
	return 0;
}

void trimChars(wchar_t** stringRef, int* stringLengthRef)
{
	wchar_t* string = *stringRef;
	int stringLength = *stringLengthRef;

	int left = 0;
	for (; left < stringLength; ++left)
	{
		wchar_t c = string[left];
		if (c == ' ' || c == '\t') {
			continue;
		}
		else {
			break;
		}
	}

	int right = stringLength;
	for (; right >= 0; --right)
	{
		wchar_t c = string[right];
		if (c == ' ' || c == '\t') {
			continue;
		}
		else {
			break;
		}
	}

	*stringRef = string + left;
	*stringLengthRef = right - left;
}

int numCharsBeforeSpace(wchar_t* string, size_t stringLength)
{
	if (!string || stringLength == 0) return 0;
	size_t i = 0;
	for (; i < stringLength; ++i)
	{
		wchar_t c = string[i];
		if (c == ' ') return i;
	}

	return i;
}


int numSpacesBeforeChar(const wchar_t* string, size_t stringLength)
{
	if (!string || stringLength == 0) return 0;
	size_t i = 0;
	for (; i < stringLength; ++i)
	{
		wchar_t c = string[i];
		if (c != ' ') return i;
	}

	return i;
}

bool LineReader::nextLine(String * line)
{
	if (pos >= source.count) return false;
	if (line == nullptr) return false;

	wchar_t* m_string = source.data + pos;
	size_t m_stringLength;

	size_t startPos = pos;
	for (; pos < source.count; ++pos)
	{
		wchar_t c = source.data[pos];
		if (c == '\n' || c == '\r') {
			int length = lineBreakLength(source.data + pos, source.count - pos);
			m_stringLength = pos - startPos;
			pos += length;

			line->data = m_string;
			line->count = m_stringLength;

			return true;
		}
	}

	int length = lineBreakLength(source.data + pos, source.count - pos);
	m_stringLength = pos - startPos;
	pos += length;

	line->data = m_string;
	line->count = m_stringLength;

	return true;
}

void LineReader::setSource(const String & string)
{
	this->source = string;
	this->pos = 0;
}
