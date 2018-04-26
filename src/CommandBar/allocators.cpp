#include "allocators.h"
#include <allocators>


StandardAllocator g_standardAllocator;
TempAllocator g_tempAllocator;


inline uintptr_t bytesRequiredToAlignPointer(void* pointer, uintptr_t alignment)
{
	return (uintptr_t)pointer % alignment;
}

inline void* AlignPointer(void* pointer, uintptr_t alignment)
{
	return (void*)((uintptr_t)pointer + ((uintptr_t)pointer % alignment));
}

inline void* addBytesToPointer(void* pointer, uintptr_t size)
{
	return (void*)((uintptr_t)pointer + size);
}

bool TempAllocator::SetSize(uintptr_t size)
{
	Dispose();

	if (size == 0)
		return true;

	start = ::malloc(size);
	if (start == nullptr)
    {
        SetLastError(ERROR_OUTOFMEMORY);
		return false;
    }

	current = start;
	end = addBytesToPointer(start, size);

	return true;
}

void* TempAllocator::Allocate(uintptr_t size)
{
    assert(start);
    assert(current);

	uintptr_t requiredAlignment = bytesRequiredToAlignPointer(current, sizeof(void*));
	void* alignedPointer = addBytesToPointer(current, requiredAlignment);
	if ((uintptr_t)alignedPointer + size > (uintptr_t)end)
	{
        // @TODO: this thing doesn't respect alignment?
		UnfitAllocation* unfit = (UnfitAllocation*)::malloc(sizeof(UnfitAllocation) + size + sizeof(void*));

		if (unfit == nullptr)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return nullptr;
        }

        unfit->next = nullptr;
		AddUnfit(unfit);

		return AlignPointer((void*)((uintptr_t)unfit + sizeof(UnfitAllocation)), sizeof(void*));
	}
	else
	{
		current = (void*)((uintptr_t)current + size + (uintptr_t)requiredAlignment);
		return alignedPointer;
	}
}

void TempAllocator::Deallocate(void* ptr)
{
	// I don't care :P
}

void* TempAllocator::Reallocate(void* block, uintptr_t size)
{
    assert(false);
    return nullptr;
}

void TempAllocator::Reset()
{
	if (start != nullptr)
		current = start;
    ClearUnfits();
}

void TempAllocator::Dispose()
{
	Reset();

    free(start);
	start = current = end = nullptr;
}

void TempAllocator::ClearUnfits()
{
	UnfitAllocation* current = unfit;

	while (current != nullptr)
	{
		UnfitAllocation* next = current->next;
        free(current);
		current = next;
	}

	unfit = nullptr;
}

void TempAllocator::AddUnfit(UnfitAllocation* newUnfit)
{
	if (unfit == nullptr)
	{
		unfit = newUnfit;
	}
	else
	{
		UnfitAllocation* alloc = unfit;
		while (alloc->next) continue;
		alloc->next = unfit;
	}
}

void* StandardAllocator::Allocate(uintptr_t size)
{
    void* block = ::malloc(size);
    if (block == nullptr)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return nullptr;
    }

    allocated += size;
    return block;
}

void StandardAllocator::Deallocate(void* block)
{
    if (block == nullptr) return;

    uintptr_t blockSize = static_cast<uintptr_t>(_msize(block));
    assert(allocated >= blockSize);
    allocated -= blockSize;

    ::free(block);
}

void* StandardAllocator::Reallocate(void* block, uintptr_t size)
{
    uintptr_t oldSize = block ? static_cast<uintptr_t>(_msize(block)) : 0;
    block = ::realloc(block, size);
    if (block == nullptr)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return nullptr;
    }
    uintptr_t newSize = static_cast<uintptr_t>(_msize(block));

    allocated = allocated - oldSize + newSize;
    return block;
}

void* operator new(size_t size, IAllocator* allocator)
{
    assert(allocator);

    return allocator->Allocate(size);
}

void operator delete(void* block, IAllocator* allocator)
{
    assert(allocator);

    allocator->Deallocate(block);
}
