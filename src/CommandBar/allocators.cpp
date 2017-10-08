#include "allocators.h"
#include <allocators>


StandardAllocator g_standardAllocator;
TempAllocator g_tempAllocator;


inline uintptr_t bytesRequiredToAlignPointer(void* pointer, uintptr_t alignment)
{
	return (uintptr_t)pointer % alignment;
}

inline void* alignPointer(void* pointer, uintptr_t alignment)
{
	return (void*)((uintptr_t)pointer + ((uintptr_t)pointer % alignment));
}

inline void* addBytesToPointer(void* pointer, uintptr_t size)
{
	return (void*)((uintptr_t)pointer + size);
}

bool TempAllocator::setSize(uintptr_t size)
{
	dispose();

	if (size == 0)
		return true;

	start = operator new(size);
	if (start == nullptr)
		return false;

	current = start;
	end = addBytesToPointer(start, size);

	return true;
}

void * TempAllocator::allocate(uintptr_t size, uintptr_t alignment)
{
	if (start == nullptr)
		return nullptr;

	uintptr_t requiredAlignment = bytesRequiredToAlignPointer(current, alignment);
	void* alignedPointer = addBytesToPointer(current, requiredAlignment);
	if ((uintptr_t)alignedPointer + size > (uintptr_t)end)
	{
		UnfitAllocation* unfit = (UnfitAllocation*)operator new(sizeof(UnfitAllocation) + size + alignment);

		if (unfit == nullptr)
			return nullptr;

		addUnfit(unfit);

		return alignPointer(unfit + sizeof(UnfitAllocation), alignment); // @Leak
	}
	else
	{
		current = (void*)((uintptr_t)current + size + (uintptr_t)requiredAlignment);
		return alignedPointer;
	}
}

void TempAllocator::deallocate(void * ptr)
{
	// I don't care :P
}

void TempAllocator::clear()
{
	if (start != nullptr)
		current = start;
}

void TempAllocator::dispose()
{
	clear();

	if (start != nullptr)
		operator delete(start);
	start = current = end = nullptr;
}

void TempAllocator::clearUnfits()
{
	UnfitAllocation* current = unfit;

	while (current != nullptr)
	{
		UnfitAllocation* next = current->next;
		operator delete(current);
		current = next;
	}

	unfit = nullptr;
}

void TempAllocator::addUnfit(UnfitAllocation * unfit)
{
	if (unfit == nullptr)
	{
		unfit = unfit;
	}
	else
	{
		UnfitAllocation* alloc = unfit;
		while (alloc->next) continue;
		alloc->next = unfit;
	}
}

void * StandardAllocator::allocate(uintptr_t size, uintptr_t alignment)
{
	try
	{
		void* p = operator new(size);
		return p;
	}
	catch (std::bad_alloc)
	{
		return nullptr;
	}
	return nullptr;
}

void StandardAllocator::deallocate(void * ptr)
{
	if (ptr == nullptr)
		return;

	operator delete(ptr);
}
