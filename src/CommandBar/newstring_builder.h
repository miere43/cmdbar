#pragma once
#include "newstring.h"


/**
 * Represents resizeable string.
 */
struct NewstringBuilder
{
    union
    {
        /**
         * Represents string builder data as Newstring instance.
         */
        Newstring string;
        struct
        {
            /**
             * String data.
             */
            wchar_t* data;
            /**
             * Number of characters stored in string.
             */
            uint32_t count;
        };
    };

    /**
     * Capacity of this string.
     */
    uint32_t capacity;

    /**
     * Allocator that was used to allocate string.
     */
    IAllocator* allocator;

    /**
     * Initializes instance of string builder.
     */
    NewstringBuilder();

    /**
     * Deallocates string data and resets string builder state to default.
     */
    void Dispose();

    /**
     * Appends character to string builder.
     * If failed to reserve storage for specified character, then does nothing.
     */
    void Append(wchar_t c);

    /**
     * Appends c-string to string builder.
     * If failed to reserve storage for specified string, then does nothing.
     */
    void Append(const wchar_t* string);

    /**
     * Appends specified string with length to string builder.
     * If failed to reserve storage for specified string, then does nothing.
     */
    void Append(const wchar_t* string, uint32_t count);

    /**
     * Appends specified Newstring to string builder.
     * If failed to reserve storage for specified string, then does nothing.
     */
    void Append(const Newstring& string);

    /**
     * If string builder string data is not zero-terminated, appends "\0" character to string builder.
     * If failed to reserve storage for terminating zero, then modifies last character to terminating zero.
     */
    void ZeroTerminate();

    /**
     * Inserts string at specified position in string builder.
     * If position is out of range, then string is appended to the end of string builder.
     * If failed to reserve storage for specified string, then does nothing.
     */
    void Insert(uint32_t pos, const Newstring& string);

    /**
     * Inserts character at specified position in string builder.
     * If position is out of range, then character is appended to the end of string builder.
     * If failed to reserve storage for specified character, then does nothing.
     */
    void Insert(uint32_t pos, wchar_t c);

    /**
     * Removes specified amount of characters from string builder starting at specified position.
     * If position is out of range, then does nothing.
     * If count is too big, does not remove characters past maximum string length.
     */
    void Remove(uint32_t pos, uint32_t count);

    /**
     * Returns remaining capacity of string builder.
     * If string data is not allocated, returns 0.
     */
    uint32_t GetRemainingCapacity() const;

    /**
     * Reserves specified amount of characters to string builder.
     * In case of error, returns false and data is not modified and capacity is not reserved.
     */
    bool Reserve(uint32_t newCapacity);

    /**
     * Transfers string data to Newstring, disposing this instance of string builder.
     * Resulting string must be deallocated using string builder's allocator.
     */
    Newstring TransferToString();
};