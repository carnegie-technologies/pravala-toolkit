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

#include "JsonCore.hpp"
#include "JsonOpCode.hpp"

namespace Pravala
{
class IpAddress;
class Timestamp;
class Json;

/// @brief A wrapper class for a JSON array built with the Jansson JSON library
class JsonArray: public JsonCore
{
    public:
        /// @brief Constructor
        JsonArray();

        /// @brief Destructor
        virtual ~JsonArray()
        {
        }

        /// @brief Returns the size of the JSON array
        /// @return the size of the JSON array
        size_t size() const;

        /// @brief Retrieves a boolean from the JSON using the specified index
        /// @param [in] idx The index to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        JsonOpCode get ( const size_t idx, bool & val ) const;

        /// @brief Retrieves an int64_t from the JSON using the specified index
        /// @param [in] idx The index to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        JsonOpCode get ( const size_t idx, int64_t & val ) const;

        /// @brief Retrieves an uint64_t from the JSON using the specified index
        /// @param [in] idx The index to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        inline JsonOpCode get ( const size_t idx, uint64_t & val ) const
        {
            return get ( idx, ( int64_t & ) val );
        }

        /// @brief Retrieves an int32_t from the JSON using the specified index
        /// @param [in] idx The index to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        inline JsonOpCode get ( const size_t idx, int32_t & val ) const
        {
            return rangeGet< -0x80000000LL, 0x7FFFFFFFLL > ( idx, val );
        }

        /// @brief Retrieves an uint32_t from the JSON using the specified index
        /// @param [in] idx The index to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        inline JsonOpCode get ( const size_t idx, uint32_t & val ) const
        {
            return rangeGet<0, 0xFFFFFFFFLL> ( idx, val );
        }

        /// @brief Retrieves a int16_t from the JSON using the specified index
        /// @param [in] idx The index to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        inline JsonOpCode get ( const size_t idx, int16_t & val ) const
        {
            return rangeGet< -0x8000, 0x7FFF > ( idx, val );
        }

        /// @brief Retrieves a uint16_t from the JSON using the specified index
        /// @param [in] idx The index to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        inline JsonOpCode get ( const size_t idx, uint16_t & val ) const
        {
            return rangeGet<0, 0xFFFF> ( idx, val );
        }

        /// @brief Retrieves an int8_t from the JSON using the specified index
        /// @param [in] idx The index to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        inline JsonOpCode get ( const size_t idx, int8_t & val ) const
        {
            return rangeGet< -0x80, 0x7F > ( idx, val );
        }

        /// @brief Retrieves an uint8_t from the JSON using the specified index
        /// @param [in] idx The index to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        inline JsonOpCode get ( const size_t idx, uint8_t & val ) const
        {
            return rangeGet<0, 0xFF> ( idx, val );
        }

        /// @brief Retrieves a double from the JSON using the specified index
        /// @param [in] idx The index to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        JsonOpCode get ( const size_t idx, double & val ) const;

        /// @brief Retrieves a string from the JSON using the specified index
        /// @param [in] idx The index to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        JsonOpCode get ( const size_t idx, String & val ) const;

        /// @brief Retrieves a string from the JSON using the specified index, and stores it in a MemHandle object.
        /// @param [in] idx The index to check
        /// @param [out] val The value; remains the same on failure. Existing content is replaced on success.
        /// @return Result code.
        JsonOpCode get ( const size_t idx, MemHandle & val ) const;

        /// @brief Retrieves an IP address from the JSON using the specified index
        /// @param [in] idx The index to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        JsonOpCode get ( const size_t idx, IpAddress & val ) const;

        /// @brief Retrieves a JSON object from the JSON using the specified index
        /// @param [in] idx The index to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        JsonOpCode get ( const size_t idx, Json & val ) const;

        /// @brief Retrieves a JSON array from the JSON using the specified index
        /// @param [in] idx The index to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        JsonOpCode get ( const size_t idx, JsonArray & val ) const;

        /// @brief A helper function, that allows for easy exporting of a JSON array to a list
        /// @param [out] val List that will be overwritten with values of this type
        /// @note This only works if the JSON array contains all of one type
        /// @return Result code. Even on failure, some items may have been appended to the list.
        template<typename T> inline JsonOpCode get ( List<T> & val ) const
        {
            val.clear();

            T tmp;

            for ( size_t i = 0; i < size(); ++i )
            {
                JsonOpCode ret = get ( i, tmp );

                if ( ret != JsonOpCode::Success )
                {
                    return ret;
                }

                val.append ( tmp );
            }

            return JsonOpCode::Success;
        }

        /// @brief Inserts a boolean value into the JSON array with the specified index
        /// @param [in] idx The index to put this value at (should be less than current size)
        /// @param [in] val The boolean value to save
        /// @return true on successful add; false on error
        bool put ( const size_t idx, bool val );

        /// @brief Inserts an int value into the JSON array with the specified index
        /// @param [in] idx The index to put this value at (should be less than current size)
        /// @param [in] val The int value to save
        /// @return true on successful add; false on error
        bool put ( const size_t idx, int val );

        /// @brief Inserts an uint32_t value into the JSON array with the specified index
        /// @param [in] idx The index to put this value at (should be less than current size)
        /// @param [in] val The uint32_t value to save
        /// @return true on successful add; false on error
        bool put ( const size_t idx, uint32_t val );

        /// @brief Inserts an int64_t value into the JSON array with the specified index
        /// @param [in] idx The index to put this value at (should be less than current size)
        /// @param [in] val The int64_t value to save
        /// @return true on successful add; false on error
        bool put ( const size_t idx, int64_t val );

        /// @brief Inserts an uint64_t value into the JSON array with the specified index
        /// @param [in] idx The index to put this value at (should be less than current size)
        /// @param [in] val The uint64_t value to save
        /// @return true on successful add; false on error
        bool put ( const size_t idx, uint64_t val );

        /// @brief Inserts a double value into the JSON array with the specified index
        /// @param [in] idx The index to put this value at (should be less than current size)
        /// @param [in] val The double value to save
        /// @return true on successful add; false on error
        bool put ( const size_t idx, double val );

        /// @brief Inserts a string value into the JSON array with the specified index
        /// @param [in] idx The index to put this value at (should be less than current size)
        /// @param [in] val The string value to save
        /// @return true on successful add; false on error
        bool put ( const size_t idx, const String & val );

        /// @brief Inserts a string value into the JSON array with the specified index
        /// @param [in] idx The index to put this value at (should be less than current size)
        /// @param [in] val The string value to save
        /// @return true on successful add; false on error
        bool put ( const size_t idx, const MemHandle & val );

        /// @brief Inserts a null-terminated string value into the JSON array with the specified index
        /// @param [in] idx The index to put this value at (should be less than current size)
        /// @param [in] val The null-terminated string value to save
        /// @return true on successful add; false on error
        bool put ( const size_t idx, const char * val );

        /// @brief Inserts an IP address value into the JSON array with the specified index
        /// @param [in] idx The index to put this value at (should be less than current size)
        /// @param [in] val The IpAddress value to save
        /// @return true on successful add; false on error
        bool put ( const size_t idx, const IpAddress & val );

        /// @brief Inserts a Timestamp value into the JSON array with the specified index
        /// @param [in] idx The index to put this value at (should be less than current size)
        /// @param [in] val The Timestamp value to save
        /// @return true on successful add; false on error
        bool put ( const size_t idx, const Timestamp & val );

        /// @brief Inserts a JSON object or JSON array into the JSON array with the specified index
        /// @param [in] idx The index to put this value at (should be less than current size)
        /// @param [in] val The JSON object to save
        /// @return true on successful add; false on error
        bool put ( const size_t idx, JsonCore & val );

        /// @brief A helper function, that allows easy use of append() functions with list
        /// @param [in] val List of something to append
        /// @return true if the append was successful, false on error.
        /// On failure, some items may have been appended to the list
        template<typename T> inline bool append ( const List<T> & val )
        {
            for ( size_t i = 0; i < val.size(); ++i )
            {
                if ( !append ( val.at ( i ) ) )
                {
                    return false;
                }
            }

            return true;
        }

        /// @brief Appends a list of Strings to the JSON array
        /// @param [in] idx The index to put this value at (should be less than current size)
        /// @param [in] val The string array value to append
        /// @return true on successful add; false on error
        /// On failure, some items may have been appended to the list
        inline bool append ( const StringList & val )
        {
            for ( size_t i = 0; i < val.size(); ++i )
            {
                if ( !append ( val.at ( i ) ) )
                {
                    return false;
                }
            }

            return true;
        }

        /// @brief Appends a boolean value to the JSON array
        /// @param [in] val The boolean value to save
        /// @return true on successful add; false on error
        bool append ( bool val );

        /// @brief Appends an int value to the JSON array
        /// @param [in] val The int value to save
        /// @return true on successful add; false on error
        bool append ( int val );

        /// @brief Appends an uint32_t value to the JSON array
        /// @param [in] val The uint32_t value to save
        /// @return true on successful add; false on error
        bool append ( uint32_t val );

        /// @brief Appends an int64_t value to the JSON array
        /// @param [in] val The int64_t value to save
        /// @return true on successful add; false on error
        bool append ( int64_t val );

        /// @brief Appends an uint64_t value to the JSON array
        /// @param [in] val The uint64_t value to save
        /// @return true on successful add; false on error
        bool append ( uint64_t val );

        /// @brief Appends a double value to the JSON array
        /// @param [in] val The double value to save
        /// @return true on successful add; false on error
        bool append ( double val );

        /// @brief Appends a string value to the JSON array
        /// @param [in] val The string value to save
        /// @return true on successful add; false on error
        bool append ( const String & val );

        /// @brief Appends a string value to the JSON array
        /// @param [in] val The string value to save
        /// @return true on successful add; false on error
        bool append ( const MemHandle & val );

        /// @brief Appends a null-terminated string value to the JSON array
        /// @param [in] val The null-terminated string value to save
        /// @return true on successful add; false on error
        bool append ( const char * val );

        /// @brief Appends an IP address value to the JSON array
        /// @param [in] val The IpAddress value to save
        /// @return true on successful add; false on error
        bool append ( const IpAddress & val );

        /// @brief Appends a JSON object to the JSON array
        /// @param [in] val The object to save
        /// @return true on successful add; false on error
        bool append ( const Json & val );

        /// @brief Appends a JSON array to the JSON array
        /// @param [in] val The array to save
        /// @return true on successful add; false on error
        bool append ( const JsonArray & val );

        /// @brief Appends the contents of a JSON array to the JSON array
        /// @param [in] val The array to append
        /// @return true on successful add; false on error
        bool appendContents ( const JsonArray & val );

        /// @brief Clear this array
        void clear();

    protected:
        /// @brief Called by decode to check _jptr to ensure it is the correct type for this class
        /// @return True if this object is the correct type
        virtual bool checkType() const;

    private:
        /// @brief A helper function, that allows easy creation get() functions for different numeric types.
        /// @param [in] idx The index to get.
        /// @param [out] val The value; remains the same on failure.
        /// @tparam MinValue Minimum value allowed.
        /// @tparam MaxValue Maximum value allowed.
        /// @tparam T The type of the value to get.
        /// @return Result code.
        template<int64_t MinValue, int64_t MaxValue, typename T> inline JsonOpCode rangeGet (
            size_t idx, T & val ) const
        {
            int64_t lVal = 0;
            JsonOpCode ret = get ( idx, lVal );

            if ( ret != JsonOpCode::Success )
            {
                return ret;
            }

            if ( lVal < MinValue || lVal > MaxValue )
            {
                return JsonOpCode::InvalidDataRange;
            }

            val = ( T ) lVal;

            return JsonOpCode::Success;
        }

        /// @brief Not available at the moment.
        JsonArray ( const JsonArray & );

        /// @brief Not available at the moment.
        const JsonArray & operator= ( const JsonArray & );
};
}
