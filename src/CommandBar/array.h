#pragma once
#include <assert.h>
#include <stdint.h>

#include "common.h"
#include "allocators.h"


template<typename T>
struct Array {
	T*  data     = nullptr;
	uint32_t count    = 0;
	uint32_t capacity = 0;
	IAllocator* allocator = nullptr;

	Array(IAllocator* allocator = &g_standardAllocator)
	{
		this->allocator = allocator;
	}

	Array(uint32_t initialCapacity, IAllocator* allocator = &g_standardAllocator)
	{
		this->allocator = allocator;
		SetCapacity(initialCapacity);
	}

	bool Reserve(uint32_t newCapacity)
	{
		if (capacity < newCapacity)
		{
			int toReserve = newCapacity <= 5 ? 10 : newCapacity;
			return SetCapacity(toReserve);
		}

		return true;
	}

	bool Append(const T& value)
	{
		if (!Reserve(count + 1))
			return false;

		data[count++] = value;

		return true;
	}

    /**
     * Removes single element at specified index. 
     * If data is not allocated or index is out of range, then does nothing.
     */
    void Remove(uint32_t index)
    {
        if (data == nullptr || index > count)  return;
        
        if (index == count - 1)
        {
            --count;
        }
        else
        {
            memcpy(&data[index], &data[index + 1], sizeof(T) * (count - 1 - index));
            --count;
        }
    }

	// Adds all elements from array 'values'.
	// Returns false if no items were added to this resizeable array or if there is nothing to add (values == nullptr or count == 0).
	bool AppendRange(const T* values, uint32_t count)
	{
		if (values == nullptr || count <= 0)
			return true;

		if (!Reserve(this->count + count))
			return false;

		for (uint32_t i = 0; i < count; ++i)
			data[this->count++] = values[i];

		return true;
	}

	void Clear()
	{
		count = 0;
	}

	void Dispose()
	{
		if (data != nullptr) {
			allocator->Deallocate(data);
			data = nullptr;
		}
	
		count = 0;
		capacity = 0;
	}

	bool SetCapacity(uint32_t newCapacity)
	{
        // @TODO: Use Reallocate method in allocator.

		if (data == nullptr)
		{
			data = static_cast<T*>(allocator->Allocate(sizeof(T) * newCapacity));
			if (data == nullptr)
				return false;
			
			capacity = newCapacity;
			count = 0;

			return true;
		}
		else
		{
			T* newData = static_cast<T*>(allocator->Allocate(sizeof(T) * newCapacity));
			if (newData == nullptr)
				return false;

			if (data != nullptr && count > 0) {
				// If newCapacity < count, this wouldn't work.
				memcpy_s(newData, sizeof(T) * newCapacity, data, sizeof(T) * count);
				allocator->Deallocate(data);
			}
			
			capacity = newCapacity;
			data = newData;

			return true;
		}
	}
};