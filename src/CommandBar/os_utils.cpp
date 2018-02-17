#include "os_utils.h"
#include "math_utils.h"
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

void* OSUtils::ReadFileContents(const Newstring& fileName, uint32_t* fileSize, IAllocator* allocator)
{
    void* data = nullptr;
    if (Newstring::IsNullOrEmpty(fileName) || fileSize == nullptr || allocator == nullptr)
        return nullptr;

    Newstring actualFileName = MaybeReallocAsZeroTerminated(fileName);

    HANDLE handle = CreateFileW(actualFileName.data, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle == INVALID_HANDLE_VALUE)
        goto exitFreeActualFileName;

    LARGE_INTEGER size;
    if (!GetFileSizeEx(handle, &size) || size.LowPart == 0)
        goto exitCloseHandle;

    data = allocator->alloc(size.LowPart);
    if (data == nullptr)
        goto exitCloseHandle;

    DWORD abcd;
    if (!ReadFile(handle, data, size.LowPart, &abcd, nullptr) || abcd != size.LowPart)
        goto exitFreeData;

    *fileSize = size.LowPart;

    goto exitCloseHandle;

exitFreeData:
    allocator->dealloc(data);
    data = nullptr;
exitCloseHandle:
    CloseHandle(handle);
exitFreeActualFileName:
    MaybeDisposeZeroTerminated(fileName, &actualFileName);
    return data;
}

bool OSUtils::WriteFileContents(const Newstring& fileName, void* contents, uint32_t contentsSize)
{
    assert(contents);

    bool result = false;
    if (Newstring::IsNullOrEmpty(fileName))
        return false;

    Newstring actualFileName = MaybeReallocAsZeroTerminated(fileName);

    HANDLE handle = CreateFileW(actualFileName.data, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle == INVALID_HANDLE_VALUE)
        goto exitFreeActualFileName;

    DWORD nwritten;
    if (!(WriteFile(handle, contents, contentsSize, &nwritten, nullptr) && nwritten == contentsSize))
        goto exitCloseHandle;

    result = true;
    goto exitCloseHandle;

exitCloseHandle:
    CloseHandle(handle);
exitFreeActualFileName:
    MaybeDisposeZeroTerminated(fileName, &actualFileName);
    return result;
}

Newstring OSUtils::ReadAllText(const Newstring& fileName, Encoding encoding, IAllocator* allocator)
{
    assert(allocator);

    if (Newstring::IsNullOrEmpty(fileName))
        return Newstring::Empty();

    encoding = normalizeEncoding(encoding);
    if (encoding == Encoding::Unknown)
        return Newstring::Empty();

    uint32_t fileSize = 0;
    void* data = ReadFileContents(fileName, &fileSize, allocator);

    if (data == nullptr)
        return Newstring::Empty();

    Newstring result = unicode::DecodeString(data, fileSize, encoding, allocator);
    allocator->dealloc(data);

    return result;
}

bool OSUtils::WriteAllText(const Newstring& fileName, const Newstring& text, Encoding encoding)
{
    encoding = normalizeEncoding(encoding);
    if (encoding == Encoding::Unknown)
        return false;

    uint32_t nsize;
    void* data = unicode::EncodeString(text, &nsize, encoding);
    if (!data)  return false;
    
    return WriteFileContents(fileName, data, nsize);
}

void OSUtils::truncateFileNameToDirectory(String* fileName)
{
    assert(fileName != nullptr);

    if (fileName->isEmpty())
        return;

    int backSlash    = fileName->lastIndexOf(L'\\');
    int forwardSlash = fileName->lastIndexOf(L'/');

    int slash = math::max(backSlash, forwardSlash);

    if (slash != -1)
        *fileName = fileName->substring(0, slash);
}

Newstring OSUtils::MaybeReallocAsZeroTerminated(const Newstring & fileName)
{
    return fileName.IsZeroTerminated() ? fileName : fileName.CloneAsCString();
}

void OSUtils::MaybeDisposeZeroTerminated(const Newstring& originalFileName, Newstring* actualFileName)
{
    if (originalFileName.data != actualFileName->data)
        actualFileName->Dispose();
}

void OSUtils::getApplicationDirectory(StringBuilder* builder)
{
    assert(builder != nullptr);
    assert(builder->allocator != nullptr);

    const DWORD count = MAX_PATH + 1;
    builder->reserve(builder->str.count + count);

    DWORD actualCount = GetModuleFileNameW(
        GetModuleHandleW(0), 
        &builder->str.data[builder->str.count],
        builder->capacity - builder->str.count);
    if (actualCount >= count)
    {
        // @TODO
        assert(false);
    }

    builder->str.count = actualCount - 1;
    truncateFileNameToDirectory(&builder->str);
}

void OSUtils::GetApplicationDirectory(NewstringBuilder* sb)
{
    assert(sb != nullptr);

    const DWORD count = MAX_PATH + 1;
    sb->Reserve(sb->count + count);

    DWORD actualCount = GetModuleFileNameW(
        GetModuleHandleW(0), 
        &sb->data[sb->count],
        sb->GetRemainingCapacity());

    if (actualCount == 0 || actualCount >= count)
    {
        // @TODO
        assert(false);
    }

    sb->count = actualCount - 1;
    TruncateFileNameToDirectory(&sb->string);
}

void OSUtils::TruncateFileNameToDirectory(Newstring* fileName)
{
    if (Newstring::IsNullOrEmpty(fileName))
        return;

    int backSlash    = fileName->LastIndexOf(L'\\');
    int forwardSlash = fileName->LastIndexOf(L'/');

    int slash = math::max(backSlash, forwardSlash);

    if (slash != -1)
    {
        fileName->count = slash + 1;
    }
}

bool OSUtils::FileExists(const Newstring& fileName)
{
    Newstring actualFileName = MaybeReallocAsZeroTerminated(fileName);

    DWORD attrs = GetFileAttributesW(actualFileName.data);
    bool result = attrs != 0 && !(attrs & FILE_ATTRIBUTE_DIRECTORY);

    MaybeDisposeZeroTerminated(fileName, &actualFileName);

    return result;
}

bool OSUtils::fileExists(const String & fileName)
{
    DWORD attrs = GetFileAttributesW(fileName.data);

    return attrs != 0 && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

