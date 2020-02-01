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

namespace Pravala
{
/// @brief A wrapper around delete operator
/// @tparam T The type to operate on.
template<typename T> void stdDelete ( T * ptr )
{
    delete ptr;
}

/// @brief A wrapper around delete[] operator
/// @tparam T The type to operate on.
template<typename T> void stdDeleteArr ( T * ptr )
{
    delete[] ptr;
}

/// @brief A base template class for AutoPtr and AutoMethodPtr.
/// @tparam T The type this class points to.
template<typename T> class AutoPtrBase
{
    public:
        /// @brief Default constructor.
        /// Creates a null pointer.
        AutoPtrBase(): _ptr ( 0 )
        {
        }

        /// @brief Constructor.
        /// @param [in] ptr The pointer to set.
        AutoPtrBase ( T * ptr ): _ptr ( ptr )
        {
        }

        /// @brief Copy constructor.
        /// It takes over the pointer from the other object.
        /// @param [in] other The AutoPtrBase to copy.
        AutoPtrBase ( AutoPtrBase & other ): _ptr ( other._ptr )
        {
            other._ptr = 0;
        }

        /// @brief Access element operator.
        /// @param [in] idx Index to query.
        /// @return Constant reference to the element.
        const T & operator[] ( size_t idx ) const
        {
            assert ( _ptr != 0 );

            return _ptr[ idx ];
        }

        /// @brief Access element operator.
        /// @param [in] idx Index to query.
        /// @return A reference to the element.
        T & operator[] ( size_t idx )
        {
            assert ( _ptr != 0 );

            return _ptr[ idx ];
        }

        /// @brief Resolves the pointer.
        /// @note This should NOT be called when the pointer is not set!
        /// @return The reference to the object pointed to by the pointer.
        T & operator*() const
        {
            assert ( _ptr != 0 );

            return *_ptr;
        }

        /// @brief Apply the -> operator to the pointer.
        /// @note This should NOT be called when the pointer is not set!
        /// @return The pointer stored.
        T * operator->()
        {
            assert ( _ptr != 0 );

            return _ptr;
        }

        /// @brief Returns the pointer stored.
        /// @return The pointer stored.
        T * get() const
        {
            return _ptr;
        }

        /// @brief Returns the pointer stored and clears it.
        /// After this call it will be caller's responsibility to cleanup the pointer.
        /// @return The pointer previously stored.
        T * release()
        {
            T * ret = _ptr;
            _ptr = 0;
            return ret;
        }

    protected:
        T * _ptr; ///< The actual pointer.

        // Unavailable:
        AutoPtrBase & operator= ( const AutoPtrBase & other );
};

/// @brief A template class that takes care of cleaning up the pointer when it goes out of scope.
/// @tparam T The type this class points to.
/// @tparam CFT The type of the cleanup function pointer.
template<typename T, typename CFT> class AutoFuncPtr: public AutoPtrBase<T>
{
    public:
        /// @brief Default constructor.
        /// Creates a null pointer and uses standard delete operator.
        AutoFuncPtr(): _cleanupFunc ( stdDelete )
        {
            assert ( _cleanupFunc != 0 );
        }

        /// @brief Constructor.
        /// Uses standard delete operator.
        /// @param [in] ptr The pointer to set.
        AutoFuncPtr ( T * ptr ): AutoPtrBase<T> ( ptr ), _cleanupFunc ( stdDelete )
        {
            assert ( _cleanupFunc != 0 );
        }

        /// @brief Constructor.
        /// Creates a null pointer.
        /// @param [in] cleanupFunc Pointer to the cleanup function.
        AutoFuncPtr ( CFT cleanupFunc ): AutoPtrBase<T> ( 0 ), _cleanupFunc ( cleanupFunc )
        {
            assert ( _cleanupFunc != 0 );
        }

        /// @brief Constructor.
        /// @param [in] ptr The pointer to set.
        /// @param [in] cleanupFunc Pointer to the cleanup function.
        AutoFuncPtr ( T * ptr, CFT cleanupFunc ): AutoPtrBase<T> ( ptr ), _cleanupFunc ( cleanupFunc )
        {
            assert ( _cleanupFunc != 0 );
        }

        /// @brief Copy constructor.
        /// It takes over the pointer from the other object.
        /// @param [in] other The AutoFuncPtr to copy.
        AutoFuncPtr ( AutoFuncPtr & other ): AutoPtrBase<T> ( other ), _cleanupFunc ( other._cleanupFunc )
        {
            assert ( _cleanupFunc != 0 );
        }

        /// @brief Destructor.
        /// It cleans up the pointer that is currently stored.
        ~AutoFuncPtr()
        {
            reset ( 0 );
        }

        /// @brief Assignment operator.
        /// It cleans up the pointer currently stored, and takes over the pointer from the other object.
        /// @param [in] other The object to copy.
        /// @return A reference to this object.
        AutoFuncPtr & operator= ( AutoFuncPtr & other )
        {
            if ( &other == this )
                return *this;

            reset ( other.release() );

            _cleanupFunc = other._cleanupFunc;

            return *this;
        }

        /// @brief Assignment operator.
        /// It cleans up the pointer currently stored, and sets the new one.
        /// @param [in] ptr The pointer to set.
        /// @return A reference to this object.
        AutoFuncPtr & operator= ( T * ptr )
        {
            reset ( ptr );
            return *this;
        }

        /// @brief Cleans up currently stored pointer and sets the new one.
        /// @param [in] ptr The new pointer to use. Could be zero to 'clear' the AutoFuncPtr.
        void reset ( T * ptr )
        {
            if ( ptr != AutoPtrBase<T>::_ptr && AutoPtrBase<T>::_ptr != 0 )
            {
                assert ( _cleanupFunc != 0 );

                _cleanupFunc ( AutoPtrBase<T>::_ptr );
            }

            AutoPtrBase<T>::_ptr = ptr;
        }

        /// @brief Cleans up currently stored pointer and returns a pointer-to-internal-pointer.
        /// It can be used by passing its return value to functions that set a pointer given
        /// to them by pointer-to-pointer.
        /// @return Pointer to (now empty) internal pointer.
        T ** reset()
        {
            reset ( 0 );
            return &( AutoPtrBase<T>::_ptr );
        }

    private:
        CFT _cleanupFunc; ///< Pointer to the cleanup function.
};

/// @brief A template class that takes care of cleaning up the pointer when it goes out of scope.
/// @tparam T The type this class points to.
/// @tparam CPT The type the cleanup function takes a pointer to.
/// @tparam CRT The return type of the cleanup function.
template<typename T, typename CPT = T, typename CRT = void> class AutoPtr: public AutoFuncPtr<T, CRT ( * )( CPT * ) >
{
    public:
        /// @brief A type that represents a pointer to the cleanup function
        typedef CRT (* CleanupFunctionType) ( CPT * );

        /// @brief Constructor.
        /// Uses standard delete operator.
        /// @param [in] ptr The pointer to set.
        AutoPtr ( T * ptr ): AutoFuncPtr<T, CRT ( * ) ( CPT * ) > ( ptr )
        {
        }

        /// @brief Constructor.
        /// Creates a null pointer.
        /// @param [in] cleanupFunc Pointer to the cleanup function.
        AutoPtr ( CleanupFunctionType cleanupFunc ):
            AutoFuncPtr<T, CRT ( * ) ( CPT * ) > ( cleanupFunc )
        {
        }

        /// @brief Constructor.
        /// @param [in] ptr The pointer to set.
        /// @param [in] cleanupFunc Pointer to the cleanup function.
        AutoPtr ( T * ptr, CleanupFunctionType cleanupFunc ):
            AutoFuncPtr<T, CRT ( * ) ( CPT * ) > ( ptr, cleanupFunc )
        {
        }

        /// @brief Assignment operator.
        /// It cleans up the pointer currently stored, and sets the new one.
        /// @param [in] ptr The pointer to set.
        /// @return A reference to this object.
        AutoPtr & operator= ( T * ptr )
        {
            AutoFuncPtr<T, CRT ( * )( CPT * ) >::operator= ( ptr );
            return *this;
        }
};

/// @brief A template class that takes care of cleaning up the pointer when it goes out of scope.
/// This version is appropriate for objects that require a method to be called in them during cleanup.
/// @tparam T The type this class points to.
/// @tparam CRT The return type of the cleanup method.
template<typename T, typename CRT = void> class AutoMethodPtr: public AutoPtrBase<T>
{
    public:
        /// @brief A type that represents a pointer to the cleanup method
        typedef CRT (T::* CleanupMethod) ();

        /// @brief Constructor.
        /// It creates a null pointer.
        /// @param [in] cleanupMethod Pointer to the cleanup method to use.
        AutoMethodPtr ( CleanupMethod cleanupMethod ): AutoPtrBase<T> ( 0 ), _cleanupMethod ( cleanupMethod )
        {
            assert ( _cleanupMethod != 0 );
        }

        /// @brief Constructor.
        /// @param [in] ptr Pointer to use.
        /// @param [in] cleanupMethod Pointer to the cleanup method to use.
        AutoMethodPtr ( T * ptr, CleanupMethod cleanupMethod ):
            AutoPtrBase<T> ( ptr ), _cleanupMethod ( cleanupMethod )
        {
            assert ( _cleanupMethod != 0 );
        }

        /// @brief Copy constructor.
        /// It takes over the pointer from the other object.
        /// @param [in] other The AutoPtr to copy.
        AutoMethodPtr ( AutoMethodPtr & other ): AutoPtrBase<T> ( other ), _cleanupMethod ( other._cleanupMethod )
        {
            assert ( _cleanupMethod != 0 );
        }

        /// @brief Destructor.
        /// It cleans up the pointer that is currently stored.
        ~AutoMethodPtr()
        {
            reset ( 0 );
        }

        /// @brief Assignment operator.
        /// It cleans up the pointer currently stored, and takes over the pointer from the other object.
        /// @param [in] other The object to copy.
        /// @return A reference to this object.
        AutoMethodPtr & operator= ( AutoMethodPtr & other )
        {
            if ( &other == this )
                return *this;

            reset ( other.release() );

            _cleanupMethod = other._cleanupMethod;

            return *this;
        }

        /// @brief Assignment operator.
        /// It cleans up the pointer currently stored, and sets the new one.
        /// @param [in] ptr The pointer to set.
        /// @return A reference to this object.
        AutoMethodPtr & operator= ( T * ptr )
        {
            reset ( ptr );
            return *this;
        }

        /// @brief Cleans up currently stored pointer and sets the new one.
        /// @param [in] ptr The new pointer to use. Could be zero to 'clear' the AutoPtr.
        void reset ( T * ptr )
        {
            if ( ptr != AutoPtrBase<T>::_ptr && AutoPtrBase<T>::_ptr != 0 )
            {
                assert ( _cleanupMethod != 0 );

                ( ( AutoPtrBase<T>::_ptr )->*_cleanupMethod )();
            }

            AutoPtrBase<T>::_ptr = ptr;
        }

        /// @brief Cleans up currently stored pointer and returns a pointer-to-internal-pointer.
        /// It can be used by passing its return value to functions that set a pointer given
        /// to them by pointer-to-pointer.
        /// @return Pointer to (now empty) internal pointer.
        T ** reset()
        {
            reset ( 0 );
            return &( AutoPtrBase<T>::_ptr );
        }

    private:
        CleanupMethod _cleanupMethod; ///< Pointer to the cleanup method.
};
}
