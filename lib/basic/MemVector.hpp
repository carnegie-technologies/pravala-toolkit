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

#include "TupleArray.hpp"
#include "MemHandle.hpp"

extern "C"
{
#ifndef SYSTEM_WINDOWS
#include <sys/uio.h>
#else
/// @brief I/O vector for scatter/gather operations
struct iovec
{
    void * iov_base;                 ///< Pointer to data.
    size_t iov_len;                  ///< Size of the data.
};
#endif
}

namespace Pravala
{
/// @brief Stores a vector of memory chunks.
class MemVector
{
    public:
        typedef uint16_t IndexType;                  ///< The type used for indexing memory chunks.

        static const IndexType MaxChunks = 0xFFFFU;  ///< The max number of data chunks allowed.

        /// @brief An empty MemVector object.
        static const MemVector EmptyVector;

        /// @brief Default constructor.
        inline MemVector(): _dataSize ( 0 )
        {
        }

        /// @brief Constructor.
        /// Creates a MemVector and reserves the memory for given number of chunks.
        /// @param [in] numEntries The number of entries to pre-allocate.
        explicit MemVector ( IndexType numEntries );

        /// @brief A copy constructor.
        /// @param [in] other The object to create a copy of.
        MemVector ( const MemVector & other );

        /// @brief Constructor.
        /// Creates a MemVector with a single element that references the same memory as the MemHandle.
        /// The vector will have either a single non-empty element, or will be empty.
        /// @param [in] data The MemHandle to include in the vector.
        MemVector ( const MemHandle & data );

        /// @brief An assignment operator.
        /// @param [in] other The object to create a copy of.
        /// @return A reference to this object.
        MemVector & operator= ( const MemVector & other );

        /// @brief An assignment operator.
        /// The vector will have either a single non-empty element, or will be empty.
        /// @param [in] data The data to store in this vector.
        /// @return A reference to this object.
        MemVector & operator= ( const MemHandle & data );

        /// @brief Destructor.
        ~MemVector();

        /// @brief Clears the vector.
        void clear();

        /// @brief Append a MemHandle to the vector.
        /// @note This method considers appending an empty handle always to succeed,
        ///       even though the vector is not actually modified; Returning 'false' means that there was
        ///       an error referencing or copying the memory, or the offset was greater than the size.
        /// @param [in] mh The handle to append.
        /// @param [in] offset The offset in 'mh' at which to start appending.
        /// @return True if the handle was appended or the handle was empty;
        ///         False if it could not be appended (in which case this vector would not be modified).
        bool append ( const MemHandle & mh, size_t offset = 0 );

        /// @brief Append another MemVector to this one.
        /// @note This method considers appending an empty vector always to succeed,
        ///       even though the vector is not actually modified; Returning 'false' means that there was
        ///       an error referencing or copying the memory, or the offset was greater than the size.
        /// @param [in] vec The vector to append.
        /// @param [in] offset The offset in 'vec' at which to start appending.
        /// @return True if the vector was appended (or it was empty);
        ///         False if it could not be appended (in which case this vector would not be modified).
        ///         Note that appending a vector to itself always returns an error.
        bool append ( const MemVector & vec, size_t offset = 0 );

        /// @brief Insert a MemHandle at the beginning of the vector.
        /// @note This method considers appending an empty handle always to succeed,
        ///       even though the vector is not actually modified; Returning 'false' means that there was
        ///       an error referencing or copying the memory.
        /// @param [in] mh The handle to insert.
        /// @return True if the handle was inserted or the handle was empty; False otherwise.
        bool prepend ( const MemHandle & mh );

        /// @brief Removes data from the beginning of the MemVector.
        /// @param [in] numBytes The number of bytes at the beginning of the vector to remove.
        ///                      It can span multiple chunks.
        ///                      If it is equal or greater than the current data size, MemVector becomes empty.
        /// @return True if the MemVector is usable (not empty) after this operation; False otherwise.
        bool consume ( size_t numBytes );

        /// @brief Truncates the MemVector to given data size.
        /// @param [in] numBytes The max number of bytes at the beginning of the vector to keep.
        ///                      It can span multiple chunks.
        ///                      If it is equal or greater than the current data size, nothing will happen.
        void truncate ( size_t numBytes );

        /// @brief Moves the data from another vector to this object.
        /// Current data will be discarded first. The other vector will be emptied after calling this.
        /// @note If 'other' and 'this' are the same, no operation is performed.
        /// @param [in] other The vector to empty by transferring its content to this object.
        void stealFrom ( MemVector & other );

        /// @brief Returns the total size of data stored in this vector.
        /// @return The total size of data stored in this vector.
        inline size_t getDataSize() const
        {
            return _dataSize;
        }

        /// @brief Checks whether the vector is empty or not.
        /// @return True if the vector is empty, false otherwise.
        inline bool isEmpty() const
        {
            return ( _dataSize < 1 );
        }

        /// @brief Returns the number of memory chunks stored in this vector.
        /// @return The number of memory chunks stored.
        inline IndexType getNumChunks() const
        {
            return _dataVec.size();
        }

        /// @brief Exposes the array of memory chunks.
        /// Returned value can be used directly with writev() or stored in struct msghdr's msg_iov field.
        /// @return Pointer to the beginning of array with memory chunks.
        inline const struct iovec * getChunks() const
        {
            return _dataVec.getSecondMemory();
        }

        /// @brief Returns a specific memory chunk as a MemHandle object.
        /// @param [in] idx The index of the chunk to return.
        /// @return MemHandle associated with the same memory as the given chunk (with its own reference);
        ///         Empty if index is invalid, or the memory cannot be referenced/copied properly.
        MemHandle getChunk ( IndexType idx ) const;

        /// @brief A pointer to the memory block associated with specific chunk.
        /// @param [in] idx The index of the block to return.
        /// @return Pointer to the block associated with the given chunk; 0 if the index is invalid.
        inline const MemBlock * getBlock ( IndexType idx ) const
        {
            return ( idx < _dataVec.size() ) ? _dataVec.getFirst ( idx ) : 0;
        }

        /// @brief Stores the content of this memory vector in a single, continuous memory segment.
        /// This can be a very expensive operation, since copy of the entire memory is created
        /// (unless this vector is empty or there is a single memory chunk in it).
        /// This function should be avoided whenever it's possible!
        /// @note This method called on an empty vector always succeeds, even though the memory handle will
        ///       be empty after the call in that case; Returning 'false' means that there was
        ///       an error referencing or copying the memory.
        /// @param [in,out] memory The MemHandle to store the memory in. It can be empty or pre-allocated,
        ///                        and will be overwritten and possibly truncated. If it is too small,
        ///                        it will be replaced with another handle. Note that if this vector
        ///                        contains a single memory chunk, the handler will be re-attached to that
        ///                        memory chunk.
        /// @return True if the memory was successfully stored in memory handle (or the vector was empty,
        ///         in which case memory would be cleared); False otherwise.
        bool storeContinuous ( MemHandle & memory ) const;

        /// @brief Returns a writable pointer to the beginning of this vector's memory.
        /// It ensures that the memory is not shared (it will be copied if necessary),
        /// and that it is stored as a single, continuous chunk.
        /// Only the requested part of the memory is treated like that.
        /// For instance, if the vector contained 1000 bytes of memory in a single chunk that was shared
        /// and this is called with the size = 10, after the call the vector will consist of two memory chunks.
        /// The first one will contain 10 bytes and will not be shared, and the following chunk will contain 990
        /// bytes (and will still be shared). That operation will require copying only 10 bytes.
        /// @param [in] size The number of bytes at the beginning of the vector that need to be checked.
        ///                  Returned memory pointer is guaranteed to only reference that many continuous bytes
        ///                  (although it may be bigger if, for instance, vector contained a single, not shared chunk
        ///                  with more bytes than 'size'). If 0, the entire vector is "flattened".
        /// @param [in] useHandle Optional MemHandle to be used for storing the continuous memory (if a specific
        ///                       MemHandle type is preferred). Only used if its size is >= 'size'.
        ///                       If it ends up being used, that MemHandle will be cleared by this call.
        /// @return Pointer to writable 'size' bytes at the beginning of this vector;
        ///         0 if that data could not be returned as a writable memory
        ///         (0 could mean memory problems, or that the vector was simply smaller than requested size).
        char * getContinuousWritable ( size_t size = 0, MemHandle * useHandle = 0 );

    protected:
        typedef TupleArray<MemBlock *, struct iovec, IndexType> DataArray;

        /// @brief Array with internal data blocks.
        /// This is a special array that maintains two continuous memory blocks with blocks of each type.
        /// Each entry has a MemBlock controlling specific memory chunk, and iovec structure that describes that memory.
        DataArray _dataVec;

        /// @brief The total data size stored in this vector (in bytes).
        size_t _dataSize;

        /// @brief References all blocks in _blocks.
        /// In case it cannot reference something, MemData will be used to copy the data.
        /// If that fails too, this object will be cleared.
        /// @note This should only be used with _blocks array that has not been referenced yet.
        void refAllBlocks();

        /// @brief A helper function that appends the memory represented by MemData to _dataVec.
        /// It does NOT modify references.
        /// It does NOT modify _dataSize.
        /// It simply appends new value at the end of _dataVec.
        /// @param [in] mData The MemData to append.
        inline void appendMemData ( const MemData & mData )
        {
            struct iovec iov =
            {
                mData.mem, mData.size
            };

            _dataVec.append ( mData.block, iov );
        }

        /// @brief A helper function that inserts the memory represented by MemData at the beginning of _dataVec.
        /// It does NOT modify references.
        /// It does NOT modify _dataSize.
        /// It simply inserts the new value at the beginning of _dataVec.
        /// @param [in] mData The MemData to append.
        inline void prependMemData ( const MemData & mData )
        {
            struct iovec iov =
            {
                mData.mem, mData.size
            };

            _dataVec.prepend ( mData.block, iov );
        }
};
}
