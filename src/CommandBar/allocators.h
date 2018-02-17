#pragma once
#include <allocators>
#include <assert.h>
#include <new>

#include "common.h"

struct IAllocator
{
	virtual void* alloc(uintptr_t size) = 0;
	virtual void  dealloc(void* block) = 0;
    virtual void* realloc(void* block, uintptr_t size) = 0;
};

struct StandardAllocator : public IAllocator
{
    uintptr_t allocated = 0;

	virtual void* alloc(uintptr_t size) override;
	virtual void  dealloc(void* block) override;
    virtual void* realloc(void* block, uintptr_t size) override;
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

	virtual void* alloc(uintptr_t size) override;
	virtual void  dealloc(void* ptr) override;
    virtual void* realloc(void* block, uintptr_t size) override;

	void clear();
	void dispose();
	uintptr_t size() const { return (uintptr_t)end - (uintptr_t)start; }
private:

	void clearUnfits();
	void addUnfit(UnfitAllocation* unfit);
};

extern StandardAllocator g_standardAllocator;
extern TempAllocator g_tempAllocator;

template<typename T>
static inline T* stdNew()
{
    T* obj = (T*)g_standardAllocator.alloc(sizeof(T));
    if (obj) new (obj) T;

    return obj;
}

template<typename T>
static inline T* stdNewArr(int count)
{
    assert(count > 0);

    T* obj = (T*)g_standardAllocator.alloc(sizeof(T) * count);
    if (obj == nullptr) return nullptr;
    
    for (int i = 0; i < count; ++i)
        new (&obj[i]) T;

    return obj;
}
