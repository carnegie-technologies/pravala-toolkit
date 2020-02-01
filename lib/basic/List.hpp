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

#include <cstdlib>

#include "internal/SharedMemory.hpp"
#include "NoCopy.hpp"

#ifndef NDEBUG
#define CHECK_STATE \
    do { \
        if ( _priv != 0 ) { \
            assert ( _priv->dataSize <= _priv->bufSize ); \
            assert ( _priv->begIndex < _priv->bufSize ); \
            assert ( _priv->endIndex < _priv->bufSize ); \
            if ( _priv->dataSize == 0 ) { \
                assert ( _priv->begIndex == 0 ); \
                assert ( _priv->endIndex == 0 ); \
            } \
            else if ( _priv->begIndex <= _priv->endIndex ) { \
                assert ( _priv->dataSize == _priv->endIndex - _priv->begIndex + 1 ); \
            } \
            else { \
                assert ( _priv->dataSize == _priv->bufSize - _priv->begIndex + _priv->endIndex + 1 ); \
            } \
        } \
    } while ( false )
#else
#define CHECK_STATE
#endif

namespace Pravala
{
/// @brief Private data of the List.
/// Common for all List objects, no matter what type they use.
/// It is NOT thread safe!
class ListPriv: public NoCopy
{
    public:
        /// @brief The minimal size for ListPriv
        static const size_t MinSize = 4;

        /// @brief Default constructor.
        /// Creates an empty ListPriv object with reference count set to 1.
        /// This class is NOT thread safe!
        /// @param [in] sizeHint The requested number of elements in the list.
        ///                      It is possible that allocated object has more elements than that.
        explicit ListPriv ( size_t sizeHint );

        /// @brief Destructor
        ~ListPriv();

        /// @brief Swaps content of two ListPriv objects
        ///
        /// It doesn't touch reference counters,
        /// it only swaps the buffer pointers, data offset and sizes.
        /// It doesn't copy the content of the buffers, it only swaps
        /// the pointers to the buffers allocated.
        ///
        /// @param [in] other The other ListPriv object to swap content with
        void swapContent ( ListPriv * other );

        /// @brief Returns a raw value at given index.
        /// Index has to be correct [0; bufSize), or the program will crash.
        /// @param [in] idx Index to query.
        /// @return Reference to raw data stored (as a void pointer).
        inline SharedMemory::Pointer & at ( size_t idx )
        {
            assert ( idx < bufSize );

            // For some WEIRD reason this code (specifically idx < bufSize part )
            // refuses to compile (with -Werror=strict-overflow message) unless we use -fno-strict-overflow option
            if ( idx < bufSize )
            {
                return buffer[ idx ];
            }

            abort();
        }

        /// @brief Returns a raw value at given index.
        /// Index has to be correct [0; bufSize), or the program will crash.
        /// @param [in] idx Index to query.
        /// @return Constant reference to raw data stored (as a void pointer).
        inline const SharedMemory::Pointer & at ( size_t idx ) const
        {
            assert ( idx < bufSize );

            if ( idx < bufSize )
            {
                return buffer[ idx ];
            }

            abort();
        }

        /// @brief Reference counter for this data segment.
        SharedMemory::RefCounter ref;

        /// @brief Pointer to allocated memory.
        ///
        SharedMemory::Pointer * buffer;

        /// @brief Size of allocated buffer (the number of objects)
        size_t bufSize;

        /// @brief Number of valid objects
        size_t dataSize;

        /// @brief Offset where first valid data object is stored
        size_t begIndex;

        /// @brief Index of the last element. When the list is empty it is the same as begIndex.
        /// It is also the same if there is a single element in the list. To distinguish between these two
        /// the dataSize should be used.
        size_t endIndex;

    private:
        /// @brief Doesn't exist.
        ListPriv();
};

/// @brief List class.
/// Offers appending and prepending data, and index-based querying.
template<typename T> class List
{
    public:
        /// @brief Default constructor.
        /// Creates an empty list.
        inline List(): _priv ( 0 )
        {
        }

        /// @brief Constructor.
        /// This constructor pre-allocates some memory, to make future appends more efficient.
        /// It can be used if the number of elements to be added to the list is known in advance.
        /// @param [in] preAllocatedSize The number of elements for which to reserve the memory.
        inline explicit List ( size_t preAllocatedSize ):
            _priv ( ( preAllocatedSize > 0 ) ? ( new ListPriv ( preAllocatedSize ) ) : 0 )
        {
        }

        /// @brief Copy constructor.
        /// The list uses implicit sharing, so the actual data
        /// is not really copied, unless one of the list objects sharing
        /// data needs to modify it.
        /// @param [in] other List object to copy.
        List ( const List & other );

        /// @brief Assignment operator.
        /// The list uses implicit sharing, so the actual data
        /// is not really copied, unless one of the list objects sharing
        /// data needs to modify it.
        /// @param [in] other List object to copy.
        /// @return Reference to this list.
        List & operator= ( const List & other );

        /// @brief Destructor.
        /// If List contains pointer objects (is declared as List<Type *>),
        /// this destructor does NOT delete those pointers.
        /// If the List contains normal objects (is declared as List<Type>),
        /// and they're stored internally as pointers, they are be deleted.
        ~List();

        /// @brief Returns the number of objects stored in the list.
        /// @return Number of objects stored in the list.
        inline size_t size() const
        {
            if ( !_priv )
            {
                return 0;
            }

            return _priv->dataSize;
        }

        /// @brief Checks whether the list is empty
        /// @return True if the list is empty, false otherwise
        inline bool isEmpty() const
        {
            return ( !_priv || ( _priv->dataSize == 0 ) );
        }

        /// @brief Compares two Lists to determine equality.
        /// Two lists are equal if they contain the same elements in the same order.
        /// @param [in] other The second list to compare
        /// @return A value indicating whether the two lists are equal
        bool operator== ( const List & other ) const;

        /// @brief Compares two Lists to determine inequality.
        /// Two lists are equal if they contain the same elements in the same order.
        /// @param [in] other The second list to compare
        /// @return A value indicating whether the two lists are inequal.
        inline bool operator!= ( const List & other ) const
        {
            return !( *this == other );
        }

        /// @brief Looks for a given value in the List
        /// @param [in] value The value to look for
        /// @param [out] foundIndex The index at which the given element has been found (if used).
        ///                          It is not set if the element is not found.
        /// @param [in] startIndex The index from which to start looking
        /// @return true if the given value has been found; false otherwise.
        bool findValue ( const T & value, size_t * foundIndex = 0, size_t startIndex = 0 ) const;

        /// @brief Returns a constant reference to an object stored under given index.
        /// Index has to be valid (from [0; size()> range), otherwise program will abort.
        /// @param [in] idx Index to query.
        /// @return Constant reference to an object.
        const T & operator[] ( size_t idx ) const;

        /// @brief The same as operator[], but only exists in const version (to ensure that we don't copy any data)
        /// Index has to be valid (from [0; size()> range), otherwise program will abort.
        /// @param [in] idx Index to query.
        /// @return Constant reference to an object.
        const T & at ( size_t idx ) const;

        /// @brief Returns a reference to an object stored under given index.
        /// Index has to be valid (from [0; size()> range), otherwise program will abort.
        /// Since this is non-constant reference, if the internal data is shared it is
        /// copied, so this object would be the only one to use it.
        /// Because of that, it is better to use const version of that function (if possible)
        /// which doesn't perform copying.
        /// @param [in] idx Index to query.
        /// @return Reference to an object.
        T & operator[] ( size_t idx );

        /// @brief Returns a constant reference to the first element in the queue.
        /// The queue cannot be empty!
        /// @return Constant reference to an object.
        const T & first() const;

        /// @brief Returns a reference to the first element in the queue.
        /// The queue cannot be empty!
        /// If the memory is shared, this will copy the data, so if possible, the const version should be used instead.
        /// @return A reference to an object.
        T & first();

        /// @brief Removes the first element from the queue
        /// @return True if the element was removed, false if the queue was empty
        bool removeFirst();

        /// @brief Returns a constant reference to the last element in the queue.
        /// The queue cannot be empty!
        /// @return Constant reference to an object.
        const T & last() const;

        /// @brief Returns a reference to the last element in the queue.
        /// The queue cannot be empty!
        /// If the memory is shared, this will copy the data, so if possible, the const version should be used instead.
        /// @return A reference to an object.
        T & last();

        /// @brief Removes the last element from the queue
        /// @return True if the element was removed, false if the queue was empty
        bool removeLast();

        /// @brief Appends the value at the end of the list.
        /// If the internal data of the list is shared, the copy is made before modifying the list.
        /// The value provided is copied and depending on the type and configuration (using TypeInfo)
        /// is internally stored directly or with pointers to its copy.
        /// @param [in] value Value to append.
        /// @return A reference to this list.
        List<T> & append ( const T & value );

        /// @brief Inserts the value at the front of the list.
        /// If the internal data of the list is shared, the copy is made before modifying the list.
        /// The value provided is copied and depending on the type and configuration (using TypeInfo)
        /// is internally stored directly or with pointers to its copy.
        /// @param [in] value Value to insert.
        /// @return A reference to this list.
        List<T> & prepend ( const T & value );

        /// @brief Removes element at a given index.
        /// Removes an element from a specified index.
        /// The following elements will be moved to the front.
        /// Calling removeIndex(0) as long as the list is not empty, or as long as this function returns true,
        /// would be equivalent to clear() - just MUCH slower!
        /// @param [in] index Index of the element to remove.
        /// @return True if the element was removed, false if the index was incorrect
        bool removeIndex ( size_t index );

        /// @brief Removes first occurrence of the given value.
        /// Values are tested using == operator.
        /// @param [in] value Value to remove.
        /// @return True if the value was found (and removed), false otherwise
        bool removeValue ( const T & value );

        /// @brief Sorts the list using the function provided
        /// @param [in] compareFunc The function to be used for sorting. It should return 'true'
        ///                          when 'a' should be IN FRONT of 'b'.
        ///                          If '<' (or '<=') is used, the smallest element will be placed at the beginning.
        /// @return A reference to this list
        List<T> & sort ( bool ( * compareFunc )( const T & a, const T & b ) );

        /// @brief Sorts the list in ascending order using < operator
        /// @return A reference to this list
        List<T> & sortAscending();

        /// @brief Sorts the list in descending order using > operator
        /// @return A reference to this list
        List<T> & sortDescending();

        /// @brief Clears the list.
        /// Removes all the elements from the list.
        inline void clear()
        {
            internalClear ( false );
        }

        /// @brief Returns a value of reference counter.
        /// This is probably useful only for debugging.
        /// @return Value of reference counter associated with shared data.
        inline uint32_t getRefCount() const
        {
            if ( !_priv )
            {
                return 1;
            }

            return _priv->ref.count();
        }

    private:
        /// @brief Pointer to internal list's data
        /// Should always be pointing to something (either valid
        /// data or shared null segment).
        ListPriv * _priv;

        /// @brief Ensures that the private data of this list is not shared and that there is at least one empty slot
        ///
        /// If the internal data is shared with some other list, its copy is created.
        ///
        /// @param [in] ensureFreeSpace If true, the function will also make sure there is some free space left
        void ensureNotShared ( bool ensureFreeSpace );

        /// @brief Copies a range of data objects, except for a specified element
        /// The data can NOT overlap
        /// It uses SharedMemory::copyData() - make sure to read the description of that function as well.
        /// @param [in] dest Destination memory address
        /// @param [in] src Source memory address
        /// @param [in] srcBegIndex The index of the first element to copy from the source memory
        /// @param [in] srcEndIndex The index of the last element to copy from the source memory
        /// @param [in] srcExceptIndex The index of a single element to skip (if it is in the correct range)
        /// @return The number of elements that were copied
        static size_t copyDataExcept (
            SharedMemory::Pointer * dest,
            const SharedMemory::Pointer * src,
            size_t srcBegIndex, size_t srcEndIndex,
            size_t srcExceptIndex );

        /// @brief Removes element at a given index (internal version)
        /// Similar to the public removeIndex() method, but operates on absolute indexes that HAVE to be correct.
        /// It MAY call clear() and set the _priv pointer to 0!
        /// @param [in] index Internal index of the element to remove.
        void internalRemoveIndex ( size_t index );

        /// @brief Unrefs the _priv object, destructing all elements if we are the last reference
        /// @param [in] forceDelete If true and we were the last reference, _priv is always deleted.
        /// Otherwise, _priv is only deleted if we are the last reference and it is larger than MinSize.
        void internalClear ( bool forceDelete );

        /// @brief Perform's quick sort's partitioning
        /// It uses internal indexes.
        /// @param [in] left The left index
        /// @param [in] right The right index
        /// @param [in] pivotIndex The 'pivot' index
        /// @param [in] compareFunc The function used for comparison. See 'sort()'.
        /// @return The new pivot index
        size_t partition (
            size_t left, size_t right, size_t pivotIndex,
            bool ( * compareFunc )( const T & a, const T & b ) );

        /// @brief Runs quick sort on the range
        /// It uses internal indexes.
        /// @param [in] left The left index
        /// @param [in] right The right index
        /// @param [in] compareFunc The function used for comparison. See 'sort()'.
        void quicksort (
            size_t left, size_t right,
            bool ( * compareFunc )( const T & a, const T & b ) );

        /// @brief A helper function that is used by the sortAscending function
        /// @param [in] a First object to compare
        /// @param [in] b Second object to compare
        /// @return True if a < b; False otherwise
        static bool compareLT ( const T & a, const T & b );

        /// @brief A helper function that is used by the sortDescending function
        /// @param [in] a First object to compare
        /// @param [in] b Second object to compare
        /// @return True if a > b; False otherwise
        static bool compareGT ( const T & a, const T & b );
};

template<typename T> List<T>::List ( const List<T> & other ): _priv ( const_cast<ListPriv *> ( other._priv ) )
{
    if ( _priv != 0 )
    {
        _priv->ref.ref();
    }
}

template<typename T> void List<T>::internalClear ( bool forceDelete )
{
    if ( !_priv )
    {
        return;
    }

    assert ( _priv != 0 );

    if ( !_priv->ref.unref() )
    {
        // We are not the last reference to _priv, just detach it from us
        _priv = 0;
        return;
    }

    assert ( _priv != 0 );
    assert ( _priv->ref.count() == 0 );

    if ( _priv->dataSize > 0 )
    {
        CHECK_STATE;

        if ( _priv->begIndex <= _priv->endIndex )
        {
            assert ( _priv->dataSize == _priv->endIndex - _priv->begIndex + 1 );

            SharedMemory::destructData<T> ( _priv->buffer + _priv->begIndex, _priv->dataSize );
        }
        else
        {
            assert ( _priv->dataSize == _priv->bufSize - _priv->begIndex + _priv->endIndex + 1 );

            SharedMemory::destructData<T> ( _priv->buffer, _priv->endIndex + 1 );
            SharedMemory::destructData<T> ( _priv->buffer + _priv->begIndex, _priv->bufSize - _priv->begIndex );
        }
    }

    if ( forceDelete || _priv->bufSize > ListPriv::MinSize )
    {
        delete _priv;
        _priv = 0;
        return;
    }

    _priv->begIndex = 0;
    _priv->endIndex = 0;
    _priv->dataSize = 0;

    // The ref count should be 0 at this point (since we unrefed it to 0 above)
    assert ( _priv->ref.count() == 0 );

    // We need to ref it back up since we will continue to use it.
    // This must be the last instruction since others may have a ref to it as well
    // after this function.
    _priv->ref.ref();
    return;
}

template<typename T> List<T>::~List()
{
    internalClear ( true );

    assert ( !_priv );
}

template<typename T> List<T> & List<T>::operator= ( const List<T> & other )
{
    if ( &other == this || _priv == other._priv )
    {
        return *this;
    }

    internalClear ( true );

    assert ( !_priv );

    _priv = const_cast<ListPriv *> ( other._priv );

    if ( _priv != 0 )
    {
        _priv->ref.ref();
    }

    return *this;
}

template<typename T> bool List<T>::operator== ( const List<T> & other ) const
{
    if ( &other == this || _priv == other._priv )
    {
        return true;
    }

    if ( size() != other.size() )
    {
        return false;
    }

    for ( size_t idx = 0; idx < size(); ++idx )
    {
        if ( at ( idx ) != other.at ( idx ) )
            return false;
    }

    return true;
}

template<typename T> bool List<T>::findValue ( const T & value, size_t * foundIndex, size_t startIndex ) const
{
    if ( !_priv || _priv->dataSize < 1 || startIndex >= _priv->dataSize )
    {
        return false;
    }

    const ListPriv & p = *_priv;

    assert ( p.bufSize > 0 );

    if ( p.endIndex >= p.begIndex )
    {
        assert ( p.begIndex + startIndex <= p.endIndex );

        for ( size_t idx = p.begIndex + startIndex; idx <= p.endIndex; ++idx )
        {
            if ( SharedMemory::convertValue<T> ( p.at ( idx ) ) == value )
            {
                assert ( idx >= p.begIndex );

                if ( foundIndex != 0 )
                {
                    *foundIndex = ( idx - p.begIndex );
                }

                return true;
            }
        }

        return false;
    }

    assert ( p.endIndex < p.begIndex );

    size_t idx = p.begIndex + startIndex;

    if ( idx < p.bufSize )
    {
        for ( ; idx < p.bufSize; ++idx )
        {
            if ( SharedMemory::convertValue<T> ( p.at ( idx ) ) == value )
            {
                assert ( idx >= p.begIndex );

                if ( foundIndex != 0 )
                {
                    *foundIndex = ( idx - p.begIndex );
                }

                return true;
            }
        }

        idx = 0;
    }
    else
    {
        assert ( p.begIndex + startIndex >= p.bufSize );

        idx = p.begIndex + startIndex - p.bufSize;
    }

    for ( ; idx <= p.endIndex; ++idx )
    {
        if ( SharedMemory::convertValue<T> ( p.at ( idx ) ) == value )
        {
            if ( foundIndex != 0 )
            {
                *foundIndex = ( p.bufSize - p.begIndex + idx );
            }

            return true;
        }
    }

    return false;
}

template<typename T> const T & List<T>::operator[] ( size_t idx ) const
{
    assert ( _priv != 0 );
    assert ( idx < _priv->dataSize );

    if ( !_priv || idx >= _priv->dataSize )
    {
        abort();
    }

    return SharedMemory::convertValue<T> ( _priv->at ( ( _priv->begIndex + idx ) % _priv->bufSize ) );
}

template<typename T> const T & List<T>::at ( size_t idx ) const
{
    assert ( _priv != 0 );
    assert ( idx < _priv->dataSize );

    if ( !_priv || idx >= _priv->dataSize )
    {
        abort();
    }

    return SharedMemory::convertValue<T> ( _priv->at ( ( _priv->begIndex + idx ) % _priv->bufSize ) );
}

template<typename T> T & List<T>::operator[] ( size_t idx )
{
    assert ( _priv != 0 );
    assert ( idx < _priv->dataSize );

    if ( !_priv || idx >= _priv->dataSize )
    {
        abort();
    }

    ensureNotShared ( false );

    assert ( _priv->ref.count() == 1 );

    return SharedMemory::convertValue<T> ( _priv->at ( ( _priv->begIndex + idx ) % _priv->bufSize ) );
}

template<typename T> const T & List<T>::first() const
{
    assert ( _priv != 0 );
    assert ( _priv->dataSize > 0 );

    if ( !_priv || _priv->dataSize < 1 )
    {
        abort();
    }

    return SharedMemory::convertValue<T> ( _priv->at ( _priv->begIndex ) );
}

template<typename T> T & List<T>::first()
{
    assert ( _priv != 0 );
    assert ( _priv->dataSize > 0 );

    if ( !_priv || _priv->dataSize < 1 )
    {
        abort();
    }

    ensureNotShared ( false );

    assert ( _priv->ref.count() == 1 );

    return SharedMemory::convertValue<T> ( _priv->at ( _priv->begIndex ) );
}

template<typename T> const T & List<T>::last() const
{
    assert ( _priv != 0 );
    assert ( _priv->dataSize > 0 );

    if ( !_priv || _priv->dataSize < 1 )
    {
        abort();
    }

    return SharedMemory::convertValue<T> ( _priv->at ( _priv->endIndex ) );
}

template<typename T> T & List<T>::last()
{
    assert ( _priv != 0 );
    assert ( _priv->dataSize > 0 );

    if ( !_priv || _priv->dataSize < 1 )
    {
        abort();
    }

    ensureNotShared ( false );

    assert ( _priv->ref.count() == 1 );

    return SharedMemory::convertValue<T> ( _priv->at ( _priv->endIndex ) );
}

template<typename T> List<T> & List<T>::append ( const T & value )
{
    ensureNotShared ( true );

    assert ( _priv != 0 );
    assert ( _priv->ref.count() == 1 );
    assert ( _priv->bufSize > 0 );
    assert ( _priv->bufSize > _priv->dataSize );

    if ( _priv->dataSize == 0 )
    {
        _priv->begIndex = 0;
        _priv->endIndex = 0;
        _priv->dataSize = 1;
    }
    else
    {
        assert ( _priv->dataSize != 0 );

        _priv->endIndex = ( _priv->endIndex + 1 ) % _priv->bufSize;
        ++_priv->dataSize;
    }

    CHECK_STATE;

    SharedMemory::copySingle ( _priv->at ( _priv->endIndex ), value );

    return *this;
}

template<typename T> List<T> & List<T>::prepend ( const T & value )
{
    ensureNotShared ( true );

    assert ( _priv != 0 );
    assert ( _priv->ref.count() == 1 );
    assert ( _priv->bufSize > 0 );
    assert ( _priv->bufSize > _priv->dataSize );

    if ( _priv->dataSize == 0 )
    {
        _priv->begIndex = 0;
        _priv->endIndex = 0;
        _priv->dataSize = 1;
    }
    else
    {
        if ( _priv->begIndex > 0 )
        {
            --_priv->begIndex;
        }
        else
        {
            _priv->begIndex = _priv->bufSize - 1;
        }

        ++_priv->dataSize;
    }

    CHECK_STATE;

    SharedMemory::copySingle ( _priv->at ( _priv->begIndex ), value );

    return *this;
}

template<typename T> bool List<T>::removeFirst()
{
    if ( !_priv || _priv->dataSize < 1 )
    {
        return false;
    }

    internalRemoveIndex ( _priv->begIndex );
    return true;
}

template<typename T> bool List<T>::removeLast()
{
    if ( !_priv || _priv->dataSize < 1 )
    {
        return false;
    }

    internalRemoveIndex ( _priv->endIndex );
    return true;
}

template<typename T> void List<T>::ensureNotShared ( bool ensureFreeSpace )
{
    if ( !_priv )
    {
        _priv = new ListPriv ( ListPriv::MinSize );

        assert ( _priv != 0 );

        return;
    }

    // Our private data is NOT shared, and either we don't need more memory, or we do but there is still space left.
    // Nothing else to do!
    if ( _priv->ref.count() < 2 && ( !ensureFreeSpace || _priv->dataSize < _priv->bufSize ) )
    {
        return;
    }

    if ( _priv->ref.count() > 1 )
    {
        // The memory is shared. We need to make a new Priv object and copy the data!

        ListPriv * orgPriv = _priv;

        assert ( orgPriv != 0 );

        const size_t oSize = orgPriv->dataSize;
        size_t newBufSize = oSize;

        if ( ensureFreeSpace && newBufSize >= orgPriv->bufSize )
        {
            assert ( newBufSize == orgPriv->bufSize );

            // We need more space, and the original memory is full.
            // The new Priv object needs more memory.
            // If the original memory was not full, we would create a new Priv object
            // with the size sufficient to store the original content,
            // and then swap the content of the original and the new one.
            // We know for sure that we need more memory,
            // other owners (of the original priv object) may not need more of it!

            newBufSize = newBufSize * 3 / 2;
        }

        assert ( newBufSize >= oSize );

        // ListPriv makes sure the size is > 0!
        _priv = new ListPriv ( newBufSize );

        assert ( _priv != 0 );
        assert ( _priv->bufSize > 0 );
        assert ( _priv->bufSize >= oSize );
        assert ( _priv->begIndex == 0 );
        assert ( _priv->endIndex == 0 );

        if ( oSize > 0 )
        {
            size_t copiedSize;

            if ( orgPriv->begIndex <= orgPriv->endIndex )
            {
                // Memory looks normal. Just a single copy.

                assert ( oSize == orgPriv->endIndex - orgPriv->begIndex + 1 );

                copiedSize = SharedMemory::copyData<T> (
                    _priv->buffer, orgPriv->buffer, orgPriv->begIndex, orgPriv->endIndex );
            }
            else
            {
                // Data is fragmented - two copy operations

                copiedSize = SharedMemory::copyData<T> ( _priv->buffer, orgPriv->buffer,
                                                         orgPriv->begIndex, orgPriv->bufSize - 1 );

                copiedSize += SharedMemory::copyData<T> ( _priv->buffer + copiedSize, orgPriv->buffer,
                                                          0, orgPriv->endIndex );
            }

            assert ( oSize == copiedSize );

            _priv->endIndex = copiedSize - 1;
        }

        _priv->dataSize = oSize;

        CHECK_STATE;

        if ( ensureFreeSpace && orgPriv->bufSize > _priv->bufSize )
        {
            // We need free memory, but we just created an object that has less of it than the original.
            // This means we need to swap their content.

            _priv->swapContent ( orgPriv );
        }

        assert ( !ensureFreeSpace || _priv->bufSize > oSize );

        CHECK_STATE;

        orgPriv->ref.unref();

        return;
    }

    // The memory is not shared, but we need more space!

    assert ( ensureFreeSpace );
    assert ( _priv->ref.count() == 1 );
    assert ( _priv->dataSize > 0 );
    assert ( _priv->dataSize == _priv->bufSize );

    /// @todo To be improved?
    size_t newBufSize = _priv->bufSize * 3 / 2;

    assert ( newBufSize > _priv->dataSize );

    // There are three types of data - stored using pointers,
    // stored directly (as embedded value) that do not need copy constructor
    // on move, and those that are stored as embedded values that need
    // copy constructor on move.
    // For the first and the second type we can use realloc,
    // for the third type we need to create a new buffer,
    // call copy constructor and destructor.
    if ( TypeInfo<T>::IsEmbedded && ( TypeInfo<T>::TypeConfig & TYPE_CONF_CALL_COPY_CONSTR_ON_MOVE ) )
    {
        // This is the third type - the difficult one.

        ListPriv * orgPriv = _priv;

        _priv = new ListPriv ( newBufSize );

        assert ( _priv->bufSize >= newBufSize );
        assert ( _priv->bufSize > orgPriv->bufSize );
        assert ( _priv->bufSize > orgPriv->dataSize );

        size_t copiedSize = 0;

        if ( orgPriv->endIndex >= orgPriv->begIndex )
        {
            // this memory looks normal - the end is after the beginning

            copiedSize = SharedMemory::moveData<T> ( _priv->buffer, orgPriv->buffer,
                                                     orgPriv->begIndex, orgPriv->endIndex );
        }
        else
        {
            assert ( orgPriv->endIndex < orgPriv->begIndex );

            // This memory has the end of the data before the beginning. We need to move it in two parts:

            copiedSize = SharedMemory::moveData<T> ( _priv->buffer, orgPriv->buffer,
                                                     orgPriv->begIndex, _priv->bufSize - 1 );

            copiedSize += SharedMemory::moveData<T> ( _priv->buffer + copiedSize, orgPriv->buffer,
                                                      0, orgPriv->endIndex );
        }

        assert ( orgPriv->dataSize == copiedSize );

        _priv->dataSize = copiedSize;

        assert ( _priv->begIndex == 0 );
        assert ( _priv->dataSize > 0 );
        assert ( _priv->dataSize < _priv->bufSize );

        _priv->endIndex = _priv->dataSize - 1;

        CHECK_STATE;

        orgPriv->ref.unref();

        return;
    }

    // This is the nicer type - it is either something that we store by keeping pointers to
    // the actual values, or we keep it as embedded type which doesn't require copy constructor on move!
    // This means we can use realloc, which should be faster!

    assert ( _priv->ref.count() == 1 );

    _priv->buffer = static_cast<SharedMemory::Pointer *> ( realloc ( _priv->buffer,
                                                                     newBufSize * sizeof ( SharedMemory::Pointer ) ) );

    if ( _priv->endIndex >= _priv->begIndex )
    {
        // this memory looks normal - the end is after the beginning!

        assert ( _priv->bufSize < newBufSize );

        _priv->bufSize = newBufSize;

        CHECK_STATE;

        return;
    }

    assert ( _priv->endIndex < _priv->begIndex );

    // This memory has the end of the data before the beginning. Since the size of the buffer changed,
    // we need to reorganize the memory.

    // We basically have two ranges - the end of the data, that starts at index 0, and goes to endIndex,
    // and the beginning of the data, that starts at begIndex and goes to the previous buffer size.
    // We either need to move the end of the data to just after its beginning, or the other way
    // around. We will move whichever is smaller:

    // Not that the _priv->bufSize is the previous size of the buffer!
    size_t partSize = _priv->bufSize - _priv->begIndex;

    assert ( newBufSize > partSize );

    if ( partSize <= _priv->endIndex + 1 )
    {
        // We move the beginning of the data (which is at buffer's old end)
        // to buffer's new end.

        memmove ( _priv->buffer + newBufSize - partSize,
                  _priv->buffer + _priv->begIndex,
                  partSize * sizeof ( SharedMemory::Pointer ) );

        _priv->begIndex = newBufSize - partSize;
        _priv->bufSize = newBufSize;

        CHECK_STATE;

        return;
    }

    // We need to move the end of the data (which starts at index 0)
    // to start just after the first part of the data.
    // We may not have enough space to move all of it, in which case we need to move
    // part of it to the end of the buffer, and shift the remaining part to the beginning

    assert ( newBufSize > _priv->bufSize );

    // We can move up to this much data:
    partSize = newBufSize - _priv->bufSize;

    // We can move all of it!
    if ( partSize >= _priv->endIndex + 1 )
    {
        partSize = _priv->endIndex + 1;

        memmove ( _priv->buffer + _priv->bufSize, _priv->buffer, partSize * sizeof ( SharedMemory::Pointer ) );

        // bufSize - 1 is the last index
        _priv->endIndex = _priv->bufSize - 1 + partSize;
        _priv->bufSize = newBufSize;

        CHECK_STATE;

        return;
    }

    // We have to perform two moves...

    assert ( _priv->bufSize + partSize == newBufSize );

    // Move whatever we can to the end of the buffer...
    memmove ( _priv->buffer + _priv->bufSize, _priv->buffer, partSize * sizeof ( SharedMemory::Pointer ) );

    assert ( _priv->endIndex >= partSize );

    // This is the new endIndex - whatever it was before minus the number of elements we moved
    _priv->endIndex -= partSize;

    memmove ( _priv->buffer, _priv->buffer + partSize, ( _priv->endIndex + 1 ) * sizeof ( SharedMemory::Pointer ) );

    _priv->bufSize = newBufSize;

    CHECK_STATE;

    return;
}

template<typename T> size_t List<T>::copyDataExcept (
        SharedMemory::Pointer * dest,
        const SharedMemory::Pointer * src,
        size_t srcBegIndex, size_t srcEndIndex,
        size_t srcExceptIndex )
{
    if ( srcExceptIndex < srcBegIndex || srcExceptIndex > srcEndIndex )
    {
        return SharedMemory::copyData<T> ( dest, src, srcBegIndex, srcEndIndex );
    }

    assert ( srcExceptIndex >= srcBegIndex );
    assert ( srcExceptIndex <= srcEndIndex );
    assert ( srcBegIndex <= srcEndIndex );

    size_t ret = 0;

    if ( srcExceptIndex > srcBegIndex )
    {
        assert ( srcExceptIndex > 0 );

        ret = SharedMemory::copyData<T> ( dest, src, srcBegIndex, srcExceptIndex - 1 );
    }

    if ( srcExceptIndex < srcEndIndex )
    {
        assert ( srcExceptIndex + 1 <= srcEndIndex );

        ret += SharedMemory::copyData<T> ( dest + ret, src, srcExceptIndex + 1, srcEndIndex );
    }

    return ret;
}

template<typename T> bool List<T>::removeValue ( const T & value )
{
    if ( !_priv || _priv->dataSize < 1 )
    {
        return false;
    }

    assert ( _priv->bufSize > 0 );

    if ( _priv->endIndex >= _priv->begIndex )
    {
        for ( size_t idx = _priv->begIndex; idx <= _priv->endIndex; ++idx )
        {
            if ( SharedMemory::convertValue<T> ( _priv->at ( idx ) ) == value )
            {
                internalRemoveIndex ( idx );
                return true;
            }
        }

        return false;
    }

    assert ( _priv->endIndex < _priv->begIndex );

    for ( size_t idx = _priv->begIndex; idx < _priv->bufSize; ++idx )
    {
        if ( SharedMemory::convertValue<T> ( _priv->at ( idx ) ) == value )
        {
            internalRemoveIndex ( idx );
            return true;
        }
    }

    for ( size_t idx = 0; idx <= _priv->endIndex; ++idx )
    {
        if ( SharedMemory::convertValue<T> ( _priv->at ( idx ) ) == value )
        {
            internalRemoveIndex ( idx );
            return true;
        }
    }

    return false;
}

template<typename T> bool List<T>::removeIndex ( size_t index )
{
    if ( !_priv || _priv->dataSize <= index )
    {
        return false;
    }

    assert ( _priv->dataSize > 0 );
    assert ( index < _priv->dataSize );

    internalRemoveIndex ( ( _priv->begIndex + index ) % _priv->bufSize );
    return true;
}

template<typename T> void List<T>::internalRemoveIndex ( size_t index )
{
    assert ( _priv != 0 );
    assert ( _priv->dataSize > 0 );

    if ( _priv->dataSize == 1 )
    {
        assert ( index == _priv->begIndex );
        assert ( index == _priv->endIndex );

        clear();

        assert ( size() == 0 );

        CHECK_STATE;

        return;
    }

    assert ( _priv->dataSize > 1 );

#ifndef NDEBUG
    if ( _priv->begIndex <= _priv->endIndex )
    {
        assert ( index >= _priv->begIndex );
        assert ( index <= _priv->endIndex );
    }
    else
    {
        assert ( index >= _priv->begIndex || index <= _priv->endIndex );
    }
#endif

    if ( _priv->ref.count() > 1 )
    {
        // We need to create a new _priv object from scratch anyway,
        // let's copy all the data except for the element that we are removing!

        ListPriv * orgPriv = _priv;

        assert ( orgPriv != 0 );

        const size_t oSize = orgPriv->dataSize;

        assert ( oSize > 1 );

        _priv = new ListPriv ( oSize - 1 );

        assert ( _priv != 0 );
        assert ( _priv->bufSize >= oSize - 1 );
        assert ( _priv->begIndex == 0 );
        assert ( _priv->endIndex == 0 );

        size_t destOffset = 0;

        if ( orgPriv->endIndex >= orgPriv->begIndex )
        {
            // Memory looks normal, the end of data is after the beginning

            // Copy everything from begIndex to endIndex, except the index:
            destOffset = copyDataExcept ( _priv->buffer, orgPriv->buffer,
                                          orgPriv->begIndex, orgPriv->endIndex, index );
        }
        else
        {
            // The beginning of the data is at the end of the buffer, and the end - at the beginning.

            // Copy everything from begIndex to end of the buffer, except the index (if it's in that range):
            destOffset = copyDataExcept ( _priv->buffer, orgPriv->buffer,
                                          orgPriv->begIndex, orgPriv->bufSize - 1, index );

            // Copy everything from the beginning of the buffer up to endIndex,
            // except the index (if it's in that range) - we copy just after the previously copied data (+destOffset)
            destOffset += copyDataExcept ( _priv->buffer + destOffset, orgPriv->buffer,
                                           0, orgPriv->endIndex, index );
        }

        // destOffset is the new size!
        assert ( destOffset + 1 == oSize );

        _priv->dataSize = destOffset;
        _priv->endIndex = destOffset - 1;

        orgPriv->ref.unref();

        CHECK_STATE;

        return;
    }

    assert ( _priv->ref.count() == 1 );

    SharedMemory::destructSingle<T> ( _priv->at ( index ) );

    // We need to move either everything after the index, or before.
    // We want to move the part that is smaller!

    // Otherwise it's either empty or size = 1 - we would call clear() instead!
    assert ( _priv->begIndex != _priv->endIndex );

    // We have two parts of data (possibly one of them is empty):
    // - the data before the index
    // - the data after the index
    // We want to move the smaller part!

    if ( _priv->begIndex <= index )
    {
        // The index is after the begIndex:

        if ( index - _priv->begIndex <= _priv->dataSize / 2 )
        {
            // The data before the index is smaller than the data after.
            // Let's move the beginning of the data up by one index.
            // If index == begIndex, nothing is moved!

            SharedMemory::moveData<T> ( _priv->buffer + _priv->begIndex + 1, _priv->buffer,
                                        _priv->begIndex, index - 1 );

            if ( ++_priv->begIndex == _priv->bufSize )
            {
                _priv->begIndex = 0;
            }

            --_priv->dataSize;

            CHECK_STATE;

            return;
        }

        // We need to move the end of the data, by one index down.
        // There are two cases, either the endIndex is after the begIndex, or it is at the beginning of the buffer

        if ( _priv->begIndex <= _priv->endIndex )
        {
            // They actually should not be equal.
            // If they were, the list would either be empty or have 1 element.
            // This case is dealt with at the beginning.
            assert ( _priv->begIndex < _priv->endIndex );

            // The memory "looks normal" - the end of the data is after the beginning.
            // This is the simple case, we simply move everything after index by one index down:
            // If index == endIndex, nothing is moved!

            SharedMemory::moveData<T> ( _priv->buffer + index, _priv->buffer,
                                        index + 1, _priv->endIndex );

            // This has to be true. begIndex should be < endIndex. Which means we can safely decrement the endIndex.
            assert ( _priv->endIndex > 0 );

            --_priv->endIndex;
            --_priv->dataSize;

            CHECK_STATE;

            return;
        }

        assert ( _priv->endIndex < _priv->begIndex );

        // the data is fragmented - the beginning of it is at the end of the buffer,
        // and the end of the data is at the beginning of the buffer.

        // First we need to move the data at the end of the buffer down.
        // If index == bufSize - 1, nothing is moved!
        SharedMemory::moveData<T> ( _priv->buffer + index, _priv->buffer,
                                    index + 1, _priv->bufSize - 1 );

        // Now let's move a single element from the beginning of the buffer to its end:
        SharedMemory::moveData<T> ( _priv->buffer + _priv->bufSize - 1, _priv->buffer, 0, 0 );

        // If there were other elements at the beginning of the buffer we need to move them down by one:
        if ( _priv->endIndex > 0 )
        {
            SharedMemory::moveData<T> ( _priv->buffer, _priv->buffer, 1, _priv->endIndex );

            --_priv->endIndex;
        }
        else
        {
            assert ( _priv->endIndex == 0 );

            // Otherwise the last element is now at the end of the buffer:

            _priv->endIndex = _priv->bufSize - 1;
        }

        --_priv->dataSize;

        CHECK_STATE;

        return;
    }

    // The index is past the end of the buffer.

    assert ( index < _priv->begIndex );
    assert ( index <= _priv->endIndex );
    assert ( _priv->endIndex < _priv->begIndex );

    // Again, let's see which part is smaller:

    if ( _priv->endIndex - index <= _priv->dataSize / 2 )
    {
        // The data after the index is smaller than the data before.
        // Let's move the end of the data down by one index.
        // If index == endIndex, nothing is moved!

        SharedMemory::moveData<T> ( _priv->buffer + index, _priv->buffer,
                                    index + 1, _priv->endIndex );

        if ( _priv->endIndex > 0 )
        {
            --_priv->endIndex;
        }
        else
        {
            assert ( _priv->endIndex == 0 );

            _priv->endIndex = _priv->bufSize - 1;
        }

        assert ( _priv->dataSize > 0 );

        --_priv->dataSize;

        CHECK_STATE;

        return;
    }

    // We need to move the beginning of the data, by one index up.
    // We already know that the data is fragmented - the endIndex is lower than the begIndex.

    assert ( _priv->endIndex < _priv->begIndex );

    // First we need to move the data at the beginning of the buffer (before the index) up by one,
    // unless the index 0!
    if ( index > 0 )
    {
        SharedMemory::moveData<T> ( _priv->buffer + 1, _priv->buffer, 0, index - 1 );
    }

    // Now let's move a single element from the end of the buffer to its beginning:
    SharedMemory::moveData<T> ( _priv->buffer, _priv->buffer, _priv->bufSize - 1, _priv->bufSize - 1 );

    // If there were other elements at the end of the buffer we need to move them up by one:
    if ( _priv->begIndex < _priv->bufSize - 1 )
    {
        SharedMemory::moveData<T> ( _priv->buffer + _priv->begIndex + 1, _priv->buffer,
                                    _priv->begIndex, _priv->bufSize - 2 );

        ++_priv->begIndex;
    }
    else
    {
        assert ( _priv->begIndex == _priv->bufSize - 1 );

        // Otherwise the first element is now at the beginning of the buffer:

        _priv->begIndex = 0;
    }

    --_priv->dataSize;

    CHECK_STATE;

    return;
}

template<typename T> size_t List<T>::partition (
        size_t left, size_t right, size_t pivotIndex,
        bool ( * compareFunc )( const T & a, const T & b ) )
{
    size_t storeIndex = left;
    SharedMemory::Pointer pivotValue;

    assert ( left < _priv->bufSize );
    assert ( right < _priv->bufSize );

    // [pivot_index] -> pivot_value
    // [right] -> [pivot_index] (after this the element at 'right' index will be INVALID!

    SharedMemory::moveData<T> ( &pivotValue, _priv->buffer, pivotIndex, pivotIndex );
    SharedMemory::moveData<T> ( _priv->buffer + pivotIndex, _priv->buffer, right, right );

    for ( uint8_t pass = 0; pass < 2; ++pass )
    {
        size_t idx = left;
        size_t limit = right;

        if ( right >= left )
        {
            // Memory "looks normal"
            pass = 2;
        }
        else
        {
            if ( pass == 0 )
            {
                limit = _priv->bufSize;
            }
            else
            {
                assert ( pass == 1 );
                idx = 0;
            }
        }

        for ( ; idx < limit; ++idx )
        {
            if ( compareFunc ( SharedMemory::convertValue<T> ( _priv->at ( idx ) ),
                               SharedMemory::convertValue<T> ( pivotValue ) ) )
            {
                if ( idx != storeIndex )
                {
                    SharedMemory::Pointer tmp;

                    // [idx] -> tmp
                    // [store_index] -> [idx]
                    // tmp -> [store_index]

                    SharedMemory::moveData<T> ( &tmp, _priv->buffer, idx, idx );
                    SharedMemory::moveData<T> ( _priv->buffer + idx, _priv->buffer, storeIndex, storeIndex );
                    SharedMemory::moveData<T> ( _priv->buffer + storeIndex, &tmp, 0, 0 );
                }

                if ( ++storeIndex == _priv->bufSize )
                    storeIndex = 0;
            }
        }
    }

    // [store_index] -> [right] (which was 'unset')
    // pivot_value -> [store_index]

    SharedMemory::moveData<T> ( _priv->buffer + right, _priv->buffer, storeIndex, storeIndex );
    SharedMemory::moveData<T> ( _priv->buffer + storeIndex, &pivotValue, 0, 0 );

    return storeIndex;
}

template<typename T> void List<T>::quicksort (
        size_t left, size_t right,
        bool ( * compareFunc )( const T & a, const T & b ) )
{
    // Nothing to do
    if ( left == right )
        return;

    size_t pivotIndex;

    if ( left <= right )
    {
        pivotIndex = left / 2 + right / 2;

        assert ( pivotIndex >= left );
        assert ( pivotIndex <= right );
    }
    else
    {
        pivotIndex = ( _priv->bufSize / 2 + left / 2 + right / 2 ) % _priv->bufSize;

        assert ( pivotIndex < _priv->bufSize );
        assert ( pivotIndex <= right || pivotIndex >= left );
    }

    pivotIndex = partition ( left, right, pivotIndex, compareFunc );

    // Recursively sort elements smaller than the pivot
    if ( left != pivotIndex )
    {
        if ( pivotIndex > 0 )
        {
            quicksort ( left, pivotIndex - 1, compareFunc );
        }
        else
        {
            quicksort ( left, _priv->bufSize - 1, compareFunc );
        }
    }

    // Recursively sort elements at least as big as the pivot
    if ( right != pivotIndex )
    {
        if ( pivotIndex < _priv->bufSize - 1 )
        {
            quicksort ( pivotIndex + 1, right, compareFunc );
        }
        else
        {
            assert ( pivotIndex == _priv->bufSize - 1 );

            quicksort ( 0, right, compareFunc );
        }
    }
}

template<typename T> List<T> & List<T>::sort ( bool ( * compareFunc )( const T & a, const T & b ) )
{
    assert ( compareFunc != 0 );

    if ( !_priv || _priv->dataSize < 2 )
    {
        return *this;
    }

    ensureNotShared ( false );

    quicksort ( _priv->begIndex, _priv->endIndex, compareFunc );

    return *this;
}

template<typename T> List<T> & List<T>::sortAscending()
{
    return sort ( compareLT );
}

template<typename T> List<T> & List<T>::sortDescending()
{
    return sort ( compareGT );
}

template<typename T> bool List<T>::compareLT ( const T & a, const T & b )
{
    return ( a < b );
}

template<typename T> bool List<T>::compareGT ( const T & a, const T & b )
{
    return ( a > b );
}

DECLARE_EMBEDDED_TEMPLATE_TYPE_INFO ( List, TYPE_CONF_STD_SHARED );
}
