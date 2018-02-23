#pragma once
#include <allocators>
#include <assert.h>
#include <new>

#include "common.h"

struct IAllocator
{
	virtual void* Allocate(uintptr_t size) = 0;
	virtual void  Deallocate(void* block) = 0;
    virtual void* Reallocate(void* block, uintptr_t size) = 0;
};

struct StandardAllocator : public IAllocator
{
    uintptr_t allocated = 0;

	virtual void* Allocate(uintptr_t size) override;
	virtual void  Deallocate(void* block) override;
    virtual void* Reallocate(void* block, uintptr_t size) override;
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

	bool SetSize(uintptr_t size);

	virtual void* Allocate(uintptr_t size) override;
	virtual void  Deallocate(void* ptr) override;
    virtual void* Reallocate(void* block, uintptr_t size) override;

	void Reset();
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
    T* obj = (T*)g_standardAllocator.Allocate(sizeof(T));
    if (obj) new (obj) T;

    return obj;
}

template<typename T>
static inline T* stdNewArr(int count)
{
    assert(count > 0);

    T* obj = (T*)g_standardAllocator.Allocate(sizeof(T) * count);
    if (obj == nullptr) return nullptr;
    
    for (int i = 0; i < count; ++i)
        new (&obj[i]) T;

    return obj;
}

void* operator new(size_t size, IAllocator* allocator);
void  operator delete(void* block, IAllocator* allocator);

#define Memnew(m_class) (new (&g_standardAllocator) m_class)
#define MemnewAllocator(m_class, m_allocator) (new (m_allocator) m_class)

template<typename T>
void Memdelete(T* object)
{
    if (!object)  return;
    object->~T();
    g_standardAllocator.Deallocate(object);
}

template<typename T>
void MemdeleteAllocator(T* object, IAllocator* allocator)
{
    if (!object)  return;
    object->~T();
    allocator->Deallocate(object);
}

//#define MemnewArray(m_class, m_count) TemplateMemnewArray<m_class>(m_count, &g_standardAllocator)
//#define MemnewArrayAllocator(m_class, m_count, m_allocator) TemplateMemnewArray<m_class>(m_count, m_allocator)
//
//template<typename T>
//T* TemplateMemnewArray(size_t count, IAllocator* allocator)
//{
//    assert(allocator);
//    if (count == 0)  return nullptr;
//
//    size_t memCount = count * sizeof(T);
//    T* memory = (T*)allocator->Allocate(memCount);
//    if (!memory)  return nullptr;
//
//    for (size_t i = 0; i < count; ++i)
//    {
//        new (&memory[i], allocator);
//    }
//
//    return memory;
//}
//
//template<typename T>
//void MemdeleteArray(T* object)
//{
//    if (!object)  return;
//
//}