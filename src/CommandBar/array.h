#pragma once
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

	Array(int initialCapacity, IAllocator* allocator = &g_standardAllocator)
	{
		this->allocator = allocator;
		setCapacity(initialCapacity);
	}

	bool reserve(uint32_t newCapacity)
	{
		if (capacity < newCapacity)
		{
			uint32_t toReserve = newCapacity <= 5 ? 10 : newCapacity;
			return setCapacity(toReserve);
		}

		return true;
	}

	bool add(const T& value)
	{
		if (!reserve(count + 1))
			return false;

		data[count++] = value;

		return true;
	}

	/// Adds all elements from array 'values'.
	/// Returns false if no items were added to this resizeable array or if there is nothing to add (values == nullptr or count == 0).
	bool addRange(const T* values, uint32_t count)
	{
		if (values == nullptr || count == 0)
			return true;

		if (!reserve(this->count + count))
			return false;

		for (uint32_t i = 0; i < count; ++i)
			data[this->count++] = values[i];

		return true;
	}

	void clear()
	{
		count = 0;
	}

	void free()
	{
		if (data != nullptr) {
			allocator->deallocate(data);
			data = nullptr;
		}
	
		count = 0;
		capacity = 0;
	}

	bool setCapacity(uint32_t newCapacity)
	{
		if (data == nullptr)
		{
			data = (T*)allocator->allocate(sizeof(T) * newCapacity);
			if (data == nullptr)
				return false;
			
			capacity = newCapacity;
			count = 0;

			return true;
		}
		else
		{
			T* newData = (T*)allocator->allocate(sizeof(T) * newCapacity);
			if (newData == nullptr)
				return false;

			if (data != nullptr && count > 0) {
				// If newCapacity < count, this wouldn't work.
				memcpy_s(newData, sizeof(T) * newCapacity, data, sizeof(T) * count);
				allocator->deallocate(data);
			}
			
			capacity = newCapacity;
			data = newData;

			return true;
		}
	}
};