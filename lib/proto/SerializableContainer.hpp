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

#include "Serializable.hpp"

namespace Pravala
{
/// @brief A container for serializable objects.
/// It is used to be able to store and copy objects using their base pointers.
/// The full objects can be copied, preserving their data, defines, etc.,
/// while only exposing access to their base type.
/// For an object to use this container it cannot be abstract, it needs to implement Serializable
/// (clear, validate, etc.), a clone() method and '==' operator.
/// This wrapper always includes internal object, either a clone of another object,
/// or constructed using T's default constructor: T().
/// @tparam T The type of the base class of the family of objects this container will be storing.
///           This type should be a class (not a pointer), and clone() method should return a pointer of that type.
template<typename T> class SerializableContainer
{
    public:
        /// @brief Default constructor.
        inline SerializableContainer(): _ptr ( new T() )
        {
        }

        /// @brief Copy constructor.
        /// @param [in] other The container to copy.
        inline SerializableContainer ( const SerializableContainer & other ): _ptr ( other._ptr->clone() )
        {
        }

        /// @brief Copy constructor.
        /// @param [in] other The object whose clone (obtained using clone()) to store.
        inline SerializableContainer ( const T & other ): _ptr ( other.clone() )
        {
        }

        /// @brief Destructor.
        inline ~SerializableContainer()
        {
            delete _ptr;
        }

        /// @brief Assignment operator.
        /// @param [in] other The container to copy.
        /// @return Reference to this container.
        inline SerializableContainer & operator= ( const SerializableContainer & other )
        {
            if ( this == &other || _ptr == other._ptr )
                return *this;

            T * const prevPtr = _ptr;

            _ptr = other._ptr->clone();
            delete prevPtr;

            return *this;
        }

        /// @brief Assignment operator.
        /// @param [in] other The object whose clone (obtained using clone()) to store.
        /// @return Reference to this container.
        inline SerializableContainer & operator= ( const T & other )
        {
            if ( _ptr == &other )
                return *this;

            T * const prevPtr = _ptr;

            _ptr = other.clone();
            delete prevPtr;

            return *this;
        }

        /// @brief Compares two containers to determine equality.
        /// @note This operator compares the objects stored in those containers ONLY at the base level.
        ///       If this returns true, it means those objects are the same at the base level,
        ///       even if they are different at some lower level of inheritance tree.
        /// @param [in] other The second container to compare.
        /// @return A value indicating whether the objects stored in those containers are equal at the base level.
        inline bool operator== ( const SerializableContainer & other ) const
        {
            return ( getObject() == other.getObject() );
        }

        /// @brief Compares a containers and another object to determine equality.
        /// @note This operator compares the objects ONLY at the base level.
        ///       If this returns true, it means that objects are the same at the base level,
        ///       even if they are different at some lower level of inheritance tree.
        /// @param [in] other The object to compare against.
        /// @return A value indicating whether the object stored in this containers is equal at the base level
        ///         to the object provided.
        inline bool operator== ( const T & other ) const
        {
            return ( getObject() == other );
        }

        /// @brief Compares two containers to determine inequality.
        /// @note This operator compares the objects stored in those containers ONLY at the base level.
        ///       If this returns false, it means those objects are the same at the base level,
        ///       even if they are different at some lower level of inheritance tree.
        /// @param [in] other The second container to compare.
        /// @return A value indicating whether the objects stored in those containers are different at the base level.
        inline bool operator!= ( const SerializableContainer & other ) const
        {
            return !( getObject() == other.getObject() );
        }

        /// @brief Compares a containers and another object to determine inequality.
        /// @note This operator compares the objects ONLY at the base level.
        ///       If this returns false, it means that objects are the same at the base level,
        ///       even if they are different at some lower level of inheritance tree.
        /// @param [in] other The object to compare against.
        /// @return A value indicating whether the object stored in this containers is different at the base level
        ///         than the object provided.
        inline bool operator!= ( const T & other ) const
        {
            return !( getObject() == other );
        }

        /// @brief Cast operator.
        /// It allows the SerializableContainer to be used as its internal type.
        /// @note For safety only the const version is provided.
        ///       To be able to modify it, getObject() needs to be used.
        /// @return A reference to internal object.
        inline operator const T &() const
        {
            return *_ptr;
        }

        /// @brief Exposes the base object of the object stored in this container.
        /// @return The base object of the object stored in this container.
        inline T & getObject()
        {
            return *_ptr;
        }

        /// @brief Exposes the base object of the object stored in this container.
        /// @return The base object of the object stored in this container.
        inline const T & getObject() const
        {
            return *_ptr;
        }

        /// @brief Clears the internal object.
        inline void clear()
        {
            _ptr->clear();
        }

        /// @brief Checks validity of the data inside internal object.
        /// Returns success if all required fields in this and all inherited objects (if any)
        /// are present and have legal values. If this is used by external code on messages
        /// or structures that are to be sent (NOT on received ones!)
        /// it is probably a good idea to call setupDefines() first.
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return Standard error code
        inline ProtoError validate ( ExtProtoError * extError = 0 ) const
        {
            return _ptr->validate ( extError );
        }

        /// @brief Sets the values of all the fields 'defined' by the internal object and all inherited objects (if any)
        inline void setupDefines()
        {
            _ptr->setupDefines();
        }

        /// @brief Serializes content of the internal object.
        /// @param [in,out] arg The argument to pass to internal object's serialize() method.
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return The error code
        template<typename V> inline ProtoError serialize ( V & arg, ExtProtoError * extError = 0 )
        {
            return _ptr->serialize ( arg, extError );
        }

        /// @brief Deserializes the internal object using data from the buffer.
        /// @param [in] buf The buffer to deserialize the data from.
        /// @param [in] offset Offset in the buffer.
        /// @param [in] dataSize The size of data (starting at given offset).
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return The error code
        inline ProtoError deserialize (
            const MemHandle & buf, size_t offset, size_t dataSize, ExtProtoError * extError = 0 )
        {
            return _ptr->deserialize ( buf, offset, dataSize, extError );
        }

    private:
        T * _ptr; ///< The pointer to the object stored in this container.
};
}
