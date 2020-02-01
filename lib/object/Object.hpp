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

#include <cassert>

extern "C"
{
#include <stdint.h>
}

#include "PoolCtrl.hpp"

#define OBJECT_HDR( type )    friend class Object<type>

namespace Pravala
{
///
/// @brief Template for objects using Object Pool instead of new/delete (de)allocations
///
/// This is NOT thread safe!
///
/// To use it your declaration should look like:
/// class MyObject: public Object<MyObject> {
///   public:
///     static MyObject * generate ( /* options? */ ) {
///         MyObject * ptr = getFromPool();
///         // Maybe some initialization?:
///         // ptr->init(options);
///         return ptr;
///     }
///
///   private:
///     MyObject();
///     ~MyObject();
///
///     // Some cleanup? Like closing file descriptors, etc.
///     void returnsToPool();
///
///   OBJECT_HDR(MyObject);
/// };
///
/// Note, that there should be only one constructor - the default one,
/// and it should be private. The destructor should be private as well.
///

template<class ObjectType> class Object
{
    public:
        /// @brief Unreferences this object.
        ///
        /// Decrements the reference counter, and if it reaches 0 - returns the object
        /// to the pool. Should be used where normally destructor would be used.
        /// Object can NOT be used by the code that just called unref().
        /// Calling unref() should be treated as removing the object from the memory
        /// (as far as the code calling unref is concerned).
        ///
        /// This function calls returnsToPool() in the object if it is
        /// being returned to the pool.
        inline void simpleUnref()
        {
            assert ( _numRef > 0 );

            if ( --_numRef < 1 )
            {
                static_cast<ObjectType *> ( this )->returnsToPool();

                assert ( _numRef == 0 );
                assert ( !_next );

#ifndef OBJECT_POOL_SKIP_POOL
                if ( !globalDestruct() )
                {
                    _next = globalPool();
                    globalPool() = static_cast<ObjectType *> ( this );
                }
                else
                {
                    delete static_cast<ObjectType *> ( this );
                }
#else
                delete static_cast<ObjectType *> ( this );
#endif
            }
        }

        /// @brief Increments the reference counter.
        ///
        /// If any object wants to store the pointer to this object
        /// in some way, it needs to call ref().
        /// If, however, a function receives a pointer to this object,
        /// does something on it (even if it includes calling other functions
        /// that take this pointer as an argument),
        /// and returns, without storing this pointer
        /// anywhere, it does not need to call ref() on this object.
        inline void simpleRef()
        {
            assert ( _numRef > 0 );

            ++_numRef;
        }

        /// @brief Checks the current value of the reference counter
        /// @return Returns value of the reference counter.
        inline uint16_t getRefCount() const
        {
            return _numRef;
        }

    protected:
        /// @brief Default constructor.
        /// Protected.
        Object(): _next ( 0 ), _numRef ( 1 )
        {
        }

        ~Object()
        {
            assert ( _numRef == 0 );
        }

        /// @brief Gets the next object from the pool.
        ///
        /// If the pool is empty, new object is created using its default constructor.
        /// @return Pointer to the object (either taken from the pool, or created)
        static inline ObjectType * getFromPool()
        {
            ObjectType * ptr = 0;

            if ( !globalPool() )
            {
                ptr = new ObjectType();
            }
            else
            {
                ptr = globalPool();
                globalPool() = ptr->_next;

                assert ( ptr->_numRef == 0 );
            }

            assert ( ptr != 0 );

            ptr->_next = 0;
            ptr->_numRef = 1;

            return ptr;
        }

    private:

        /// @brief Class storing list of object - the actual pool

        class ObjectPool
        {
            public:
                /// @brief Default constructor.
                ///
                /// Initializes pool to 0.
                ObjectPool(): objects ( 0 )
                {
                }

                /// @brief Destructor.
                ///
                /// Deletes all the objects in the pool.
                ~ObjectPool()
                {
                    globalDestruct() = true;

                    ObjectType * ptr = objects;

                    while ( ptr != 0 )
                    {
                        objects = ptr->_next;
                        assert ( ptr->_numRef == 0 );

                        delete static_cast<ObjectType *> ( ptr );
                        ptr = objects;
                    }
                }

                /// @brief The pointer to the head of the list with object pointers - this is the actual pool.
                ObjectType * objects;
        };

        /// @brief A static method returning the pool of objects.
        /// @return Global (for this type) pool of objects.
        static ObjectType * & globalPool()
        {
            static ObjectPool _global;

            return _global.objects;
        }

        static bool & globalDestruct()
        {
            static bool _global = false;

            return _global;
        }

        ObjectType * _next; ///< Pointer to the next object in the pool.

        uint16_t _numRef; ///< Number of references to this object. (Reference counter).

        /// @brief Private, non-existent copy constructor.
        ///
        /// Pooled/referenced objects can not be copied.
        Object ( const Object & );

        /// @brief Private, non-existent operator=.
        ///
        /// Pooled/referenced objects can not be copied.
        Object & operator= ( const Object & );
};
}
