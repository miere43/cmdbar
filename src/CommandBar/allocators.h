#pragma once
#include "common.h"
#include <allocators>

struct IAllocator
{
	virtual void* allocate(uintptr_t size, uintptr_t alignment = sizeof(void*))   = 0;
	virtual void  deallocate(void* ptr) = 0;
};

struct StandardAllocator : public IAllocator
{
	virtual void* allocate(uintptr_t size, uintptr_t alignment = sizeof(void*)) override;
	virtual void  deallocate(void * ptr) override;
};

struct TempAllocator : public IAllocator
{
	struct UnfitAllocation
	{
		UnfitAllocation* next = nullptr;
	};

	void* current = nullptr;
	void* start = nullptr;
	void* end = nullptr;
	UnfitAllocation* unfit = nullptr;

	bool setSize(uintptr_t size);

	virtual void* allocate(uintptr_t size, uintptr_t alignment = sizeof(void*))   override;
	virtual void  deallocate(void* ptr) override;

	void clear();
	void dispose();
	uintptr_t size() const { return (uintptr_t)end - (uintptr_t)start; }
private:

	void clearUnfits();
	void addUnfit(UnfitAllocation* unfit);
};

extern StandardAllocator g_standardAllocator;
extern TempAllocator g_tempAllocator;
