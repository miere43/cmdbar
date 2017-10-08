#include "os_utils.h"
#include "math_utils.h"
#include "trace.h"

String OSUtils::formatErrorCode(DWORD errorCode, DWORD languageID, IAllocator* allocator)
{
	const DWORD flags = FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK;
	const uint32_t maxMessageSize = 256;

	if (allocator == nullptr)
		return String::null;

	String msg = allocateStringOfLength(maxMessageSize, allocator);
	if (msg.data == nullptr)
		return String::null;

	DWORD count = FormatMessageW(flags, 0, errorCode, languageID, msg.data, msg.count, nullptr);
	if (count == 0) {
		allocator->deallocate(msg.data);
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

	void* data = allocator->allocate(size.LowPart);
	if (data == nullptr)
		goto exitCloseHandle;

	DWORD abcd;
	if (!ReadFile(handle, data, size.LowPart, &abcd, nullptr) || abcd != size.LowPart)
		goto exitFreeData;

	CloseHandle(handle);
	*fileSize = size.LowPart;

	return data;

exitFreeData:
	allocator->deallocate(data);
exitCloseHandle:
	CloseHandle(handle);
	return nullptr;
}

String OSUtils::readAllText(const String& fileName, Encoding encoding, IAllocator* allocator)
{
	if (allocator == nullptr)
		return String::null;

	encoding = normalizeEncoding(encoding);
	if (encoding == Encoding::Unknown)
		return String::null;

	if (encoding != Encoding::ASCII)
		return String::null; // Not supported yet.

	uint32_t fileSize = 0;
	void* data = readFileContents(fileName, &fileSize, allocator);

	if (data == nullptr)
		return String::null;

	const size_t stringDataSize = (fileSize + 1) * sizeof(wchar_t);

	wchar_t* stringData = (wchar_t*)allocator->allocate(stringDataSize);
	if (stringData == nullptr)
		goto freeDataAndFail;

	if (!charToWideChar((char*)data, fileSize, stringData, stringDataSize))
		goto freeStringDataAndFail;

	allocator->deallocate(data);

	return String(stringData, fileSize);

freeStringDataAndFail:
	allocator->deallocate(stringData);
freeDataAndFail:
	allocator->deallocate(data);
	return String::null;
}

String OSUtils::getDirectoryFromFileName(const String& fileName, IAllocator* allocator)
{
	if (fileName.isEmpty() || allocator == nullptr)
		return String::null;

	int backSlash    = lastIndexOf(fileName, L'\\');
	int forwardSlash = lastIndexOf(fileName, L'/');

	int slash = math::max(backSlash, forwardSlash);

	if (slash == -1)
		return clone(fileName, allocator);

	return clone(substringRef(fileName, 0, slash));
}

String OSUtils::buildCommandLine(const String * strings[], size_t stringsArrayLength, IAllocator* allocator)
{
	if (strings == nullptr || stringsArrayLength == 0 || allocator == nullptr)
		return String::null;

	Array<wchar_t> result { allocator };
	result.reserve(64);

	for (size_t i = 0; i < stringsArrayLength; ++i)
	{
		const String* arg = strings[i];
		if (arg == nullptr || arg->isEmpty())
			continue;

		bool hasSpaces = indexOf(*arg, L' ') != -1;
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

	return String { result.data, (int)result.count - 1 };
}

