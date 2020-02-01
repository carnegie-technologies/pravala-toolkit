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
/// @brief TupleArray class.
///
/// Offers an auto-resizeable array of two data values.
///
/// It is similar to SimpleArray, but it can be used for storing two different data sets at the same time.
/// Each set will have the exact same size. The exact same thing could be achieved by using a SimpleArray
/// of structures with two values inside (one for each of those types).
/// The difference is, this class keeps both types as arrays of continuous memory.
///
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
///
/// @tparam A The type of the first value.
/// @tparam B The type of the second value.
/// @tparam S The type to be used for this array's size.
template<typename A, typename B, typename S = size_t> class TupleArray
{
    public:
        /// @brief A type that represents a pair of values stored in TupleArray.
        struct Tuple
        {
            A & first;  ///< First value stored in the TupleArray.
            B & second; ///< Second value stored in the TupleArray.

            /// @brief Constructor.
            /// @param [in] f First value.
            /// @param [in] s Second value.
            Tuple ( A & f, B & s ): first ( f ), second ( s )
            {
            }

            /// @brief Copy constructor.
            /// @param [in] other The object to copy.
            Tuple ( const Tuple & other ): first ( other.first ), second ( other.second )
            {
            }
        };

        /// @brief A type that represents a pair of values stored in TupleArray.
        struct ConstTuple
        {
            const A & first;  ///< First value stored in the TupleArray.
            const B & second; ///< Second value stored in the TupleArray.

            /// @brief Constructor.
            /// @param [in] f First value.
            /// @param [in] s Second value.
            ConstTuple ( const A & f, const B & s ): first ( f ), second ( s )
            {
            }

            /// @brief Copy constructor.
            /// @param [in] other The object to copy.
            ConstTuple ( const ConstTuple & other ): first ( other.first ), second ( other.second )
            {
            }

            /// @brief Copy constructor.
            /// @param [in] other The object to copy.
            ConstTuple ( const Tuple & other ): first ( other.first ), second ( other.second )
            {
            }
        };

        /// @brief Constructor.
        /// Creates an empty TupleArray. All the elements are initialized to 0.
        /// @param [in] initSize The initial number of elements to allocate the memory for.
        ///                      This is just for memory allocation, the actual used size will be 0.
        inline explicit TupleArray ( S initSize = 0 ):
            _values ( 0 ),
            _allocSize ( initSize ),
            _usedSize ( 0 )
        {
            if ( _allocSize > 0 )
            {
                _values = calloc ( 1, getTotalMemSize ( _allocSize ) );
            }
        }

        /// @brief Copy constructor.
        /// Be careful when using it for TupleArrays that store pointers, since this just creates a copy of the data
        /// (so in that case a copy of pointers, not a copy of the actual objects).
        /// Deleting objects from one TupleArray will make the objects
        /// pointed to by pointers in the other TupleArray invalid!
        /// @param [in] other The array to copy.
        TupleArray ( const TupleArray & other ):
            _values ( 0 ),
            _allocSize ( other._usedSize ),
            _usedSize ( other._usedSize )
        {
            if ( _allocSize > 0 )
            {
                // We only allocate the memory sufficient to store the original array.
                // We could allocate more, but chances are this is all we will ever need.

                assert ( other._values != 0 );

                _values = malloc ( getTotalMemSize ( _allocSize ) );

                memcpy ( getFirstWritableMemory(), other.getFirstMemory(), _usedSize * sizeof ( A ) );
                memcpy ( getSecondWritableMemory(), other.getSecondMemory(), _usedSize * sizeof ( B ) );
            }
        }

        /// @brief Assignment operator.
        /// Be careful when using it for TupleArrays that store pointers, since this just creates a copy of the data
        /// (so in that case a copy of pointers, not a copy of the actual objects).
        /// Deleting objects from one TupleArray will make the objects
        /// pointed to by pointers in the other TupleArray invalid!
        /// @param [in] other The original TupleArray to create a copy of.
        /// @return A reference to this TupleArray.
        TupleArray & operator= ( const TupleArray & other )
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
                memcpy ( getFirstWritableMemory(), other.getFirstMemory(), _usedSize * sizeof ( A ) );
                memcpy ( getSecondWritableMemory(), other.getSecondMemory(), _usedSize * sizeof ( B ) );
            }

            return *this;
        }

        /// @brief Destructor.
        /// It simply deallocates the memory.
        /// It doesn't do anything fancy, so if the TupleArray contains pointers to other objects,
        /// they will not be destructed!
        ~TupleArray()
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

        /// @brief Compares two TupleArrays by comparing their contents
        /// @param [in] other The TupleArray to compare against
        /// @return True if the contents of the TupleArrays are the same
        inline bool operator== ( const TupleArray & other ) const
        {
            if ( _usedSize != other.size() )
            {
                return false;
            }

            {
                const A * const mem = getFirstMemory();
                const A * const oMem = other.getFirstMemory();

                for ( S i = 0; i < _usedSize; ++i )
                {
                    if ( mem[ i ] != oMem[ i ] )
                        return false;
                }
            }

            {
                const B * const mem = getSecondMemory();
                const B * const oMem = other.getSecondMemory();

                for ( S i = 0; i < _usedSize; ++i )
                {
                    if ( mem[ i ] != oMem[ i ] )
                        return false;
                }
            }

            return true;
        }

        /// @brief Compares two TupleArrays by comparing their contents
        /// @param [in] other The TupleArray to compare against
        /// @return True if the contents of the TupleArrays are different
        inline bool operator!= ( const TupleArray & other ) const
        {
            return !( this == other );
        }

        /// @brief Ensures that the allocated size of the array is equal (or greater) to the size given
        ///
        /// @param [in] minAllocSize The minimum allocated size this array should have. If it is smaller than
        ///                         or equal to the currently allocated size, then it does nothing.
        ///                         Otherwise it reallocates the memory to at least the size given. It could
        ///                         allocate more.
        /// @param [in] maxAllocSizeHint Preferred max memory size (only if it's larger or equal to minAllocSize)
        ///                      It's a hint for the memory reallocator if we know what is the max
        ///                      size we will ever use. TupleArray, in general, allocates more memory
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

            // We need to do this BEFORE we change _allocSize!
            const B * const secondMem = getSecondMemory();

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

            if ( _usedSize < 1 )
            {
                // We only use realloc if there is no existing data.
                // If we used it with existing data, we would have to move the second array anyway...

                _values = realloc ( _values, getTotalMemSize ( _allocSize ) );

                if ( !_values )
                {
                    abort();
                }

                memset ( _values, 0, getTotalMemSize ( _allocSize ) );
                return;
            }

            // We have to allocate new memory and copy the data manually.
            void * const oldValues = _values;

            _values = calloc ( 1, getTotalMemSize ( _allocSize ) );

            if ( !_values )
            {
                abort();
            }

            assert ( oldValues != 0 );
            assert ( secondMem != 0 );
            assert ( _values != 0 );
            assert ( _allocSize > _usedSize );

#if ( defined NDEBUG ) && ( ( __GNUC__ == 7 ) || ( ( __GNUC__ == 8 ) && ( __GNUC_MINOR__ <= 2 )  ) )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

            // New versions of GCC (verified with 7.2.0, 8.1.1, and 8.2.0) in release mode don't like following code.
            // They throw following errors:
            //
            // error: ‘void* memcpy(void*, const void*, size_t)’ pointer overflow
            //   between offset 0 and size [-8, 9223372036854775807] [-Werror=array-bounds]
            // error: ‘void* memcpy(void*, const void*, size_t)’ pointer overflow
            //   between offset [0, -4] and size [-16, 9223372036854775807] [-Werror=array-bounds]
            // error: ‘void* memcpy(void*, const void*, size_t)’ specified size 18446744073709551608 exceeds
            //   maximum object size 9223372036854775807 [-Werror=stringop-overflow=]
            // error: ‘void* memcpy(void*, const void*, size_t)’ specified size 18446744073709551600 exceeds
            //   maximum object size 9223372036854775807 [-Werror=stringop-overflow=]
            //
            // Essentially, gcc thinks that _usedSize=((size_t) -1)) - so really large.
            // However, the code has been extensively studied, and that should never happen.
            // Also, clang and Windows compilers don't have any issues with it.
            // So we silence those warnings around this piece of code when compiling with GCC>=7:

            // Copy existing data.
            // NOTE: For the second array we use the pointer we obtained BEFORE changing _allocSize!
            //       Otherwise it would be (wrongly) calculated using the new _allocSize.
            memcpy ( getFirstWritableMemory(), oldValues, _usedSize * sizeof ( A ) );
            memcpy ( getSecondWritableMemory(), secondMem, _usedSize * sizeof ( B ) );

#if ( defined NDEBUG ) && ( ( __GNUC__ == 7 ) || ( ( __GNUC__ == 8 ) && ( __GNUC_MINOR__ <= 2 )  ) )
#pragma GCC diagnostic pop
#endif

            if ( _usedSize < _allocSize )
            {
                // Clear unused entries:
                memset ( getFirstWritableMemory() + _usedSize, 0, ( _allocSize - _usedSize ) * sizeof ( A ) );
                memset ( getSecondWritableMemory() + _usedSize, 0, ( _allocSize - _usedSize ) * sizeof ( B ) );
            }

            free ( oldValues );
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

        /// @brief Returns a const reference to first of the elements stored at specified position.
        /// @param [in] index Index of the element to get; Has to be valid!
        /// @return Const reference to first of the elements stored at specified position.
        inline const A & getFirst ( S index ) const
        {
            assert ( index < _usedSize );

            return getFirstMemory()[ index ];
        }

        /// @brief Returns a reference to first of the elements stored at specified position.
        /// @param [in] index Index of the element to get; Has to be valid!
        /// @return Reference to first of the elements stored at specified position.
        inline A & getFirst ( S index )
        {
            assert ( index < _usedSize );

            return getFirstWritableMemory()[ index ];
        }

        /// @brief Returns a const reference to second of the elements stored at specified position.
        /// @param [in] index Index of the element to get; Has to be valid!
        /// @return Const reference to second of the elements stored at specified position.
        inline const B & getSecond ( S index ) const
        {
            assert ( index < _usedSize );

            return getSecondMemory()[ index ];
        }

        /// @brief Returns a reference to second of the elements stored at specified position.
        /// @param [in] index Index of the element to get; Has to be valid!
        /// @return Reference to second of the elements stored at specified position.
        inline B & getSecond ( S index )
        {
            assert ( index < _usedSize );

            return getSecondWritableMemory()[ index ];
        }

        /// @brief Returns a tuple with const references to values at specified position.
        ///
        /// This function does not check if the provided index is in the valid range,
        /// and does not reallocate anything. It should be used when the size is known,
        /// for example in loops iterating from a certain index to size() - 1.
        ///
        /// @param [in] index Index of the element to access; Has to be valid!
        /// @return A tuple with const references to values at specified position.
        inline ConstTuple operator[] ( S index ) const
        {
            assert ( index < _usedSize );

            return ConstTuple ( getFirstMemory()[ index ], getSecondMemory()[ index ] );
        }

        /// @brief Returns a tuple with references to values at specified position.
        ///
        /// This function does not check if the provided index is in the valid range,
        /// and does not reallocate anything. It should be used when the size is known,
        /// for example in loops iterating from a certain index to size() - 1.
        ///
        /// @param [in] index Index of the element to access; Has to be valid!
        /// @return A tuple with references to values at specified position.
        inline Tuple operator[] ( S index )
        {
            assert ( index < _usedSize );

            return Tuple ( getFirstWritableMemory()[ index ], getSecondWritableMemory()[ index ] );
        }

        /// @brief Sets a pair of values in the array.
        /// @param [in] index Index of the element to set; Has to be valid!
        /// @param [in] first Value to set in first internal array.
        /// @param [in] second Value to set in second internal array.
        inline void set ( S index, const A & first, const B & second )
        {
            assert ( index < _usedSize );

            getFirstWritableMemory()[ index ] = first;
            getSecondWritableMemory()[ index ] = second;
        }

        /// @brief Appends a pair of values at the end of the array.
        /// @param [in] first Value to append to first internal array.
        /// @param [in] second Value to append to second internal array.
        /// @param [in] maxAllocSizeHint Preferred max memory size; See comment for ensureSizeAllocated()
        inline void append ( const A & first, const B & second, S maxAllocSizeHint = 0 )
        {
            const S index = _usedSize;

            ensureSizeUsed ( index + 1, maxAllocSizeHint );

            assert ( index < _usedSize );

            getFirstWritableMemory()[ index ] = first;
            getSecondWritableMemory()[ index ] = second;
        }

        /// @brief Inserts a value at the beginning of the array.
        /// @brief Inserts a pair of values at the beginning of the array.
        /// @param [in] first Value to append to first internal array.
        /// @param [in] second Value to append to second internal array.
        /// @param [in] maxAllocSizeHint Preferred max memory size; See comment for ensureSizeAllocated()
        inline void prepend ( const A & first, const B & second, S maxAllocSizeHint = 0 )
        {
            const S orgUsedSize = _usedSize;

            ensureSizeUsed ( orgUsedSize + 1, maxAllocSizeHint );

            assert ( _usedSize == orgUsedSize + 1 );

            if ( orgUsedSize > 0 )
            {
                {
                    A * const mem = getFirstWritableMemory();

                    memmove ( mem + 1, mem, orgUsedSize * sizeof ( A ) );
                }

                {
                    B * const mem = getSecondWritableMemory();

                    memmove ( mem + 1, mem, orgUsedSize * sizeof ( B ) );
                }
            }

            getFirstWritableMemory()[ 0 ] = first;
            getSecondWritableMemory()[ 0 ] = second;
        }

        /// @brief Truncates the array.
        /// It will clear the memory in the truncated segment.
        /// @param [in] newSize The new size. If it's greater than or equal to current size, nothing happens.
        inline void truncate ( S newSize )
        {
            if ( newSize < _usedSize )
            {
                memset ( getFirstWritableMemory() + newSize, 0, ( _usedSize - newSize ) * sizeof ( A ) );
                memset ( getSecondWritableMemory() + newSize, 0, ( _usedSize - newSize ) * sizeof ( B ) );

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

            if ( _usedSize <= sizeToTrim )
            {
                clear();
                return;
            }

            _usedSize -= sizeToTrim;

            {
                A * const mem = getFirstWritableMemory();

                memmove ( mem, mem + sizeToTrim, _usedSize * sizeof ( A ) );
                memset ( mem + _usedSize, 0, sizeToTrim * sizeof ( A ) );
            }

            {
                B * const mem = getSecondWritableMemory();

                memmove ( mem, mem + sizeToTrim, _usedSize * sizeof ( B ) );
                memset ( mem + _usedSize, 0, sizeToTrim * sizeof ( B ) );
            }
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

            memset ( getFirstWritableMemory() + indexToZero, 0, sizeof ( A ) );
            memset ( getSecondWritableMemory() + indexToZero, 0, sizeof ( B ) );
        }

        /// @brief Clears the array
        ///
        /// Sets used size to 0. Memsets content to 0.
        /// Does not deallocate the memory or change the size of the memory.
        inline void clear()
        {
            if ( _usedSize > 0 )
            {
                memset ( getFirstWritableMemory(), 0, _usedSize * sizeof ( A ) );
                memset ( getSecondWritableMemory(), 0, _usedSize * sizeof ( B ) );

                _usedSize = 0;
            }
        }

        /// @brief Returns constant pointer to the continuous memory where 'A' values are stored.
        /// @return Constant pointer to the continuous memory where 'A' values are stored.
        inline const A * getFirstMemory() const
        {
            return static_cast<const A *> ( _values );
        }

        /// @brief Returns writable pointer to the continuous memory where 'A' values are stored.
        /// @return Writable pointer to the continuous memory where 'A' values are stored.
        inline A * getFirstWritableMemory()
        {
            return static_cast<A *> ( _values );
        }

        /// @brief Returns constant pointer to the continuous memory where 'A' values are stored.
        /// @return Constant pointer to the continuous memory where 'A' values are stored.
        inline const B * getSecondMemory() const
        {
            return ( _values != 0 )
                   ? ( reinterpret_cast<const B *> (
                           static_cast<const char *> ( _values ) + getFirstMemSize ( _allocSize ) ) )
                   : 0;
        }

        /// @brief Returns writable pointer to the continuous memory where 'B' values are stored.
        /// @return Writable pointer to the continuous memory where 'B' values are stored.
        inline B * getSecondWritableMemory()
        {
            return ( _values != 0 )
                   ? ( reinterpret_cast<B *> (
                           static_cast<char *> ( _values ) + getFirstMemSize ( _allocSize ) ) )
                   : 0;
        }

    private:
        void * _values;    ///< The underlying memory.
        S _allocSize; ///< The number of elements allocated.
        S _usedSize;  ///< The number of entries that are actually used.

        /// @brief Returns the size of the memory (in bytes) needed for the array of 'A' values.
        /// It rounds up the size to ensure proper alignment of the array of 'B' values.
        /// @note Returned value is the size of the memory (not number of elements) so it always uses size_t.
        /// @param [in] numElems The number of 'A' elements.
        /// @return The size of the memory (in bytes) needed to store those elements.
        inline static size_t getFirstMemSize ( S numElems )
        {
            // This rounds up the size to the next multiple of 4.
            // We need to do this to make sure that the array of 'B' elements is properly aligned
            // (in case they require proper alignment).
            // Possible options for the last 2 bits, +3 and &~3:
            // - size % 4 = 0: 00 + 11 = 11 & 1...100 = 00
            // - size % 4 = 1: 01 + 11 = 00 & 1...100 = 00
            // - size % 4 = 2: 10 + 11 = 01 & 1...100 = 00
            // - size % 4 = 3: 11 + 11 = 10 & 1...100 = 00
            return ( ( sizeof ( A ) * numElems + 3 ) & ~0x03 );
        }

        /// @brief Returns the total size of the memory (in bytes) needed for two arrays, of 'A' and 'B' values.
        /// It assumes necessary padding between 'A' and 'B' arrays.
        /// @note Returned value is the size of the memory (not number of elements) so it always uses size_t.
        /// @param [in] numElems The number of elements.
        /// @return The size of the memory (in bytes) needed to store those elements.
        inline static size_t getTotalMemSize ( S numElems )
        {
            // No need to pad anything after the second array.
            return getFirstMemSize ( numElems ) + sizeof ( B ) * numElems;
        }
};
}
