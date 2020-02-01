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

namespace Pravala
{
/// @brief Class for objects that wish to have reference counting without ObjectPool to inherit.
class SimpleObject
{
    public:
        /// @brief Unreferences this object.
        ///
        /// Decrements the reference counter, and if it reaches 0 - deletes the object.
        inline void unref()
        {
            assert ( _numRef > 0 );

            if ( --_numRef < 1 )
            {
                delete this;
            }
        }

        /// @brief Increments the reference counter.
        ///
        /// If any object wants to store the pointer to this object
        /// in some way, it needs to call ref().
        /// If, however, a function receives a pointer to this object,
        /// does something on it (even if it includes calling other functions
        /// that take this pointer as an argument), and returns,
        /// without storing this pointer
        /// anywhere, it does not need to call ref() on this object.
        inline void ref()
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
        /// Sets number of references to 1.
        /// Protected.
        SimpleObject(): _numRef ( 1 )
        {
        }

        /// @brief Virtual destructor
        virtual ~SimpleObject()
        {
            assert ( _numRef == 0 );
        }

    private:
        uint16_t _numRef; ///< Number of references to this object. (Reference counter).

        /// @brief Private, non-existent copy constructor.
        ///
        /// Objects with reference counters can not be copied.
        SimpleObject ( const SimpleObject & );

        /// @brief Private, non-existent operator=.
        ///
        /// Objects with reference counters can not be copied.
        SimpleObject & operator= ( const SimpleObject & );
};
}
