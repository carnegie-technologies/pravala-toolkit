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
#include "internal/MemBlock.hpp"
#include "internal/MemData.hpp"

namespace Pravala
{
class Buffer;

/// @brief A handle to a block of memory.
/// A single memory block can have multiple handles attached to it.
/// Whenever a handle that shares the memory block with other handles wants to modify it (getWritable() is called),
/// it creates a copy of the memory (copy on write). As long as the operations are read-only (which also includes
/// creating new handles and changing the size and offset of handles) the memory is not copied.
/// @note This class is not completely thread safe. Having different threads accessing the same memory block
///        using their own copies of MemHandle/Buffer is safe. However, multiple threads accessing (including creating
///        copies) of the same MemHandle/Buffer is NOT safe. Whenever memory is to be accessed by multiple threads,
///        a new copy of MemHandle/Buffer should be created (on the same thread) for each of the threads.
class MemHandle
{
    public:
        /// @brief An empty MemHandle object.
        static const MemHandle EmptyHandle;

        /// @brief Default constructor
        /// It creates an empty MemHandle, that is not associated with any memory
        inline MemHandle()
        {
        }

        /// @brief Constructor.
        /// Creates a MemHandle and allocates its memory.
        /// That memory will not be initialized (so it will contain unspecified data).
        /// Created MemHandle will be "full" - its size will be the same as requested.
        /// If memory cannot be allocated, MemHandle will be empty.
        /// @param [in] dataSize The number of bytes to allocate.
        explicit MemHandle ( size_t dataSize );

        /// @brief Constructor.
        /// Creates a MemHandle and reads its content from a file.
        /// If memory cannot be allocated, or the file cannot be read, MemHandle will be empty.
        /// @param [in] filePath The path of the file to read from
        /// @param [in] ok If used, the error flag value is placed there.
        explicit MemHandle ( const char * filePath, bool * ok = 0 );

        /// @brief Constructor.
        /// Creates a MemHandle and reads its content from a file.
        /// If memory cannot be allocated, or the file cannot be read, MemHandle will be empty.
        /// @param [in] filePath The path of the file to read from
        /// @param [in] ok If used, the error flag value is placed there.
        explicit MemHandle ( const String & filePath, bool * ok = 0 );

        /// @brief Constructor
        ///
        /// It does not copy the data, it attaches itself to the buffer in RO mode
        /// If the parameters are incorrect, an empty object handle will be created.
        ///
        /// @param [in] buffer The buffer to attach this MemHandle to
        MemHandle ( const Buffer & buffer );

        /// @brief A copy constructor
        ///
        /// It does not copy the data, it attaches itself to the same buffer as the other MemHandle.
        /// It also has the same offset and size as the other MemHandle.
        ///
        /// @param [in] other The object to create a copy of
        MemHandle ( const MemHandle & other );

        /// @brief An assignment operator
        ///
        /// It does not copy the data, it attaches itself to the same buffer as the other MemHandle.
        /// It also has the same offset and size as the other MemHandle. It is equivalent to calling getHandle ( 0 )
        ///
        /// @param [in] other The object to create a copy of
        /// @return A reference to this object.
        MemHandle & operator= ( const MemHandle & other );

        /// @brief An assignment operator
        ///
        /// It does not copy the data, it attaches itself to the same buffer as the other MemHandle.
        /// It also has the same offset and size as the other MemHandle. It is equivalent to calling getHandle ( 0 )
        ///
        /// @param [in] buffer The buffer to attach this MemHandle to (instead of the current attachment)
        /// @return A reference to this object.
        MemHandle & operator= ( const Buffer & buffer );

        /// @brief Destructor
        ~MemHandle();

        /// @brief Clears the MemHandle.
        /// Detaches it from the underlying buffer and makes it empty.
        void clear();

        /// @brief Creates a new MemHandle, starting at the given offset within this MemHandle's memory
        /// The memory range contains all the memory until the end of this MemHandle's range
        /// If the offset is incorrect, an empty memory handle will be created
        /// @param [in] offset The offset within this MemHandle's memory
        /// @return The new MemHandle
        MemHandle getHandle ( size_t offset ) const;

        /// @brief Creates a new MemHandle, starting at the given offset within this MemHandle's memory and given size
        /// If the offset is incorrect, or the size is 0, an empty memory handle will be created.
        /// However, if size is greater than max possible size (for this MemHandle, not the buffer itself),
        /// the created MemHandle will be smaller than requested.
        /// @param [in] offset The offset within this MemHandle's memory
        /// @param [in] size The size of the new MemHandle
        /// @return The new MemHandle
        MemHandle getHandle ( size_t offset, size_t size ) const;

        /// @brief Shrinks the MemHandle removing bytes from the end of the MemHandle.
        /// @param [in] newSize The new size of this handle. If it is greater than the current size, nothing happens.
        /// @return True if the MemHandle is usable (not empty) after this operation; False otherwise.
        bool truncate ( size_t newSize );

        /// @brief Shrinks the MemHandle, removing bytes from the beginning of the MemHandle.
        /// @param [in] numBytes The number of bytes in the front of the memory to remove.
        ///                       If it is equal or greater than the current size, MemHandle becomes empty.
        /// @return True if the MemHandle is usable (not empty) after this operation; False otherwise.
        bool consume ( size_t numBytes );

        /// @brief Returns the size of the memory range represented by this handle
        /// @return Size of data stored = length of data that could be read starting from pointer returned by get(0)
        inline size_t size() const
        {
            return _data.size;
        }

        /// @brief Checks whether the handle is empty or not.
        /// @return True if the handle is empty, false otherwise.
        inline bool isEmpty() const
        {
            return ( _data.size < 1 );
        }

        /// @brief Returns content of the handle represented by this handle as a new String
        /// @return Content of the handle represented by this handle as a new String
        inline String toString() const
        {
            return String ( _data.mem, ( int ) _data.size );
        }

        /// @brief Splits the content of the handle into a list of strings wherever one of separators occurs
        /// This function splits the content of the handle into a list of strings
        /// wherever one of characters in 'separators' string occurs.
        /// @param [in] separators Separators to be used. Each character in this string
        ///             is treated as a separate separator.
        /// @param [in] keepEmpty Should empty strings be included in the list
        /// @return Generated list of strings.
        inline StringList toStringList ( const String & separators, bool keepEmpty = false ) const
        {
            return String::toStringList ( _data.mem, _data.size, separators, keepEmpty );
        }

        /// @brief Returns a hex dump of the data in this MemHandle.
        /// @param [in] include0x Should the '0x' be included before each value or not
        /// @param [in] separator The string used as the hex separator
        /// @param [in] useLowercase If true, lowercase letters will be used
        /// @return String with the hex dump of the data in this MemHandle.
        inline String getHexDump (
            bool include0x = true, const String & separator = " ", bool useLowercase = false ) const
        {
            return String::hexDump ( _data.mem, _data.size, include0x, separator, useLowercase );
        }

        /// @brief Returns a pointer to the memory
        /// @param [in] offset Offset within the MemHandle
        /// @return Pointer to the data represented by this handle, or 0 if it is empty (or the offset is incorrect)
        inline const char * get ( size_t offset = 0 ) const
        {
            return ( offset < _data.size ) ? ( _data.mem + offset ) : 0;
        }

        /// @brief Extracts a memory chunk from this MemHandle.
        /// @param [in,out] offset Offset within the MemHandle of the beginning of the memory to copy into.
        ///                        On success it is modified to point to the first byte after the data that was read
        ///                        (and it may be equal to MemHandle's size).
        /// @param [out] mem Pointer to memory to copy the data to.
        /// @param [in] memSize The size of the memory to copy.
        ///                     Copying 0 bytes is considered a success, unless the offset is invalid.
        /// @return True on success (if 'memSize' bytes were copied);
        ///         False if the MemHandle was too small (given the offset and memSize),
        ///         or if mem was 0 and memSize > 0. On failure no data is copied (no partial copy).
        bool getData ( size_t & offset, void * mem, size_t memSize ) const;

        /// @brief Stores a memory chunk in this MemHandle.
        /// @param [in,out] offset Offset within the MemHandle of the beginning of the memory to copy into.
        ///                        On success it is modified to point to the first byte after the data that was stored
        ///                        (and it may be equal to MemHandle's size).
        /// @param [in] mem Pointer to memory to copy the data from.
        /// @param [in] memSize The size of the memory to copy.
        ///                     Copying 0 bytes is considered a success, unless the offset is invalid.
        /// @return True on success (if 'memSize' bytes were copied);
        ///         False if the MemHandle was too small (given the offset and memSize),
        ///         or if mem was 0 and memSize > 0. On failure no data is copied (no partial copy).
        bool setData ( size_t & offset, const void * mem, size_t memSize );

        /// @brief Reads a value from the memory in this MemHandle object.
        /// @param [in,out] offset Offset within the MemHandle of the beginning of the memory to copy.
        ///                        On success it is modified to point to the first byte after the value that was read
        ///                        (and it may be equal to MemHandle's size).
        /// @param [out] value The value to read from the MemHandle.
        /// @return True on success (if value's size bytes were copied);
        ///         False if the MemHandle was too small (given the offset and value's size).
        template<typename T> inline bool getValue ( size_t & offset, T & value ) const
        {
            return getData ( offset, &value, sizeof ( value ) );
        }

        /// @brief Copies a value into memory in this MemHandle object.
        /// @param [in,out] offset Offset within the MemHandle of the beginning of the memory to copy into.
        ///                        On success it is modified to point to the first byte after the value that was stored
        ///                        (and it may be equal to MemHandle's size).
        /// @param [out] value The value to copy into the MemHandle.
        /// @return True on success (if value's size bytes were copied);
        ///         False if the MemHandle was too small (given the offset and value's size),
        ///         or it couldn't be written into.
        template<typename T> inline bool setValue ( size_t & offset, const T & value )
        {
            return setData ( offset, &value, sizeof ( value ) );
        }

        /// @brief Returns a writable pointer to the memory
        /// @param [in] offset Offset within the MemHandle
        /// @return Pointer to the data represented by this handle, or 0 if it is empty (or the offset is incorrect)
        char * getWritable ( size_t offset = 0 );

        /// @brief Writes zeros to the entire memory of this MemHandle.
        void setZero();

        /// @brief Reads the content of the MemHandle from a file.
        /// This MemHandle will release the memory previously used (doesn't matter if the read succeeded or not)
        /// and if the read succeeds MemHandle will attach to a new memory block with the content of the file.
        /// @param [in] filePath The path of the file to read from.
        /// @return True if the operation was successful; False otherwise.
        bool readFile ( const char * filePath );

        /// @brief Reads the content of the MemHandle from a file.
        /// This MemHandle will release the memory previously used (doesn't matter if the read succeeded or not)
        /// and if the read succeeds MemHandle will attach to a new memory block with the content of the file.
        /// @param [in] filePath The path of the file to read from.
        /// @return True if the operation was successful; False otherwise.
        inline bool readFile ( const String & filePath )
        {
            return readFile ( filePath.c_str() );
        }

        /// @brief Reads the content of the MemHandle from a file.
        /// This MemHandle will release the memory previously used (doesn't matter if the read succeeded or not)
        /// and if the read succeeds MemHandle will attach to a new memory block with the content of the file.
        /// @param [in] fd The file descriptor of an open file. This file will NOT be closed by the MemHandle,
        ///                 but offset in the file MAY (not necessarily) be modified.
        /// @return True if the operation was successful; False otherwise.
        bool readFile ( int fd );

        /// @brief Writes content of the buffer to a file
        /// @param [in] filePath The path of the file to write to
        /// @param [in] appendToFile If set to true, the data will be appended to the file. Otherwise the content
        ///                     of the file (if any) is overwritten (this is the default).
        /// @return True if the operation was successful, false otherwise
        inline bool writeToFile ( const char * filePath, bool appendToFile = false ) const
        {
            return writeDataToFile ( _data.mem, _data.size, filePath, appendToFile );
        }

        /// @brief Writes content of the buffer to a file
        /// @param [in] filePath The path of the file to write to
        /// @param [in] appendToFile If set to true, the data will be appended to the file. Otherwise the content
        ///                     of the file (if any) is overwritten (this is the default).
        /// @return True if the operation was successful, false otherwise
        inline bool writeToFile ( const String & filePath, bool appendToFile = false ) const
        {
            return writeDataToFile ( _data.mem, _data.size, filePath.c_str(), appendToFile );
        }

        /// @brief A function that returns the type of the memory used.
        /// @return The type of the memory used.
        inline uint8_t getMemoryType() const
        {
            return ( _data.block != 0 ) ? _data.block->getType() : static_cast<uint8_t> ( MemBlock::TypeUnknown );
        }

        /// @brief A function that returns the tag associated with underlying memory.
        /// @return The tag associated with underlying memory (see MemBlock::_tag); or 0 if there is not memory.
        inline uint8_t getMemoryTag() const
        {
            return ( _data.block != 0 ) ? _data.block->getTag() : 0;
        }

        /// @brief Returns the number of references to the underlying memory.
        /// This is mostly used for debugging.
        /// @return The number of references to the underlying memory. Could be 0, if there is no memory.
        inline uint16_t getRefCount() const
        {
            return ( _data.block != 0 ) ? _data.block->getRefCount() : 0;
        }

        /// @brief Returns the total size of the payload memory used by this block.
        /// @note The size may not be available in some block types, or on some platforms.
        /// @warning It should NEVER be used to determine how much memory can be stored in or read from the block.
        ///          This is just a hint about memory usage caused by this block.
        /// @return The total size of the underlying payload memory associated with this block; 0 if it is unknown.
        ///         It does not include the size of the block header or any memory used for book-keeping.
        inline size_t getMemorySize() const
        {
            return ( _data.block != 0 ) ? _data.block->getMemorySize() : 0;
        }

    protected:
        /// @brief Constructor.
        /// Creates a MemHandle based on given MemData object.
        /// @note This constructor takes over the MemData passed, and it does NOT increment the reference counter!
        /// @param [in] memData The MemData object to use.
        MemHandle ( const MemData & memData );

        /// @brief Exposes internal MemData object.
        /// @return Internal MemData object.
        inline const MemData & getMemData() const
        {
            return _data;
        }

        /// @brief Writes data to a file.
        /// @param [in] data The pointer to the data to be written.
        /// @param [in] size The size of the data.
        /// @param [in] filePath The path of the file to write to.
        /// @param [in] appendToFile If set to true, the data will be appended to the file.
        ///                           Otherwise the content of the file (if any) is overwritten (this is the default).
        /// @return True if the operation was successful; False otherwise.
        static bool writeDataToFile (
            const char * data, size_t size,
            const char * filePath, bool appendToFile = false );

    private:
        MemData _data; ///< Contains configuration of underlying memory.

        friend class MemVector;
        friend class Buffer;
        friend class BasicMemPool;
};

/// @brief A wrapper around MemHandle, to be used with external memory blocks with custom deallocator.
class ExtMemHandle: public MemHandle
{
    public:
        /// @brief Constructor.
        /// @param [in] data Pointer to external data block.
        /// @param [in] dataSize The size of the data block.
        /// @param [in] deallocator Function to be called when the memory is released.
        ///                         If it's 0, it means that memory does not need to be deallocated.
        /// @param [in] deallocatorData User data that will be associated with the memory block.
        ///                             It can be used to store additional state needed by the deallocator.
        ///                             It will be stored inside the DeallocatorMemBlock and can
        ///                             be accessed by deallocator function (which is being passed a pointer
        ///                             to the DeallocatorMemBlock being released).
        ExtMemHandle (
            char * data,
            size_t dataSize,
            DeallocatorMemBlock::DeallocatorFunctionType deallocator = 0,
            void * deallocatorData = 0 );
};
}
