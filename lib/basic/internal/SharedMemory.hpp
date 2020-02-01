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
#include <cstring>
#include <cstddef>
#include <new>

#if defined( __x86_64 ) || defined( __x86_64__ ) || defined( __aarch64__ ) || defined( __mips64 ) || defined( __LP64__ )
typedef uint64_t ptr_cast_t;
#else
typedef uint32_t ptr_cast_t;
#endif

namespace Pravala
{
/// @brief Configuration for different types
enum
{
    /// @brief Basic config.
    /// Used when storing pointers to values, or when directly (embedded)
    /// storing simple values of sizes not larger than a pointer (like integer).
    /// If a value is stored using a pointer to it, copy constructor and destructor
    /// is called anyway (on the created copy of the originally passed object).
    TYPE_CONF_ZERO = 0,

    /// @brief Means that the copy constructor will be used for copying embedded objects
    TYPE_CONF_CALL_COPY_CONSTR = 1,

    /// @brief When set, destructor is called in the embedded object.
    TYPE_CONF_CALL_DESTR = 2,

    /// @brief If used, copy constructor is called on move operations as well
    /// Destructor is called as well on the original copy (if CALL_DESTR is specified)
    TYPE_CONF_CALL_COPY_CONSTR_ON_MOVE = 4
};

/// @brief Default TypeInfo
/// By default, types are not embedded (so they are stored in containers using pointers to them)
/// And Config is not relevant, since pointers are easily moved around. Destructor is called
/// at the end, and copy constructor while copying data.
template<typename T> class TypeInfo
{
    public:
        enum
        {
            IsEmbedded = false,

            // Since IsEmbedded is set to false, this configuration doesn't matter anyway:
            TypeConfig = TYPE_CONF_ZERO

                         // Alternatively, if we wanted to store small complex objects
                         // directly (embedded) instead of storing pointers to them, it could be:
                         //
                         // IsEmbedded = (sizeof ( T ) <= sizeof ( void* ) ),
                         // TypeConfig = (sizeof ( T ) <= sizeof ( void* ) )?
                         // (TYPE_CONF_CALL_COPY_CONSTR|TYPE_CONF_CALL_DESTR|TYPE_CONF_CALL_COPY_CONSTR_ON_MOVE):
                         // TYPE_CONF_ZERO
        };
};

/// @brief TypeInfo for all pointer types
/// Pointers are handled directly using memory copying. No constructors or destructors
/// are called (its a responsibility of a user of a container that stores pointers)
template<typename T> class TypeInfo<T *>
{
    public:
        // Maybe we want TYPE_CONF_INIT_ZERO here to have all
        // unitialized pointer types initialized to 0 automatically?
        enum
        {
            IsEmbedded = true,
            TypeConfig = TYPE_CONF_ZERO
        };
};

/// @brief Standard config for data with implicit sharing (String, containers)
/// Copy constructor and destructor are called, but when reallocating, they can be
/// simply copied using memory copy.
#define TYPE_CONF_STD_SHARED    ( TYPE_CONF_CALL_COPY_CONSTR | TYPE_CONF_CALL_DESTR )

/// @brief Macro for declaring "embedded" types
/// Direct types are treated as direct only if their size is <= pointer size.
/// Configuration depends on the option passed.
#define DECLARE_EMBEDDED_TYPE_INFO( type, type_conf ) \
    template<> class TypeInfo<type> \
    { \
        public: enum { IsEmbedded = ( sizeof ( type ) <= sizeof ( void * ) ), TypeConfig = type_conf }; }

/// @brief Macro for declaring template types
/// Direct types are treated as direct only if their size is <= pointer size.
/// Configuration depends on the option passed.
#define DECLARE_EMBEDDED_TEMPLATE_TYPE_INFO( templ_type, type_conf ) \
    template<typename T> class TypeInfo<templ_type<T> > \
    { \
        public: enum { IsEmbedded = ( sizeof ( templ_type<T> ) <= sizeof ( void * ) ), TypeConfig = type_conf }; }

/// @brief Macro for declaring template types
/// Direct types are treated as direct only if their size is <= pointer size.
/// Configuration depends on the option passed.
#define DECLARE_EMBEDDED_2TEMPLATE_TYPE_INFO( templ_type, type_conf ) \
    template<typename A, typename B> class TypeInfo<templ_type<A, B> > \
    { \
        public: enum { IsEmbedded = ( sizeof ( templ_type<A, B> ) <= sizeof ( void * ) ), TypeConfig = type_conf }; }

// Integer-based types are direct. Except or 64bit integers/or long doubles
// on 32bit arch, where pointers are shorter.
DECLARE_EMBEDDED_TYPE_INFO ( bool, TYPE_CONF_ZERO );
DECLARE_EMBEDDED_TYPE_INFO ( uint8_t, TYPE_CONF_ZERO );
DECLARE_EMBEDDED_TYPE_INFO ( int8_t, TYPE_CONF_ZERO );
DECLARE_EMBEDDED_TYPE_INFO ( char, TYPE_CONF_ZERO );
DECLARE_EMBEDDED_TYPE_INFO ( uint16_t, TYPE_CONF_ZERO );
DECLARE_EMBEDDED_TYPE_INFO ( int16_t, TYPE_CONF_ZERO );
DECLARE_EMBEDDED_TYPE_INFO ( uint32_t, TYPE_CONF_ZERO );
DECLARE_EMBEDDED_TYPE_INFO ( int32_t, TYPE_CONF_ZERO );
DECLARE_EMBEDDED_TYPE_INFO ( uint64_t, TYPE_CONF_ZERO );
DECLARE_EMBEDDED_TYPE_INFO ( int64_t, TYPE_CONF_ZERO );

DECLARE_EMBEDDED_TYPE_INFO ( float, TYPE_CONF_ZERO );
DECLARE_EMBEDDED_TYPE_INFO ( double, TYPE_CONF_ZERO );
DECLARE_EMBEDDED_TYPE_INFO ( long double, TYPE_CONF_ZERO );

// Problems on 32bit:
// DECLARE_EMBEDDED_TYPE_INFO ( long long, TYPE_CONF_ZERO );
// DECLARE_EMBEDDED_TYPE_INFO ( unsigned long long, TYPE_CONF_ZERO );

/// @brief Class providing easier operations on a shared memory segments.
class SharedMemory
{
    public:
        /// @brief Data type stored in the segments
        typedef void * Pointer;

        /// @brief Class providing reference counting.
        class RefCounter
        {
            public:
                /// @brief Constructor.
                /// Sets reference count to 1.
                inline RefCounter(): _count ( 1 )
                {
                }

                /// @brief Increments reference counter.
                inline void ref()
                {
                    // We do not want to check for count > 0 here, since it's possible to
                    // unref this down to 0, then ref it back up to 1 to continue using it.
                    //
                    // In a threaded environment, we cannot safely check for count() == 1, then do cleanup.
                    // since it's possible another thread will do the same check and try to do something as well.
                    //
                    // You can however safely call unref() and do cleanup if it returns true,
                    // since that atomically decrements the count.

#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )
                    InterlockedIncrement ( &_count );
#else
                    __sync_add_and_fetch ( &_count, 1 );
#endif
                }

                /// @brief Decrements reference counter.
                /// @return Returns true if the last reference has been removed; false otherwise.
                inline bool unref()
                {
                    assert ( _count > 0 );

#if defined( SYSTEM_WINDOWS ) && defined( _MSC_VER )
                    return ( InterlockedDecrement ( &_count ) < 1 );
#else
                    return ( __sync_sub_and_fetch ( &_count, 1 ) < 1 );
#endif
                }

                /// @brief Returns reference count
                /// @return Reference count.
                inline uint32_t count() const
                {
                    return _count;
                }

            private:
                /// @brief Actual ref count value.
                volatile uint32_t _count;
        };

        /// @brief Function performing copying of a single data entry.
        /// Copy operation depends on the type, either it is just a memory operation,
        /// or a copy constructor is called.
        /// @param [in] dest Destination cell of the data.
        /// @param [in] other The object to be copied.
        template<typename T> static void copySingle ( SharedMemory::Pointer & dest, const T & other );

        /// @brief Copies several data objects.
        /// Copy operation depends on the type, either it is just a memory operation,
        /// or a copy constructor is called.
        /// The data can NOT overlap.
        /// @param [in] dest Destination of the copy operation. The first copied element will be placed
        ///                   at this address.
        /// @param [in] srcBase Base source address of the operation. It is the base for calculating the address
        ///                  using given index
        /// @param [in] firstIndex The index of the first element to copy
        /// @param [in] lastIndex The index of the last element to copy. If it is smaller than firstIndex,
        ///                        nothing is copied.
        /// @return The number of elements copied
        template<typename T> static size_t copyData (
            SharedMemory::Pointer * dest,
            const SharedMemory::Pointer * srcBase,
            size_t firstIndex, size_t lastIndex );

        /// @brief Copies several data objects, when they're simply moved in the memory.
        /// Copy operation depends on the type, either it is just a memory operation,
        /// or a copy constructor is called. This is performed when the same shared memory segment is resized,
        /// and those objects are simply moved in the memory, instead of being copied to another shared data segment.
        /// This function also calls (if both CALL_DESTR and CALL_COPY_CONSTR_ON_MOVE are specified)
        /// destructor on the old object.
        /// The data can overlap. Source can not be equal to destination.
        /// @param [in] dest Destination of the copy operation. The first copied element will be placed
        ///                   at this address.
        /// @param [in] srcBase Base source address of the operation. It is the base for calculating the address
        ///                  using given index
        /// @param [in] firstIndex The index of the first element to copy
        /// @param [in] lastIndex The index of the last element to copy. If it is smaller than firstIndex,
        ///                        nothing is copied.
        /// @return The number of elements copied
        template<typename T> static size_t moveData (
            SharedMemory::Pointer * dest,
            SharedMemory::Pointer * srcBase,
            size_t firstIndex, size_t lastIndex );

        /// @brief Function performing destruction of a single data entry.
        /// Destructor operation depends on the type, either it is either calling a destructor in the object,
        /// deleting a pointer (to indirectly stored object), or simply nothing.
        /// @param [in] data Data to be destructed.
        template<typename T> static void destructSingle ( SharedMemory::Pointer & data );

        /// @brief Function performing destruction on a several data objects.
        /// Destructor operation depends on the type, either it is either calling a destructor in the object,
        /// deleting a pointer (to indirectly stored object), or simply nothing.
        /// @param [in] buffer Beginning of the data segment of objects to be destructed.
        /// @param [in] size Length of the data segment.
        template<typename T> static void destructData ( SharedMemory::Pointer * buffer, size_t size );

        /// @brief Helper function to present data inside void* as a requested type (const reference).
        /// The conversion depends on the type of data. So if the type is direct,
        /// the data stored in the pointer is used as the memory of the object itself.
        /// If the object is stored indirectly, the void pointer is treated as a pointer to this object
        /// and resolved.
        /// @param [in] data Data cell containing the memory.
        template<typename T> static const T & convertValue ( const SharedMemory::Pointer & data );

        /// @brief Helper function to present data inside void* as a requested type (reference).
        /// The conversion depends on the type of data. So if the type is direct,
        /// the data stored in the pointer is used as the memory of the object itself.
        /// If the object is stored indirectly, the void pointer is treated as a pointer to this object
        /// and resolved.
        /// @param [in] data Data cell containing the memory.
        template<typename T> static T & convertValue ( SharedMemory::Pointer & data );
};

template<typename T> size_t SharedMemory::copyData (
        SharedMemory::Pointer * dest,
        const SharedMemory::Pointer * srcBase, size_t firstIndex, size_t lastIndex )
{
    if ( lastIndex < firstIndex )
        return 0;

    assert ( srcBase != 0 );
    assert ( dest != 0 );

    const SharedMemory::Pointer * const src = srcBase + firstIndex;
    const size_t size = lastIndex - firstIndex + 1;

    // Overlap check:
    assert ( !( dest >= src && dest < src + size ) );
    assert ( !( src >= dest && src < dest + size ) );

    if ( TypeInfo<T>::IsEmbedded )
    {
        // We are storing values directly, we ned to be careful!

        if ( !( TypeInfo<T>::TypeConfig & TYPE_CONF_CALL_COPY_CONSTR ) )
        {
            // Directly stored data doesn't need copy constructor on copy.
            // Just copy it to the new memory!

            // This doesn't handle memory overlaps!
            memcpy ( dest, src, size * sizeof ( Pointer ) );
        }
        else
        {
            // We need to call copy constructor

            // This doesn't handle memory overlaps!
            for ( size_t idx = 0; idx < size; ++idx )
            {
                T * destObj = reinterpret_cast<T *> ( dest + idx );
                const T & srcRef = *reinterpret_cast<const T *> ( src + idx );

                new ( destObj ) T ( srcRef );
            }
        }
    }
    else
    {
        // We are storing pointers to actual values.
        // Since we are creating a copy, we need to call copy constructor and
        // put the new pointer in the destination.

        assert ( !TypeInfo<T>::IsEmbedded );

        for ( size_t idx = 0; idx < size; ++idx )
        {
            const T & srcRef = *reinterpret_cast<const T *> ( src[ idx ] );
            dest[ idx ] = new T ( srcRef );
        }
    }

    return size;
}

template<typename T> size_t SharedMemory::moveData (
        SharedMemory::Pointer * dest,
        SharedMemory::Pointer * srcBase, size_t firstIndex, size_t lastIndex )
{
    if ( lastIndex < firstIndex )
        return 0;

    assert ( srcBase != 0 );
    assert ( dest != 0 );

    SharedMemory::Pointer * src = srcBase + firstIndex;
    const size_t size = lastIndex - firstIndex + 1;

    if ( TypeInfo<T>::IsEmbedded
         && ( TypeInfo<T>::TypeConfig & TYPE_CONF_CALL_COPY_CONSTR_ON_MOVE ) )
    {
        // We are storing embedded values, of a type that requires copy constructor (and destructor in the original)
        // on move!

        // We need to call copy constructor even on move!

        assert ( size > 0 );

        // There is no data overlap, or dest is smaller than source,
        // which means that if starting with copying src[0] first
        // and going up we won't overwrite something.
        if ( src >= dest + 1 || dest >= src + size )
        {
            for ( size_t idx = 0; idx < size; ++idx )
            {
                T * destObj = reinterpret_cast<T *> ( dest + idx );
                T & srcRef = *reinterpret_cast<T *> ( src + idx );

                new ( destObj ) T ( ( const T & ) srcRef );

                srcRef.~T();
            }

            return size;
        }

        // dest[0] is inside segment <src, src+size>
        // We need to start copying from [src+size-1] first,
        // and go down.
        for ( size_t idx = size; idx > 0; --idx )
        {
            // Let's make sure that src and dest do not overlap in the "wrong" way.
            // "wrong" way is when they're either the same, or partially overlapping.
            // They're ok if dest is smaller than src by at least one full element,
            // or if src is smaller than dest by at lest one full element
            assert ( ( dest < src && dest + 1 <= src )
                     || ( src < dest && src + 1 <= dest ) );

            T * destObj = reinterpret_cast<T *> ( dest + idx - 1 );
            T & srcRef = *reinterpret_cast<T *> ( src + idx - 1 );

            new ( destObj ) T ( ( const T & ) srcRef );

            srcRef.~T();
        }

        return size;
    }

    assert ( !TypeInfo<T>::IsEmbedded
             || !( TypeInfo<T>::TypeConfig & TYPE_CONF_CALL_COPY_CONSTR_ON_MOVE ) );

    // We are storing pointers to actual values,
    // or we are using embedded types which don't require copy constructor on move.
    // Just copy it to the new memory object

    // This is memmove becase src and dest could overlap.
    memmove ( dest, src, size * sizeof ( Pointer ) );

    return size;
}

template<typename T> void SharedMemory::copySingle ( SharedMemory::Pointer & dest, const T & other )
{
    // Type stored using a pointer:
    if ( !TypeInfo<T>::IsEmbedded )
    {
        dest = new T ( other );
        return;
    }

    // Embedded type

    // And it needs a copy constructor
    if ( TypeInfo<T>::TypeConfig & TYPE_CONF_CALL_COPY_CONSTR )
    {
        T * locObj = reinterpret_cast<T *> ( &dest );

        new ( locObj ) T ( other );
    }
    else
    {
        // Can be copied with memory operation.
        *reinterpret_cast<T *> ( &dest ) = other;
    }
}

template<typename T> const T & SharedMemory::convertValue ( const SharedMemory::Pointer & data )
{
    return *reinterpret_cast<const T *> ( TypeInfo<T>::IsEmbedded ? &data : data );
}

template<typename T> T & SharedMemory::convertValue ( SharedMemory::Pointer & data )
{
    return *reinterpret_cast<T *> ( TypeInfo<T>::IsEmbedded ? &data : data );
}

template<typename T> void SharedMemory::destructSingle ( SharedMemory::Pointer & data )
{
    if ( !TypeInfo<T>::IsEmbedded )
    {
        // We are storing an object using its pointer - delete it.
        T * ptr = reinterpret_cast<T *> ( data );

        assert ( ptr != 0 );
        delete ptr;
    }
    else if ( TypeInfo<T>::TypeConfig & TYPE_CONF_CALL_DESTR )
    {
        // We are storing objects directly and they need their destructors to be called.

        T & locRef = *reinterpret_cast<T *> ( &data );

        locRef.~T();
    }
}

template<typename T> void SharedMemory::destructData ( SharedMemory::Pointer * buffer, size_t size )
{
    if ( size < 1 )
        return;

    if ( !TypeInfo<T>::IsEmbedded )
    {
        // We are storing an object using its pointer - delete it.

        for ( size_t idx = 0; idx < size; ++idx )
        {
            T * ptr = reinterpret_cast<T *> ( buffer[ idx ] );

            assert ( ptr != 0 );
            delete ptr;
        }
    }
    else if ( TypeInfo<T>::TypeConfig & TYPE_CONF_CALL_DESTR )
    {
        // We are storing objects directly and they need their destructors to be called.

        assert ( TypeInfo<T>::IsEmbedded );

        for ( size_t idx = 0; idx < size; ++idx )
        {
            T & locRef = *reinterpret_cast<T *> ( &buffer[ idx ] );

            locRef.~T();
        }
    }
}
}
