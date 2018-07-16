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
	bool SetSize(uintptr_t size);

	virtual void* Allocate(uintptr_t size) override;
	virtual void  Deallocate(void* ptr) override;
    virtual void* Reallocate(void* block, uintptr_t size) override;

	void Reset();
	void Dispose();
private:
    struct UnfitAllocation
    {
        UnfitAllocation* next = nullptr;
    };

    void* current = nullptr;
    void* start = nullptr;
    void* end = nullptr;
    UnfitAllocation* firstUnfit = nullptr;
    UnfitAllocation* lastUnfit  = nullptr;

	void ClearUnfits();
	void AddUnfit(UnfitAllocation* unfit);
};

extern StandardAllocator g_standardAllocator;
extern TempAllocator g_tempAllocator;

void* operator new(size_t size, IAllocator* allocator);
void  operator delete(void* block, IAllocator* allocator);

#define Memnew(m_class, ...) (new (&g_standardAllocator) m_class(__VA_ARGS__))
#define MemnewAllocator(m_class, m_allocator, ...) (new (m_allocator) m_class(__VA_ARGS__))

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