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

extern "C"
{
#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )
// We need winsock2 before windows.h, otherwise some things are defined wrong.
#include <winsock2.h>
#include <windows.h>
#endif

#include <stdint.h>
#include <sys/types.h>
}

#include <cassert>

namespace Pravala
{
class MemPool;

/// @brief Describes a block of data used by MemHandle and Buffer related classes.
/// The total size must be 4 byte aligned.
struct MemBlock
{
    /// @brief Value used for "type" field to determine what type of memory this object represents.
    enum BlockType
    {
        TypeUnknown     =  0,     ///< Unknown/not initialized type.
        TypeBuffer      =  1,     ///< Backed by a regular buffer. The payload is stored after the MemBlock.
        TypeMMapRO      =  2,     ///< Backed by a read-only mem-map.
        TypePool        =  3,     ///< Backed by a pool of blocks.
        TypeDeallocator =  4,     ///< Backed by external memory with custom deallocator.
        TypeMax                   ///< All types have to be less than this value.
    };

    /// @brief Initializes the memory block.
    /// It sets _refCount to 1 and configures type and tag of the block.
    /// @param [in] type Block type to set.
    /// @param [in] tag A tag value to assign to this block.
    ///                 The values are not in any way enforced, but to avoid conflicts,
    ///                 the values used should be assigned using assignTag().
    inline void init ( BlockType type, uint8_t tag = 0 )
    {
        _refCount = 1;
        _type = type;
        _tag = tag;
    }

    /// @brief Returns the type of the block.
    /// @return The type of the block.
    inline uint8_t getType() const
    {
        return _type;
    }

    /// @brief Returns true if this block uses read-only memory.
    /// @return True if this block uses read-only memory; False otherwise.
    inline bool usesReadOnlyType() const
    {
        // Right now it's only a single type.
        return ( _type == TypeMMapRO );
    }

    /// @brief Returns the tag value set on the block.
    /// @return The tag value set on the block.
    inline uint8_t getTag() const
    {
        return _tag;
    }

    /// @brief Increments reference counter.
    /// @return True if the new reference count is below (or equal) to the max limit we want to allow;
    ///         False if it would go over the limit. If it returns 'false', the reference counter was NOT incremented.
    inline bool ref()
    {
        assert ( _refCount > 0 );

        // We want to keep number of references below certain limit.
        // Otherwise creating too many references could cause the counter to overflow.

        if ( _refCount >= RefCountLimit )
        {
            return false;
        }

        if (
#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )
            InterlockedIncrement16 ( &_refCount ) <= RefCountLimit
#else
            __sync_add_and_fetch ( &_refCount, 1 ) <= RefCountLimit
#endif
        )
        {
            // The number of references is below (or at) the limit. All good!
            return true;
        }

        // We went over the limit. We need to remove the reference we created.
        unref();

        return false;
    }

    /// @brief Decrements block's reference counter.
    /// It also removes this block when the last reference is removed.
    /// This block should not be used by the caller after calling this function,
    /// unless it holds another reference to it.
    inline void unref()
    {
        assert ( _refCount > 0 );

        if (
#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )
            InterlockedDecrement16 ( &_refCount ) < 1
#else
            __sync_sub_and_fetch ( &_refCount, 1 ) < 1
#endif
        )
        {
            releaseBlock();
        }
    }

    /// @brief Returns the number of references.
    /// @return The number of references.
    inline uint16_t getRefCount() const
    {
        return _refCount;
    }

    /// @brief Returns the total size of the payload memory used by this block.
    /// @note The size may not be available in some block types, or on some platforms.
    /// @warning It should NEVER be used to determine how much memory can be stored in or read from the block.
    ///          This is just a hint about memory usage caused by this block.
    /// @return The total size of the underlying payload memory associated with this block; 0 if it is unknown.
    ///         It does not include the size of the block header or any memory used for book-keeping.
    size_t getMemorySize() const;

    /// @brief Assigns a new tag value.
    /// @return Assigned tag value (>0), or 0 if a new tag cannot be assigned.
    static uint8_t assignTag();

    protected:

#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )
        /// @brief The max counter value we want to allow.
        /// We use this to keep counters below certain value, to avoid overflowing.
        /// On Windows this limit is lower, because we have to use a signed type (to use atomic built-ins).
        /// We "reserve" about 4K of values below int16's limit (0x7FFF - 0x1000).
        static const SHORT RefCountLimit = 0x6FFF;

        volatile SHORT _refCount;     ///< Number of references to this block.
#else
        /// @brief The max counter value we want to allow.
        /// We use this to keep counters below certain value, to avoid overflowing.
        /// We "reserve" about 4K of values below uint16's limit (0xFFFF - 0x1000).
        static const uint16_t RefCountLimit = 0xEFFF;

        volatile uint16_t _refCount; ///< Number of references to this block.
#endif
        uint8_t _type;               ///< Block type.

        /// @brief A tag associated with this block.
        /// Blocks can be "tagged" with specific values.
        uint8_t _tag;

        /// @brief Releases the block once the last reference is removed.
        /// What exactly happens depend on the block type.
        /// @note This should be only called once the reference counter goes down to 0!
        void releaseBlock();
};

/// @brief Used if this block is backed by external memory.
/// It can be used directly by TypeMMapRO, or inherited by DeallocatorMemBlock.
struct ExternalMemBlock: public MemBlock
{
    char * data; ///< Pointer to where the data is located.
    size_t size; ///< The size of the data.
};

/// @brief Used if this block is backed by external memory with custom deallocator.
struct DeallocatorMemBlock: public ExternalMemBlock
{
    /// @brief A type that represents a pointer to the deallocator function.
    /// That function takes a pointer to DeallocatorMemBlock being released.
    /// It should release/deallocate underlying memory.
    /// @note It must NOT deallocate the block object itself.
    typedef void (* DeallocatorFunctionType) ( DeallocatorMemBlock * );

    DeallocatorFunctionType deallocator; ///< Pointer to deallocator function.
    void * deallocatorData; ///< User data that can be used to store additional state needed by the deallocator.
};

/// @brief To be used by blocks backed by memory pools.
/// When this block is used, it stores a pointer to memory pool that owns this block.
/// When this block is not used, it is used to store a pointer to the next block in the pool.
struct PoolMemBlock: public MemBlock
{
    union
    {
        /// @brief Memory pool that own this block.
        /// Only used when this block is NOT a part of a list of free blocks.
        MemPool * memPool;

        /// @brief Pointer to the next block in the pool.
        /// Only used when this block IS a part of a list of free blocks.
        PoolMemBlock * next;
    } u;
};
}
