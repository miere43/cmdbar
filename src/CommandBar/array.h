#pragma once
#include <assert.h>
#include <stdint.h>

#include "common.h"
#include "allocators.h"


/**
 * Represents resizeable array.
 */
template<typename T>
struct Array
{
    /**
     * Pointer to array data.
     */
	T* data = nullptr;

    /**
     * Number of elements in use.
     */
	uint32_t count = 0;

    /**
     * Number of elements this array is able to store without reallocating array data.
     */
	uint32_t capacity = 0;

    /**
     * Allocator that is used to allocate array data.
     */
	IAllocator* allocator = nullptr;

    /**
     * Initializes array with specified allocator.
     */
	Array(IAllocator* allocator = &g_standardAllocator)
	{
		this->allocator = allocator;
	}

    /**
     * Initializes array with specified initial capacity and allocator.
     */
	Array(uint32_t initialCapacity, IAllocator* allocator = &g_standardAllocator)
	{
		this->allocator = allocator;

        if (initialCapacity > 0)
            SetCapacity(initialCapacity);
	}

    /**
     * Reserves specified amount of elements to store in this array.
     * Returns true if specified capacity is successfully reserved.
     */
	bool Reserve(uint32_t newCapacity)
	{
		if (capacity < newCapacity)
		{
			int toReserve = newCapacity <= 5 ? 10 : newCapacity;
			return SetCapacity(toReserve);
		}

		return true;
	}

    /**
     * Appends specified value to the end of array.
     * Returns true if specified element was added to array.
     */
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

    /**
     * Appends elements from specified array to the end of this array.
     * Returns true if specified elements were successfully added to array.
     */
	bool AppendRange(const T* values, uint32_t count)
	{
		if (values == nullptr || count == 0)
			return true;

		if (!Reserve(this->count + count))
			return false;

		for (uint32_t i = 0; i < count; ++i)
			data[this->count++] = values[i];

		return true;
	}

    /**
     * Resets array element count to zero.
     */
	void Clear()
	{
		count = 0;
	}

    /**
     * Disposes allocated data of this array and resets array state.
     */
	void Dispose()
	{
		if (data != nullptr)
        {
			allocator->Deallocate(data);
			data = nullptr;
		}
	
		count = 0;
		capacity = 0;
	}

private:
    /**
     * Sets capacity of current array to specified value.
     * If changing capacity is failed, returns false.
     */
	bool SetCapacity(uint32_t newCapacity)
    {
        const uintptr_t newSize = sizeof(T) * newCapacity;
        T* newData = (T*)allocator->Reallocate(data, newSize);
        if (!newData)
            return false;

        data = newData;
        capacity = newCapacity;

        return true;
	}
};