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
#include <stdint.h>
#include <sys/types.h>
}

#include <cstring>
#include <cassert>

#include "MemBlock.hpp"

namespace Pravala
{
/// @brief A helper struct for storing pointers to MemBlock, associated memory and the size of the memory.
/// It does not manage references automatically, but provides a number of helper functions.
struct MemData
{
    MemBlock * block;         ///< Pointer to the memory block associated with the memory.
    char * mem;               ///< Pointer to the beginning of the payload memory described by the block.
    size_t size;              ///< The size of the payload memory.

    /// @brief Default constructor.
    inline MemData(): block ( 0 ), mem ( 0 ), size ( 0 )
    {
    }

    /// @brief Constructor.
    /// It does not modify references, just copies all values.
    /// @note It requires the values to make sense - either all 0, or all not 0.
    /// @param [in] b Pointer to the memory block associated with the memory.
    /// @param [in] m Pointer to the beginning of the payload memory described by the block.
    /// @param [in] s The size of the payload memory.
    inline MemData ( MemBlock * b, char * m, size_t s ): block ( b ), mem ( m ), size ( s )
    {
        assert ( ( !block && !mem && size == 0 ) || ( block != 0 && mem != 0 && size > 0 ) );
    }

    /// @brief Clears the content of this object.
    /// It does NOT unreference anything, just sets all the fields to 0.
    inline void clear()
    {
        block = 0;
        mem = 0;
        size = 0;
    }

    /// @brief A helper function that releases the memory.
    /// If this object is empty, nothing happens.
    /// Otherwise it unreferences underlying memory block and clears the state of this object.
    inline void unref()
    {
        if ( block != 0 )
        {
            block->unref();
            clear();
        }
#ifndef NDEBUG
        else
        {
            assert ( !mem );
            assert ( size == 0 );
        }
#endif
    }

    /// @brief Increments memory block's reference counter.
    /// If this object is empty, nothing happens and 'false' is returned.
    /// If it cannot create a new reference, it will create a brand new copy of the data,
    /// but without unreferencing the original data.
    /// @return True on success (which also means that this MemData has a memory block);
    ///         False if the memory couldn't be referenced or copied. This will also empty MemData object.
    inline bool ref()
    {
        if ( !block )
        {
            return false;
        }
        else if ( !block->ref() && !intMakeCopy ( false ) )
        {
            // We pass 'false' to intMakeCopy so it wouldn't release the original reference.
            // This is because we only call it if we fail to obtain that reference.
            clear();
            return false;
        }
        else
        {
            return true;
        }
    }

    /// @brief Replaces content of MemData with another MemData object.
    /// It will create a new reference in the new data (if needed),
    /// and remove the reference from the old data (if needed).
    /// @param [in] what The new content to set.
    /// @return True on success (also when 'what' was empty);
    ///         False if the memory couldn't be referenced or copied. This will also empty MemData object.
    inline bool replaceWith ( const MemData & what )
    {
        if ( what.block == block )
        {
            // The same memory block (or empty).
            // No need to modify references, but 'mem' and/or 'size' may need to be changed.
            // For simplicity we just copy the entire thing:
            *this = what;
            return true;
        }

        // Different memory block.
        // We need to release current one, set the new one, and reference it.

        if ( block != 0 )
        {
            // We call it directly, to skip memset (we are overwriting everything anyway).
            block->unref();
        }

        *this = what;

        // Note: if 'what' is empty we want to return 'true', even though 'ref()' would fail:
        return ( !block || ref() );
    }

    /// @brief Prepares the memory to be written into.
    /// If the memory type does not allow writing (TypeMMapRO),
    /// or if there are more than one references a copy of the data will be created.
    /// If copying fails, this will return false, and the original data will be intact.
    /// @return True if the underlying memory can be written into; False otherwise (also when object is empty).
    inline bool ensureWritable()
    {
        if ( !block )
        {
            assert ( !mem );
            assert ( size == 0 );

            return false;
        }

        if ( block->getRefCount() > 1 || block->usesReadOnlyType() )
        {
            return intMakeCopy ( true );
        }

        assert ( block != 0 );
        assert ( mem != 0 );
        assert ( size > 0 );

        return true;
    }

    private:
        /// @brief Creates a new copy of the memory and points this object at the copied data.
        /// It tries to use the same memory type (if possible).
        /// @param [in] unrefOrg If true, original block will be unreferenced;
        ///                      If false, original block will be simply abandoned.
        /// @return True if the data was copied;
        ///         False if it couldn't be copied - in that case the original will be intact.
        bool intMakeCopy ( bool unrefOrg );

        friend class MemVector;
};
}
