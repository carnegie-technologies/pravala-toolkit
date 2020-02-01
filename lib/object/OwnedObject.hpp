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

#include "basic/NoCopy.hpp"

namespace Pravala
{
/// @brief Template for objects that use reference counting.
/// This is NOT thread safe!
/// @tparam OwnerType The type of the owner.
template<class OwnerType> class OwnedObject: public NoCopy
{
    public:
        /// @brief Unsets object's owner.
        /// @note It does not modify the reference counter.
        /// @param [in] owner If this owner matches the current owner, the current owner is set to 0
        inline void unsetOwner ( OwnerType * owner )
        {
            if ( _owner == owner )
            {
                _owner = 0;
                ownerChanged();
            }
        }

        /// @brief Sets object's owner.
        /// @note It does not modify the reference counter.
        /// @param [in] owner The new owner
        inline void setOwner ( OwnerType * owner )
        {
            if ( _owner != owner )
            {
                _owner = owner;
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

            if ( _owner == owner )
            {
                _owner = 0;
                ownerChanged();
            }

            if ( --_numRef < 1 )
            {
                assert ( !_owner );
                assert ( _numRef == 0 );

                delete this;
            }
        }

        /// @brief Decrement the reference count on this object without changing the owner
        inline void simpleUnref()
        {
            unrefOwner ( 0 );
        }

        /// @brief Increments the reference counter.
        ///
        /// If any object wants to store the pointer to this object in some way, it needs to call simpleRef/refOwner().
        /// If, however, a function receives a pointer to this object, does something on it (even if it
        /// includes calling other functions that take this pointer as an argument), and returns, without
        /// storing this pointer anywhere, it does not need to call simpleRef/refOwner() on this object.
        ///
        /// @param [in] owner New owner of this object, 0 to leave it unchanged
        inline void refOwner ( OwnerType * owner )
        {
            assert ( _numRef > 0 );

            ++_numRef;

            if ( owner != 0 && _owner != owner )
            {
                _owner = owner;
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
        inline uint32_t getRefCount() const
        {
            return _numRef;
        }

        /// @brief Gets the owner of this object
        /// @return The owner of this object
        inline OwnerType * getOwner()
        {
            return _owner;
        }

    protected:
        /// @brief Constructor.
        /// It sets the owner and initializes the reference counter to 1.
        /// @param [in] owner The original owner of the object. Could be 0.
        OwnedObject ( OwnerType * owner ): _owner ( owner ), _numRef ( 1 )
        {
        }

        /// @brief Destructor.
        virtual ~OwnedObject()
        {
            assert ( !_owner );
            assert ( _numRef == 0 );
        }

        /// @brief Called whenever 'owner' changes, just after the 'owner' field has been modified.
        /// Default implementation does nothing.
        /// @note This may get called just before removing the last reference (and destroying the object)!
        virtual void ownerChanged()
        {
        }

    private:
        OwnerType * _owner; ///< Owner of this object.

        uint32_t _numRef; ///< Number of references to this object (a reference counter).

        /// @brief Private, non-existent copy constructor.
        /// Pooled/referenced objects can not be copied.
        OwnedObject ( const OwnedObject & );

        /// @brief Private, non-existent operator=.
        /// Pooled/referenced objects can not be copied.
        OwnedObject & operator= ( const OwnedObject & );
};
}
