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

#include "PoolCtrl.hpp"

#define POOLED_OWNED_OBJECT_HDR( type, owner_type )    friend class PooledOwnedObject<type, owner_type>

namespace Pravala
{
///
/// @brief Template for objects using Object Pool instead of new/delete (de)allocations
///
/// This is NOT thread safe!
///
/// To use it your declaration should look like:
///
/// class MyObject;
///
/// class MyObjectOwner {
///   protected:
///     virtual someMethod(int arg) = 0;
///     friend class MyObject;
/// };
///
/// class MyObject: public PooledOwnedObject<MyObject, MyObjectOwner> {
///   public:
///
///     // Generate an object from the pool
///     static MyObject * generate ( MyObjectOwner * owner /* , options? */ ) {
///         MyObject * ptr = getFromPool(owner);
///         // Maybe some initialization?:
///         // ptr->init(options);
///         return ptr;
///     }
///
///   private:
///     MyObject();
///     ~MyObject();
///
///     // Create a new object for the pool - used internally by PooledOwnedObject to create new objects
///     static MyObject * generateNew()
///     {
///         return new MyObject();
///     }
///
///     // Some cleanup? Like closing file descriptors, etc.
///     void returnsToPool();
///
///     POOLED_OWNED_OBJECT_HDR(MyObject, MyObjectOwner);
/// };
///
/// Note, that there should be only one constructor - the default one,
/// and it should be private. The destructor should be private as well.
///

template<class ObjectType, class OwnerType> class PooledOwnedObject
{
    public:
        /// @brief Unsets object's owner.
        /// Does not modify the reference counter.
        /// @param [in] owner If this owner matches the current owner, the current owner is set to 0
        inline void unsetOwner ( OwnerType * owner )
        {
            if ( _u.owner == owner )
            {
                _u.owner = 0;
                ownerChanged();
            }
        }

        /// @brief Sets object's owner.
        /// Does not modify the reference counter.
        /// @param [in] owner The new owner
        inline void setOwner ( OwnerType * owner )
        {
            if ( _u.owner != owner )
            {
                _u.owner = owner;
                ownerChanged();
            }
        }

        /// @brief Unreferences this object.
        ///
        /// Decrements the reference counter, and if it reaches 0 - returns the object to the pool.
        /// Should be used where normally destructor would be used.
        /// Object can NOT be used by the code that just called unref().
        /// Calling unref() should be treated as removing the object from the memory
        /// (as far as the code calling unref is concerned).
        ///
        /// This function calls returnsToPool() in the object if it is
        /// being returned to the pool.
        ///
        /// @param [in] owner If this owner matches the current owner, the current owner is set to 0
        inline void unrefOwner ( OwnerType * owner )
        {
            assert ( _numRef > 0 );

            unsetOwner ( owner );

            if ( --_numRef < 1 )
            {
                assert ( !_u.owner );

                static_cast<ObjectType *> ( this )->returnsToPool();

                assert ( _numRef == 0 );
                assert ( !_u.next );

#ifndef OBJECT_POOL_SKIP_POOL
                if ( !globalDestruct() )
                {
                    _u.next = globalPool();
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

        /// @brief Decrement the reference count on this object without changing the owner
        inline void simpleUnref()
        {
            unrefOwner ( 0 );
        }

        /// @brief Increments the reference counter.
        ///
        /// If any object wants to store the pointer to this object in some way, it needs to call ref().
        /// If, however, a function receives a pointer to this object, does something on it (even if it
        /// includes calling other functions that take this pointer as an argument), and returns, without
        /// storing this pointer anywhere, it does not need to call ref() on this object.
        ///
        /// @param [in] owner New owner of this object, 0 to leave it unchanged
        inline void refOwner ( OwnerType * owner )
        {
            assert ( _numRef > 0 );

            ++_numRef;

            if ( owner != 0 && _u.owner != owner )
            {
                _u.owner = owner;
                ownerChanged();
            }
        }

        /// @brief Increment the reference count on this object without changing the owner
        inline void simpleRef()
        {
            refOwner ( 0 );
        }

        /// @brief Checks the current value of the reference counter
        /// @return Returns value of the reference counter.
        inline short getRefCount() const
        {
            return _numRef;
        }

        /// @brief Gets the owner of this object
        /// @return The owner of this object
        inline OwnerType * getOwner()
        {
            return _u.owner;
        }

    protected:
        /// @brief Default constructor.
        /// Protected.
        PooledOwnedObject(): _numRef ( 1 )
        {
            memset ( &_u, 0, sizeof ( _u ) );
        }

        /// @brief Destructor
        /// This is intentionally non-virtual, nothing should call it directly, and it doesn't do anything anyways.
        ~PooledOwnedObject()
        {
            assert ( _numRef == 0 );

            // We cannot assert anything about _u.* here.
            // It shouldn't have an owner, but it might have a next object in the pool.
        }

        /// @brief Called whenever 'owner' changes, just after the 'owner' field has been modified.
        /// Default implementation does nothing.
        /// @note This may get called just before removing the last reference!
        virtual void ownerChanged()
        {
        }

        /// @brief Gets the next object from the pool.
        ///
        /// If the pool is empty, new object is created using its default constructor.
        /// @return Pointer to the object (either taken from the pool, or created)
        static inline ObjectType * getFromPool ( OwnerType * owner )
        {
            ObjectType * ptr = 0;

            if ( !globalPool() )
            {
                ptr = ObjectType::generateNew();
            }
            else
            {
                ptr = globalPool();
                globalPool() = ptr->_u.next;

                assert ( ptr->_numRef == 0 );
            }

            assert ( ptr != 0 );

            ptr->_u.owner = owner;
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
                        objects = ptr->_u.next;
                        assert ( ptr->_numRef == 0 );

                        delete ptr;
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

        /// @brief Returns a reference to the bool indicating whether the pools have been destroyed.
        /// @return A reference to the bool indicating whether the pools have been destroyed.
        /// True if the pools have been destroyed, false if the pools are present.
        static bool & globalDestruct()
        {
            static bool _global = false;

            return _global;
        }

        union
        {
            ObjectType * next; ///< Pointer to the next object in the pool (only when this object is free)
            OwnerType * owner; ///< Owner of this object (only when this object is active)
        } _u; ///< Union with possible data depending on whether this object is active or free

        uint16_t _numRef; ///< Number of references to this object. (Reference counter).

        /// @brief Private, non-existent copy constructor.
        ///
        /// Pooled/referenced objects can not be copied.
        PooledOwnedObject ( const PooledOwnedObject & );

        /// @brief Private, non-existent operator=.
        ///
        /// Pooled/referenced objects can not be copied.
        PooledOwnedObject & operator= ( const PooledOwnedObject & );
};
}
