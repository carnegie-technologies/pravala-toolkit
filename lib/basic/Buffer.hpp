/*
 *  Copyright 2019 Carnegie Technologies
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#pragma once

#include "String.hpp"

namespace Pravala
{
class MemHandle;
struct MemBlock;
struct MemData;

/// @brief Represents a buffer that can be appended to.
/// It uses implicit sharing, so the data is not copied unless it's necessary.
/// @note This class is not completely thread safe. Having different threads accessing the same memory block
///        using their own copies of MemHandle/Buffer is safe. However, multiple threads accessing (including creating
///        copies) of the same MemHandle/Buffer is NOT safe. Whenever memory is to be accessed by multiple threads,
///        a new copy of MemHandle/Buffer should be created (on the same thread) for each of the threads.
class Buffer
{
    public:
        /// @brief Default constructor
        inline Buffer(): _data ( 0 ), _allocSize ( 0 ), _size ( 0 )
        {
        }

        /// @brief Constructor.
        /// @param [in] preAllocateMemory The amount of memory that should be preallocated in the buffer.
        ///                                If this size is sufficient for future appends it will improve
        ///                                the memory usage and performance. This memory will not be marked as used.
        Buffer ( size_t preAllocateMemory );

        /// @brief A copy constructor
        ///
        /// It does not actually copy the data, it simply attaches itself to the underlying data in RW mode
        /// (so if anything else tries to modify the memory it will create its own copy first)
        ///
        /// @param [in] other The object to create a copy of
        Buffer ( const Buffer & other );

        /// @brief An assignment operator
        ///
        /// It does not actually copy the data, it simply attaches itself to the underlying data in RW mode
        /// (so if anything else tries to modify the memory it will create its own copy first)
        ///
        /// @param [in] other The object to create a copy of
        /// @return A reference to this object.
        Buffer & operator= ( const Buffer & other );

        /// @brief Destructor
        inline ~Buffer()
        {
            clear();
        }

        /// @brief Clears the buffer
        void clear();

        /// @brief Returns the (used) size of the buffer
        /// @return Size of data stored = length of data that could be read starting from pointer returned by get(0)
        inline size_t size() const
        {
            return _size;
        }

        /// @brief Checks whether the buffer is empty or not.
        ///
        /// @return True if the buffer is empty, false otherwise.
        inline bool isEmpty() const
        {
            return ( _size < 1 );
        }

        /// @brief Returns content of the buffer as a new String
        ///
        /// @return Content of the buffer as a new String
        inline String toString() const
        {
            if ( _size < 1 )
                return String::EmptyString;

            return String ( get(), ( int ) _size );
        }

        /// @brief Splits the content of the buffer into a list of strings wherever one of separators occurs
        ///
        /// This function splits the content of the buffer into a list of strings
        /// wherever one of characters in 'separators' string occurs.
        /// @param [in] separators Separators to be used. Each character in this string
        ///             is treated as a separate separator.
        /// @param [in] keepEmpty Should empty strings be included in the list
        /// @return Generated list of strings.
        inline StringList toStringList ( const String & separators, bool keepEmpty = false ) const
        {
            return String::toStringList ( get(), size(), separators, keepEmpty );
        }

        /// @brief Returns a pointer to the data stored in the buffer
        ///
        /// @param [in] offset Offset within the buffer
        /// @return Pointer to the data stored in the buffer, or 0 if it is empty (or the offset is incorrect)
        const char * get ( size_t offset = 0 ) const;

        /// @brief Creates a read-only handle to buffer's memory
        ///
        /// It does not copy the data, it attaches itself to the buffer in RO mode
        /// If the offset is incorrect, an empty object handle will be created
        ///
        /// @param [in] offset The offset in buffer's memory this handle should start at. The range will
        ///                     start at the given offset, and contain the rest of the buffer.
        /// @return Created MemHandle
        MemHandle getHandle ( size_t offset = 0 ) const;

        /// @brief Creates a read-only handle to buffer's memory
        ///
        /// It does not copy the data, it attaches itself to the buffer in RO mode
        /// If the offset is incorrect (or the size is 0), an empty object handle will be created
        ///
        /// @param [in] offset The offset in buffer's memory this handle should start at.
        /// @param [in] size The size of the memory range. If the size is larger than buffer's size,
        ///                   the created MemHandle will be smaller than requested.
        /// @return Created MemHandle
        MemHandle getHandle ( size_t offset, size_t size ) const;

        /// @brief Returns a writable pointer to the beginning of free data at the end of the buffer
        ///
        /// If needed, more memory is allocated.
        ///
        /// If the same memory is used by anything else (another Buffer, RwBuffer, or MemHandle),
        /// a copy of the memory is created, and this instance is "unattached" from the original memory
        /// - so existing users will not see any changes.
        ///
        /// @param [in] count Size of the memory requested
        /// @return Pointer to the first byte of the free memory segment of size count.
        char * getAppendable ( size_t count );

        /// @brief Marks the memory returned by getAppendable function as used
        /// @param [in] count Size of the memory to be marked as used. It has to be valid!
        void markAppended ( size_t count );

        /// @brief Appends data to the buffer
        /// @param [in] buf Pointer to the data to be appended. It has to be valid, unless count is 0
        /// @param [in] count Size of the data to be appended
        void appendData ( const char * data, size_t count );

        /// @brief Appends a C string to the buffer
        /// This does not append the terminating null
        /// @param [in] str Pointer to the C-style (null-terminated) string to be appended
        void append ( const char * str );

        /// @brief Appends a String to the buffer
        /// This does not append the terminating null
        /// @param [in] str String to be appended
        void append ( const String & str );

        /// @brief Appends the content of another buffer to this one
        /// @param [in] memHandle The data to be appended. If this buffer is still empty, and that MemHandle
        ///                        has the right offset and size, the data is simply referenced.
        void append ( const MemHandle & memHandle );

        /// @brief Appends the content of another buffer to this one
        /// @param [in] buf The buffer to be appended. If this buffer is still empty, the data is simply referenced.
        void append ( const Buffer & buf );

        /// @brief Writes content of the buffer to a file
        /// @param [in] filePath The path of the file to write to
        /// @param [in] appendToFile If set to true, the data will be appended to the file. Otherwise the content
        ///                     of the file (if any) is overwritten (this is the default).
        /// @return True if the operation was successful, false otherwise
        bool writeToFile ( const char * filePath, bool appendToFile = false );

        /// @brief Writes content of the buffer to a file
        /// @param [in] filePath The path of the file to write to
        /// @param [in] appendToFile If set to true, the data will be appended to the file. Otherwise the content
        ///                     of the file (if any) is overwritten (this is the default).
        /// @return True if the operation was successful, false otherwise
        bool writeToFile ( const String & filePath, bool appendToFile = false );

    protected:
        MemBlock * _data; ///< Buffer's internal memory.
        size_t _allocSize; ///< The size of the allocated memory. DOES include include the MemBlock.
        size_t _size; ///< The size of the data stored in the buffer. Does NOT include the MemBlock.

        /// @brief A helper function that exposes internal memory as a MemData object.
        /// That object will NOT contain an additional reference for the caller to use.
        /// If a copy is stored, the caller needs to create its own reference.
        /// @return Internal memory as a MemData object.
        MemData getMemData() const;

        friend class MemHandle;
};

/// @brief Represents a buffer whose existing memory can be modified (in addition to appending data)
/// It uses implicit sharing, so the data is not copied unless it's necessary
class RwBuffer: public Buffer
{
    public:
        /// @brief Default constructor
        inline RwBuffer()
        {
        }

        /// @brief Constructor
        /// @param [in] preAllocateMemory The amount of memory that should be preallocated in the buffer.
        ///                                If this size is sufficient for future appends it will improve
        ///                                the memory usage and performance. This memory will not be marked as used.
        inline RwBuffer ( size_t preAllocateMemory ): Buffer ( preAllocateMemory )
        {
        }

        /// @brief A copy constructor
        /// @param [in] other The object to create a copy of (an instance of Buffer, does not have to be an RwBuffer)
        inline RwBuffer ( const Buffer & other ): Buffer ( other )
        {
        }

        /// @brief An assignment operator
        /// @param [in] other The object to create a copy of (an instance of Buffer, does not have to be an RwBuffer)
        /// @return A reference to this object.
        inline RwBuffer & operator= ( const Buffer & other )
        {
            Buffer::operator= ( other );
            return *this;
        }

        /// @brief Returns a writable pointer to the underlying memory
        ///
        /// Same as get(), but memory can be modified. Also, if anything else uses the same memory,
        /// either in RO or RW mode, this object is unattached from that memory and creates its own copy.
        ///
        /// @param [in] offset Offset within the buffer
        /// @return Pointer to the data stored in the buffer, or 0 if it is empty (or the offset is incorrect)
        char * getWritable ( size_t offset = 0 );

        /// @brief "Consumes" some amount of data, by discarding 'size' bytes of data from the beginning of the buffer.
        /// @param [in] consumeSize The number of bytes to discard. If it is 0, nothing is done.
        ///                          If it is equal to or greater than the available size, the buffer is cleared.
        void consumeData ( size_t consumeSize );

        /// @brief Truncates the buffer to only include given size of data
        /// If truncatedSize is equal to or larger than the current size of regular data, nothing is changed.
        /// If the underlying data is used by anybody else in RW mode, a copy of the data is created.
        /// @param [in] truncatedSize The new size of regular data
        void truncateData ( size_t truncatedSize );
};
}
