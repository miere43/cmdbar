#include "os_utils.h"
#include "math_utils.h"
#include "trace.h"
#include "parse_utils.h"
#include "unicode.h"


String OSUtils::formatErrorCode(DWORD errorCode, DWORD languageID, IAllocator* allocator)
{
	static const DWORD flags = FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK;
	static const uint32_t maxMessageSize = 256;

    assert(allocator);

	String msg = String::alloc(maxMessageSize, allocator);
	if (msg.data == nullptr)
		return String::null;

	DWORD count = FormatMessageW(flags, 0, errorCode, languageID, msg.data, msg.count, nullptr);
	if (count == 0)
    {
		allocator->dealloc(msg.data);
		return String::null;
	}

	return msg;
}

void* OSUtils::readFileContents(const String& fileName, uint32_t* fileSize, IAllocator* allocator)
{
	if (allocator == nullptr || fileSize == nullptr)
		return nullptr;

	HANDLE handle = CreateFileW(fileName.data, FILE_READ_ACCESS, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (handle == INVALID_HANDLE_VALUE)
		return nullptr;

	LARGE_INTEGER size;
	if (!GetFileSizeEx(handle, &size) || size.LowPart == 0)
		goto exitCloseHandle;

	void* data = allocator->alloc(size.LowPart);
	if (data == nullptr)
		goto exitCloseHandle;

	DWORD abcd;
	if (!ReadFile(handle, data, size.LowPart, &abcd, nullptr) || abcd != size.LowPart)
		goto exitFreeData;

	CloseHandle(handle);
	*fileSize = size.LowPart;

	return data;

exitFreeData:
	allocator->dealloc(data);
exitCloseHandle:
	CloseHandle(handle);
	return nullptr;
}

String OSUtils::readAllText(const String& fileName, Encoding encoding, IAllocator* allocator)
{
    assert(allocator);

	encoding = normalizeEncoding(encoding);
	if (encoding == Encoding::Unknown)
		return String::null;

	uint32_t fileSize = 0;
	void* data = readFileContents(fileName, &fileSize, allocator);

	if (data == nullptr)
		return String::null;

    String result = unicode::decodeString(data, fileSize, encoding, allocator);
	allocator->dealloc(data);

    return result;
}

String OSUtils::getDirectoryFromFileName(const String& fileName, IAllocator* allocator)
{
    assert(allocator != nullptr);

	if (fileName.isEmpty())
		return String::null;

	int backSlash    = fileName.lastIndexOf(L'\\');
	int forwardSlash = fileName.lastIndexOf(L'/');

	int slash = math::max(backSlash, forwardSlash);

	if (slash == -1)
		return String::clone(fileName, allocator);

	return String::clone(fileName.substring(0, slash));
}

String OSUtils::buildCommandLine(const String* strings[], size_t stringsArrayLength, IAllocator* allocator)
{
    assert(allocator != nullptr);

	if (strings == nullptr || stringsArrayLength == 0)
		return String::null;

	Array<wchar_t> result { allocator };
	result.reserve(64);

	for (size_t i = 0; i < stringsArrayLength; ++i)
	{
		const String* arg = strings[i];
		if (arg == nullptr || arg->isEmpty())
			continue;

		bool hasSpaces = arg->indexOf(L' ') != -1;
		if (hasSpaces)
		{
			result.reserve(result.count + arg->count + 2);
			result.add(L'"');
		} else
			result.reserve(result.count + arg->count);

		memcpy(result.data + result.count, arg->data, arg->count * sizeof(wchar_t));
		result.count += arg->count;

		if (hasSpaces) 
			result.add(L'"');

		if (i != stringsArrayLength - 1)
			result.add(L' ');
	}

	result.add('\0');

	return String { result.data, result.count - 1 };
}

