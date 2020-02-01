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
#include <cstring>
#include <cassert>

namespace Pravala
{
/// @brief SimpleArray class.
/// Offers very simple auto-resizeable array.
/// While resizing, values inside are simply copied using memory operation.
/// To be used by very simple types (numbers, structs with numbers), or pointers.
/// copy constructor and operator= are not available.
///
/// @warning This SHOULD NEVER EVER be used for types that have any of these properties:
/// - contain classes using/inheriting virtual functions
/// - require the use of a destructor to clean up
/// - require the use of a copy constructor
/// (directly, storing pointers to such classes is fine)
///
/// @tparam T The type to be stored in this array.
/// @tparam S The type to be used for this array's size.
template<typename T, typename S = size_t> class SimpleArray
{
    public:
        /// @brief A type that represents a pointer to comparison function.
        /// That function is used for sorting.
        /// It should return 'true' when 'a' should be IN FRONT of 'b' while sorting in ascending order.
        /// If '<' (or '<=') is used, the smallest element will be placed at the beginning.
        typedef bool (* CompareFunctionType) ( const T & a, const T & b );

        /// @brief Constructor.
        /// Creates an empty SimpleArray. All the elements are initialized to 0.
        /// @param [in] initSize The initial number of elements to allocate the memory for.
        ///                      This is just for memory allocation, the actual used size will be 0.
        inline explicit SimpleArray ( S initSize = 0 ):
            _values ( 0 ),
            _allocSize ( initSize ),
            _usedSize ( 0 )
        {
            if ( _allocSize > 0 )
            {
                _values = static_cast<T *> ( calloc ( _allocSize, sizeof ( T ) ) );
            }
        }

        /// @brief Copy constructor.
        /// Be careful when using it for SimpleArrays that store pointers, since this just creates a copy of the data
        /// (so in that case a copy of pointers, not a copy of the actual objects).
        /// Deleting objects from one SimpleArray will make the objects
        /// pointed to by pointers in the other SimpleArray invalid!
        /// @param [in] other The array to copy.
        SimpleArray ( const SimpleArray & other ):
            _values ( 0 ),
            _allocSize ( other._usedSize ),
            _usedSize ( other._usedSize )
        {
            if ( _allocSize > 0 )
            {
                // We only allocate the memory sufficient to store the original array.
                // We could allocate more, but chances are this is all we will ever need.

                assert ( other._values != 0 );

                _values = static_cast<T *> ( malloc ( _allocSize * sizeof ( T ) ) );

                memcpy ( _values, other._values, _usedSize * sizeof ( T ) );
            }
        }

        /// @brief Assignment operator.
        /// Be careful when using it for SimpleArrays that store pointers, since this just creates a copy of the data
        /// (so in that case a copy of pointers, not a copy of the actual objects).
        /// Deleting objects from one SimpleArray will make the objects
        /// pointed to by pointers in the other SimpleArray invalid!
        /// @param [in] other The original SimpleArray to create a copy of.
        /// @return A reference to this SimpleArray.
        SimpleArray & operator= ( const SimpleArray & other )
        {
            if ( &other == this )
                return *this;

            // The new content will overwrite our current content.
            // But in case 'other' has less elements, we need to clear everything past that.
            truncate ( other._usedSize );

            ensureSizeAllocated ( other._usedSize );

            // To make sure things got cleared (if needed)
            assert ( _usedSize <= other._usedSize );

            _usedSize = other._usedSize;

            assert ( _allocSize > 0 );
            assert ( _allocSize >= _usedSize );
            assert ( _allocSize >= other._usedSize );

            if ( _usedSize > 0 )
            {
                memcpy ( _values, other._values, _usedSize * sizeof ( T ) );
            }

            return *this;
        }

        /// @brief Destructor.
        /// It simply deallocates the memory.
        /// It doesn't do anything fancy, so if the SimpleArray contains pointers to other objects,
        /// they will not be destructed!
        ~SimpleArray()
        {
            free ( _values );

            _values = 0;
            _allocSize = _usedSize = 0;
        }

        /// @brief Returns the size of the allocated memory.
        /// @return Total number of elements for which the memory has been allocated.
        inline S sizeAllocated() const
        {
            return _allocSize;
        }

        /// @brief Returns size of the array.
        /// @return The number of elements stored in the array.
        inline S size() const
        {
            return _usedSize;
        }

        /// @brief Ensures that the allocated size of the array is equal (or greater) to the size given
        ///
        /// @param [in] minAllocSize The minimum allocated size this array should have. If it is smaller than
        ///                         or equal to the currently allocated size, then it does nothing.
        ///                         Otherwise it reallocates the memory to at least the size given. It could
        ///                         allocate more.
        /// @param [in] maxAllocSizeHint Preferred max memory size (only if it's larger or equal to minAllocSize)
        ///                      It's a hint for the memory reallocator if we know what is the max
        ///                      size we will ever use. SimpleArray, in general, allocates more memory
        ///                      than needed, to avoid often reallocations. But in some situations
        ///                      we know what is the max size we ever are going to use. In those
        ///                      situation we pass this max size to this function, but this is just
        ///                      a hint that could be ignored. By default it is disabled (= 0).
        void ensureSizeAllocated ( S minAllocSize, S maxAllocSizeHint = 0 )
        {
            if ( minAllocSize <= _allocSize )
            {
                // No need to do anything.
                return;
            }

            // Let's increase the size 1.5x.
            _allocSize *= 3;
            _allocSize /= 2;

            if ( _allocSize < minAllocSize )
            {
                _allocSize = minAllocSize;
            }
            else if ( maxAllocSizeHint >= minAllocSize && _allocSize > maxAllocSizeHint )
            {
                // We have a memory size hint that we can use (>= minAllocSize),
                // and we would allocate too much memory. Let's not allocate too much.
                _allocSize = maxAllocSizeHint;
            }

            assert ( _allocSize > _usedSize );
            assert ( _allocSize >= minAllocSize );

            _values = static_cast<T *> ( realloc ( _values, _allocSize * sizeof ( T ) ) );

            if ( !_values )
            {
                abort();
            }

            assert ( _values != 0 );
            assert ( _allocSize > _usedSize );

            memset ( _values + _usedSize, 0, ( _allocSize - _usedSize ) * sizeof ( T ) );
        }

        /// @brief Ensures that the "used" size of the array is equal (or greater) to the size given
        ///
        /// It reallocates the memory if necessary using ensureSizeAllocated()
        /// @param [in] minUsedSize The minimum "used" size this array should have. If it is smaller than
        ///                         or equal to the current "used" size, then it does nothing.
        ///                         Otherwise it increases the size used to the size given.
        /// @param [in] maxAllocSizeHint Preferred max memory size. See comment for ensureSizeAllocated()
        void ensureSizeUsed ( S minUsedSize, S maxAllocSizeHint = 0 )
        {
            if ( minUsedSize > _allocSize )
            {
                ensureSizeAllocated ( minUsedSize, maxAllocSizeHint );
            }

            assert ( minUsedSize <= _allocSize );

            if ( _usedSize < minUsedSize )
            {
                _usedSize = minUsedSize;
            }
        }

        /// @brief Returns reference to the element in specified position. It is created if it doesn't exist.
        ///
        /// It reallocates the memory if necessary using ensureSizeUsed() (and, consequently, ensureSizeAllocated())
        ///
        /// @param [in] index Index of the element to access.
        /// @param [in] maxAllocSizeHint Preferred max memory size. See comment for ensureSizeAllocated()
        /// @return The reference to the specified element.
        T & getOrCreate ( S index, S maxAllocSizeHint = 0 )
        {
            // The index CANNOT be a const S &!
            // It HAS to be a copy. Otherwise we can't use size()
            // for accessing data!
            // Calling arr.getOrCreate(arr.size()) = foo;
            // would crash because _usedSize would change while
            // running ensureSizeUsed.

            ensureSizeUsed ( index + 1, maxAllocSizeHint );

            assert ( index < _usedSize );

            return _values[ index ];
        }

        /// @brief Appends a value at the end of the array.
        /// @param [in] value Value to append.
        inline void append ( const T & value )
        {
            getOrCreate ( _usedSize ) = value;
        }

        /// @brief Inserts a value at the beginning of the array.
        /// @param [in] value Value to insert.
        inline void prepend ( const T & value )
        {
            const S orgUsedSize = _usedSize;

            ensureSizeUsed ( orgUsedSize + 1 );

            assert ( _usedSize == orgUsedSize + 1 );

            if ( orgUsedSize > 0 )
            {
                memmove ( _values + 1, _values, orgUsedSize * sizeof ( T ) );
            }

            _values[ 0 ] = value;
        }

        /// @brief Returns reference to the element in specified position.
        ///
        /// This function does not check if the provided index is in the valid range,
        /// and does not reallocate anything. It should be used when the size is known,
        /// for example in loops iterating from a certain index to size() - 1.
        /// If the size is unknown, or the index may need to be created, getOrCreate()
        /// should be used instead.
        ///
        /// @param [in] index Index of the element to access. Has to be valid!
        /// @return The reference to the specified element.
        inline T & operator[] ( S index )
        {
            assert ( index < size() );

            return _values[ index ];
        }

        /// @brief Returns const reference to the element in specified position.
        ///
        /// This function does not check if the provided index is in the valid range,
        /// and does not reallocate anything. It should be used when the size is known,
        /// for example in loops iterating from a certain index to size() - 1.
        /// If the size is unknown, or the index may need to be created, getOrCreate()
        /// should be used instead.
        ///
        /// @param [in] index Index of the element to access. Has to be valid!
        /// @return Const reference to the specified element.
        inline const T & operator[] ( S index ) const
        {
            assert ( index < size() );

            return _values[ index ];
        }

        /// @brief Finds the position of an element
        /// @param [in] value Element to find
        /// @param [out] foundIndex The index at which the given element has been found (if used).
        ///                          It is not set if the element is not found.
        /// @param [in] startIndex Index to begin searching from
        /// @return true if the element was found; false otherwise
        inline bool findValue ( const T & value, S * foundIndex = 0, S startIndex = 0 ) const
        {
            for ( S idx = startIndex; idx < size(); ++idx )
            {
                if ( _values[ idx ] == value )
                {
                    if ( foundIndex != 0 )
                        *foundIndex = idx;

                    return true;
                }
            }

            return false;
        }

        /// @brief Compares two SimpleArrays by comparing their contents
        /// @param [in] other The SimpleArray to compare against
        /// @return True if the contents of the SimpleArrays are the same
        inline bool operator== ( const SimpleArray & other ) const
        {
            if ( size() != other.size() )
            {
                return false;
            }

            for ( S i = 0; i < size(); ++i )
            {
                if ( _values[ i ] != other._values[ i ] )
                {
                    return false;
                }
            }

            return true;
        }

        /// @brief Compares two SimpleArrays by comparing their contents
        /// @param [in] other The SimpleArray to compare against
        /// @return True if the contents of the SimpleArrays are different
        inline bool operator!= ( const SimpleArray & other ) const
        {
            return !( this == other );
        }

        /// @brief Truncates the array.
        /// It will clear the memory in the truncated segment.
        /// @param [in] newSize The new size. If it's greater than or equal to current size, nothing happens.
        inline void truncate ( S newSize )
        {
            if ( newSize < _usedSize )
            {
                memset ( _values + newSize, 0, ( _usedSize - newSize ) * sizeof ( T ) );
                _usedSize = newSize;
            }
        }

        /// @brief Removes given number of elements from the beginning of the array.
        /// All remaining elements will be shifted left.
        /// @param [in] sizeToTrim The number of elements at the beginning to remove.
        ///                        If it's greater than or equal to current size, the array will be cleared.
        inline void leftTrim ( S sizeToTrim )
        {
            if ( sizeToTrim < 1 )
                return;

            if ( sizeToTrim < _usedSize )
            {
                _usedSize -= sizeToTrim;

                memmove ( _values, _values + sizeToTrim, _usedSize * sizeof ( T ) );
                memset ( _values + _usedSize, 0, sizeToTrim * sizeof ( T ) );
            }
            else
            {
                clear();
            }
        }

        /// @brief This decreases the 'size' property.
        ///
        /// The index passed is the index of the element that can be unset.
        /// The last used element will be used to overwrite the index being unset
        /// (using memcpy), and the value of size will be decreased.
        /// The last element past the new size will be set to 0 using memset.
        ///
        /// @warning This moves elements around, so should not be called on an
        /// array where element's position is their ID or important in some way.
        ///
        /// @param [in] indexToUnset Index of the element to be overwritten by the
        /// last used one. If negative, the last element is removed.
        inline void shrinkArray ( S indexToUnset )
        {
            assert ( indexToUnset < _usedSize );

            if ( _usedSize < 1 ) return;

            --_usedSize;

            if ( indexToUnset != _usedSize )
            {
                memcpy ( _values + indexToUnset, _values + _usedSize, sizeof ( T ) );
            }

            memset ( _values + _usedSize, 0, sizeof ( T ) );
        }

        /// @brief This sets the memory used by the specified index to 0
        ///
        /// The index passed is the index of the element that will be set
        /// (using memset) to 0. The index has to be a legal one.
        /// This function does not modify the size.
        ///
        /// @param [in] indexToZero Index of the element to be set to 0.
        inline void memsetZero ( S indexToZero )
        {
            assert ( indexToZero < _usedSize );

            if ( _usedSize < 1 ) return;

            if ( indexToZero >= _usedSize )
                abort();

            memset ( _values + indexToZero, 0, sizeof ( T ) );
        }

        /// @brief Clears the array
        ///
        /// Sets used size to 0. Memsets content to 0.
        /// Does not deallocate the memory or change the size of the memory.
        inline void clear()
        {
            if ( _usedSize > 0 )
            {
                memset ( _values, 0, _usedSize * sizeof ( T ) );
                _usedSize = 0;
            }
        }

        /// @brief Returns constant pointer to the actual memory where the values are stored
        /// @return constant pointer to the actual memory where the values are stored
        inline const T * getMemory() const
        {
            return _values;
        }

        /// @brief Returns writable pointer to the actual memory where the values are stored
        /// @return writable pointer to the actual memory where the values are stored
        inline T * getWritableMemory()
        {
            return _values;
        }

        /// @brief Sorts the array using the function provided
        /// @param [in] compareFunc The function to be used for sorting.
        ///                         It should return 'true' when 'a' should be IN FRONT of 'b'.
        ///                         If '<' (or '<=') is used, the smallest element will be placed at the beginning.
        inline void sort ( CompareFunctionType compareFunc )
        {
            if ( !compareFunc || _usedSize < 1 )
            {
                return;
            }

            quicksort ( 0, _usedSize - 1, compareFunc );
        }

        /// @brief Sorts the array in ascending order using < operator
        inline void sortAscending()
        {
            sort ( compareLT );
        }

        /// @brief Sorts the array in descending order using > operator
        inline void sortDescending()
        {
            sort ( compareGT );
        }

        /// @brief Less-than comparison function.
        /// @param [in] a First value to compare.
        /// @param [in] b Second value to compare.
        /// @return True if a < b.
        static bool compareLT ( const T & a, const T & b )
        {
            return ( a < b );
        }

        /// @brief Greater-than comparison function.
        /// @param [in] a First value to compare.
        /// @param [in] b Second value to compare.
        /// @return True if a > b.
        static bool compareGT ( const T & a, const T & b )
        {
            return ( a > b );
        }

    private:
        T * _values;       ///< The underlying memory.
        S _allocSize; ///< The number of elements allocated.
        S _usedSize;  ///< The number of entries that are actually used.

        /// @brief Perform's quick sort's partitioning
        /// @param [in] left The left index
        /// @param [in] right The right index
        /// @param [in] pivotIndex The 'pivot' index
        /// @param [in] compareFunc The function used for comparison. See 'sort()'.
        /// @return The new pivot index
        inline S partition ( S left, S right, S pivotIndex, CompareFunctionType compareFunc )
        {
            assert ( left < _usedSize );
            assert ( right < _usedSize );

            // [pivot_index] -> pivot_value
            // [right] -> [pivot_index] (after this the element at 'right' index will be INVALID!

            const T pivotValue = _values[ pivotIndex ];
            _values[ pivotIndex ] = _values[ right ];

            S storeIndex = left;

            for ( S idx = left; idx < right; ++idx )
            {
                if ( compareFunc ( _values[ idx ], pivotValue ) )
                {
                    if ( idx != storeIndex )
                    {
                        assert ( storeIndex < _usedSize );

                        // Switch [idx] <-> [storeIndex]:
                        // [idx] -> tmp
                        // [store_index] -> [idx]
                        // tmp -> [store_index]

                        const T tmp = _values[ idx ];
                        _values[ idx ] = _values[ storeIndex ];
                        _values[ storeIndex ] = tmp;
                    }

                    ++storeIndex;
                }
            }

            assert ( storeIndex < _usedSize );

            // [store_index] -> [right] (which was 'unset')
            // pivot_value -> [store_index]

            _values[ right ] = _values[ storeIndex ];
            _values[ storeIndex ] = pivotValue;

            return storeIndex;
        }

        /// @brief Runs quick sort on the range.
        /// @param [in] left The left index
        /// @param [in] right The right index
        /// @param [in] compareFunc The function used for comparison. See 'sort()'.
        inline void quicksort ( S left, S right, CompareFunctionType compareFunc )
        {
            // Nothing to do
            if ( left >= right )
                return;

            S pivotIndex = partition ( left, right, left / 2 + right / 2, compareFunc );

            // Recursively sort elements smaller than the pivot value
            if ( left < pivotIndex )
            {
                quicksort ( left, pivotIndex - 1, compareFunc );
            }

            // Recursively sort elements at least as big as the pivot value
            if ( right > pivotIndex )
            {
                quicksort ( pivotIndex + 1, right, compareFunc );
            }
        }
};
}
