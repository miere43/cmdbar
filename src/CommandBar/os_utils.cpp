#include "os_utils.h"
#include "math_utils.h"
#include "parse_utils.h"
#include "unicode.h"
#include "array.h"

Newstring OSUtils::FormatErrorCode(DWORD errorCode, DWORD languageID, IAllocator* allocator)
{
    static const DWORD flags = FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK;
    static const uint32_t maxMessageSize = 256;

    assert(allocator);

    Newstring msg = Newstring::New(maxMessageSize, allocator);
    if (Newstring::IsNullOrEmpty(msg))
        return Newstring::Empty();

    DWORD count = FormatMessageW(flags, 0, errorCode, languageID, msg.data, msg.count, nullptr);
    if (count == 0)
    {
        allocator->Deallocate(msg.data);
        return Newstring::Empty();
    }

    msg.count = count;
    return msg;
}

Newstring OSUtils::GetDirectoryFromFileName(const Newstring& fileName, IAllocator* allocator)
{
    assert(allocator != nullptr);

    if (Newstring::IsNullOrEmpty(fileName))
        return Newstring::Empty();

	int backSlash    = fileName.LastIndexOf(L'\\');
	int forwardSlash = fileName.LastIndexOf(L'/');

	int slash = math::max(backSlash, forwardSlash);

	if (slash == -1)
		return Newstring::Clone(fileName, allocator);

	return Newstring::Clone(fileName.RefSubstring(0, slash), allocator);
}

Newstring OSUtils::BuildCommandLine(const Newstring* strings[], size_t stringsArrayLength, IAllocator* allocator)
{
    assert(allocator != nullptr);

	if (strings == nullptr || stringsArrayLength == 0)
		return Newstring::Empty();

	Array<wchar_t> result { allocator };
	result.reserve(64);

	for (size_t i = 0; i < stringsArrayLength; ++i)
	{
		const Newstring* arg = strings[i];
		if (Newstring::IsNullOrEmpty(arg))
			continue;

		bool hasSpaces = arg->IndexOf(L' ') != -1;
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

	return Newstring(result.data, result.count - 1);
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

    data = allocator->Allocate(size.LowPart);
    if (data == nullptr)
        goto exitCloseHandle;

    DWORD abcd;
    if (!ReadFile(handle, data, size.LowPart, &abcd, nullptr) || abcd != size.LowPart)
        goto exitFreeData;

    *fileSize = size.LowPart;

    goto exitCloseHandle;

exitFreeData:
    allocator->Deallocate(data);
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
    allocator->Deallocate(data);

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
    
    bool result = WriteFileContents(fileName, data, nsize);

    g_standardAllocator.Deallocate(data);

    return result;
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

