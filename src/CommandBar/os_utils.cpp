#include <algorithm>

#include "os_utils.h"
#include "parse_utils.h"
#include "unicode.h"
#include "array.h"
#include "defer.h"


namespace OSUtils {

//
// Helper procedures.
//
Newstring MaybeReallocAsZeroTerminated(const Newstring& fileName)
{
    return fileName.IsZeroTerminated() ? fileName : fileName.CloneAsCString(&g_tempAllocator);
}

Newstring FormatErrorCode(DWORD errorCode, DWORD languageID, IAllocator* allocator)
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
    return msg.TrimmedRight();
}

Newstring GetDirectoryFromFileName(const Newstring& fileName, IAllocator* allocator)
{
    assert(allocator != nullptr);

    if (Newstring::IsNullOrEmpty(fileName))
        return Newstring::Empty();

	int backSlash    = fileName.LastIndexOf(L'\\');
	int forwardSlash = fileName.LastIndexOf(L'/');

	int slash = std::max(backSlash, forwardSlash);

	if (slash == -1)
		return Newstring::Clone(fileName, allocator);

	return Newstring::Clone(fileName.RefSubstring(0, slash), allocator);
}

Newstring BuildCommandLine(const Newstring* strings[], size_t stringsArrayLength, IAllocator* allocator)
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

void* ReadFileContents(const Newstring& fileName, uint32_t* fileSize, IAllocator* allocator)
{
    bool hasError = true;
    if (Newstring::IsNullOrEmpty(fileName) || fileSize == nullptr || allocator == nullptr)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    Newstring actualFileName = MaybeReallocAsZeroTerminated(fileName);
    if (Newstring::IsNullOrEmpty(actualFileName))
        return nullptr;

    HANDLE handle = CreateFileW(actualFileName.data, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle == INVALID_HANDLE_VALUE)
        return nullptr;
    defer(CloseHandle(handle));

    LARGE_INTEGER size;
    if (!GetFileSizeEx(handle, &size) || size.LowPart == 0)
        return nullptr;

    void* data = allocator->Allocate(size.LowPart);
    defer(
        if (hasError) {
            allocator->Deallocate(data);
            data = nullptr;
        }
    );

    if (!data)  return nullptr;

    DWORD abcd;
    if (!ReadFile(handle, data, size.LowPart, &abcd, nullptr) || abcd != size.LowPart)
        return nullptr;

    *fileSize = size.LowPart;

    hasError = false;
    return data;
}

bool WriteFileContents(const Newstring& fileName, void* contents, uint32_t contentsSize)
{
    assert(contents);

    if (Newstring::IsNullOrEmpty(fileName))
        return false;

    Newstring actualFileName = MaybeReallocAsZeroTerminated(fileName);

    HANDLE handle = CreateFileW(actualFileName.data, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    defer(CloseHandle(handle));
    
    if (handle == INVALID_HANDLE_VALUE)
        return false;

    DWORD nwritten;
    if (!(WriteFile(handle, contents, contentsSize, &nwritten, nullptr) && nwritten == contentsSize))
        return false;

    return true;
}

Newstring ReadAllText(const Newstring& fileName, Encoding encoding, IAllocator* allocator)
{
    assert(allocator);

    if (Newstring::IsNullOrEmpty(fileName))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return Newstring::Empty();
    }

    encoding = NormalizeEncoding(encoding);
    if (encoding == Encoding::Unknown)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return Newstring::Empty();
    }

    uint32_t fileSize = 0;
    void* data = ReadFileContents(fileName, &fileSize, &g_tempAllocator);

    if (data == nullptr)
        return Newstring::Empty();

    Newstring result = Unicode::DecodeString(data, fileSize, encoding, allocator);

    return result;
}

bool WriteAllText(const Newstring& fileName, const Newstring& text, Encoding encoding)
{
    encoding = NormalizeEncoding(encoding);
    if (encoding == Encoding::Unknown)
        return false;

    uint32_t nsize;
    void* data = Unicode::EncodeString(text, &nsize, encoding);
    if (!data)  return false;
    
    bool result = WriteFileContents(fileName, data, nsize);

    g_standardAllocator.Deallocate(data);

    return result;
}

void GetApplicationDirectory(NewstringBuilder* sb)
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

void TruncateFileNameToDirectory(Newstring* fileName)
{
    if (Newstring::IsNullOrEmpty(fileName))
        return;

    int backSlash    = fileName->LastIndexOf(L'\\');
    int forwardSlash = fileName->LastIndexOf(L'/');

    int slash = std::max(backSlash, forwardSlash);

    if (slash != -1)
    {
        fileName->count = slash + 1;
    }
}

bool FileExists(const Newstring& fileName)
{
    Newstring actualFileName = MaybeReallocAsZeroTerminated(fileName);

    DWORD attrs = GetFileAttributesW(actualFileName.data);
    bool result = attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);

    return result;
}

bool DirectoryExists(const Newstring& fileName)
{
    Newstring actualFileName = MaybeReallocAsZeroTerminated(fileName);

    DWORD attrs = GetFileAttributesW(actualFileName.data);
    bool result = attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);

    return result;
}

Encoding NormalizeEncoding(Encoding encoding)
{
    if (encoding < (Encoding)0 || encoding >= Encoding::MAX_VALUE)
        return Encoding::Unknown;
    return encoding;
}

} // namespace OSUtils