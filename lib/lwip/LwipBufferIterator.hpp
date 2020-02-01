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

#include <cstddef>

#include "basic/SimpleArray.hpp"

struct pbuf;
struct iovec;

namespace Pravala
{
/// @brief An iterator over an lwIP pbuf buffer object.
/// lwIP buffers are chains of memory chunks. This wrapper allows access to individual chunks in order.
class LwipBufferIterator
{
    public:
        /// @brief Default constructor.
        /// This creates an empty LwipBufferIterator, that is not associated with any pbuf.
        LwipBufferIterator();

        /// @brief A copy constructor.
        /// This does not copy the data, it attaches itself to the same pbuf as the other LwipBufferIterator and
        /// increments the underlying pbuf's reference count.
        /// This LwipBufferIterator will also have the same data chunk position as the other LwipBufferIterator.
        /// @param [in] other The object to create a copy of.
        LwipBufferIterator ( const LwipBufferIterator & other );

        /// @brief An assignment operator.
        /// This does not copy the data, it attaches itself to the same pbuf as the other LwipBufferIterator and
        /// increments underlying pbuf's reference count.
        /// This LwipBufferIterator will also have the same data chunk position as the other LwipBufferIterator.
        /// @param [in] other The object to create a copy of.
        /// @return A reference to this object
        LwipBufferIterator & operator= ( const LwipBufferIterator & other );

        /// @brief Destructor
        /// Decrements the reference count in the underlying pbuf buffer.
        ~LwipBufferIterator();

        /// @brief Clears this LwipBufferIterator
        /// Releases the underlying pbuf buffer and resets all sizes.
        void clear();

        /// @brief Consumes bytes at the beginning of the buffer.
        ///
        /// @param [in] numBytes The number of bytes in the front of the buffer to remove.
        ///                       If it is equal or greater than the remaining size, LwipBufferIterator becomes empty
        ///                       (and not associated with a pbuf buffer anymore).
        ///                       If it is equal to or greater than the current chunk's size, this iterator will
        ///                       move to the next data chunk. Note: if greater than the current chunk's size,
        ///                       then part of the next chunk will also be consumed!
        /// @note This also skips all empty pbuf objects at the beginning of the chain.
        /// @return True if the LwipBufferIterator is usable (not empty) after this operation; False otherwise.
        bool consume ( size_t numBytes );

        /// @brief Returns the pointer to the current chunk of memory.
        ///
        /// Each LwipBufferIterator represents multiple chunks of data in a linked list.
        /// Hence it is only possible to access a single chunk of data at a time.
        ///
        /// The next chunk of memory can only be accessed by calling consume with the current size or greater.
        ///
        /// @return The pointer to the current chunk of memory, or 0 if empty.
        const char * getCurrentData() const;

        /// @brief Returns the size of the current chunk of memory.
        ///
        /// The next chunk of memory can only be accessed by calling consume with the current size or greater.
        ///
        /// @return The max number of bytes that could be read starting from the pointer returned by getCurrentData().
        size_t getCurrentSize() const;

        /// @brief Returns the total remaining size of the memory represented by this iterator.
        /// This is sum of the sizes of all remaining data chunks.
        /// @return The total remaining size of the memory accessible using this iterator, or 0 if empty.
        size_t getSize() const;

        /// @brief Checks whether this iterator is empty or not.
        /// @return True if this iterator is empty, false otherwise.
        inline bool isEmpty() const
        {
            return ( getSize() < 1 );
        }

        /// @brief Append the current sequence of buffers to an array of iovec structs.
        /// This doesn't copy any data, it just stores the pointers and lengths of each data chunk in the iovec structs.
        /// This means this iterator should NOT be modified while the array of iovec structs is being used.
        /// This always skips any chunks that are empty.
        /// This does not clear the array before appending.
        /// @param [out] array The array of iovec structs to append buffers to.
        void appendToIovecArray ( SimpleArray<struct iovec> & array ) const;

    protected:
        /// @brief Constructor.
        /// This creates an LwipBufferIterator attached to the given pbuf, holding a reference to it.
        /// @note This constructor will create a new reference to the pbuf structure,
        ///       so the caller should remove its own reference (if it owns one).
        /// @param [in] buffer The pbuf to attach to. Could be 0.
        LwipBufferIterator ( struct pbuf * buffer );

    private:
        /// @brief The pointer to the current pbuf chunk.
        /// This iterator always holds a single reference in this buffer.
        struct pbuf * _buffer;

        /// @brief The offset within the current chunk.
        size_t _offset;

        friend class LwipTcpSocket;
        friend class LwipUdpSocket;
};
}
