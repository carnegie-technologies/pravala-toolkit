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
class JsonArray;

/// @brief A wrapper class for a JSON object built with the Jansson JSON library
class Json: public JsonCore
{
    public:
        /// @brief Constructor
        Json();

        /// @brief Destructor
        virtual ~Json()
        {
        }

        /// @brief Retrieves a boolean from the JSON using the specified key
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        JsonOpCode get ( const char * key, bool & val ) const;

        /// @brief Retrieves an int from the JSON using the specified key
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        JsonOpCode get ( const char * key, int64_t & val ) const;

        /// @brief Retrieves an int from the JSON using the specified key
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        inline JsonOpCode get ( const char * key, uint64_t & val ) const
        {
            return get ( key, ( int64_t & ) val );
        }

        /// @brief Retrieves an int32_t from the JSON using the specified key
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        inline JsonOpCode get ( const char * key, int32_t & val ) const
        {
            return rangeGet< -0x80000000LL, 0x7FFFFFFFLL > ( key, val );
        }

        /// @brief Retrieves an uint32_t from the JSON using the specified key
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        inline JsonOpCode get ( const char * key, uint32_t & val ) const
        {
            return rangeGet<0, 0xFFFFFFFFLL > ( key, val );
        }

        /// @brief Retrieves a int16_t from the JSON using the specified key
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        inline JsonOpCode get ( const char * key, int16_t & val ) const
        {
            return rangeGet< -0x8000, 0x7FFF > ( key, val );
        }

        /// @brief Retrieves a uint16_t from the JSON using the specified key
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        inline JsonOpCode get ( const char * key, uint16_t & val ) const
        {
            return rangeGet<0, 0xFFFF> ( key, val );
        }

        /// @brief Retrieves an int8_t from the JSON using the specified key
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        inline JsonOpCode get ( const char * key, int8_t & val ) const
        {
            return rangeGet< -0x80, 0x7F > ( key, val );
        }

        /// @brief Retrieves an uint8_t from the JSON using the specified key
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        inline JsonOpCode get ( const char * key, uint8_t & val ) const
        {
            return rangeGet<0, 0xFF > ( key, val );
        }

        /// @brief Retrieves a double from the JSON using the specified key
        /// @note If the JSON value is an integer, it will be read properly (and stored as a double).
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        JsonOpCode get ( const char * key, double & val ) const;

        /// @brief Retrieves a string from the JSON using the specified key
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        JsonOpCode get ( const char * key, String & val ) const;

        /// @brief Retrieves a string from the JSON using the specified key, and stores it in a MemHandle object.
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure. Existing content is replaced on success.
        /// @return Result code.
        JsonOpCode get ( const char * key, MemHandle & val ) const;

        /// @brief Retrieves an IP address from the JSON using the specified key
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        JsonOpCode get ( const char * key, IpAddress & val ) const;

        /// @brief Retrieves a JSON object from the JSON using the specified key
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        JsonOpCode get ( const char * key, Json & val ) const;

        /// @brief Retrieves a JSON array from the JSON using the specified key
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        JsonOpCode get ( const char * key, JsonArray & val ) const;

        /// @brief Compatibility function that retrieves a String List from the JSON using the specified key
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @return Result code.
        JsonOpCode get ( const char * key, StringList & val ) const;

        /// @brief A helper function, that allows easy use of get() functions with String keys
        /// @note We cannot do the same trick with 'put' operation,
        ///        due to implicit type casting when const references are used.
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @tparam T The type of the value to get.
        /// @return Result code.
        template<typename T> inline JsonOpCode get ( const String & key, T & val ) const
        {
            return get ( key.c_str(), val );
        }

        /// @brief Inserts a boolean value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The boolean value to save
        /// @return true on successful add; false on error
        bool put ( const char * key, bool val );

        /// @brief Inserts a boolean value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The boolean value to save
        /// @return true on successful add; false on error
        inline bool put ( const String & key, bool val )
        {
            return put ( key.c_str(), val );
        }

        /// @brief Inserts an int value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The int value to save
        /// @return true on successful add; false on error
        bool put ( const char * key, int val );

        /// @brief Inserts an int value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The int value to save
        /// @return true on successful add; false on error
        inline bool put ( const String & key, int val )
        {
            return put ( key.c_str(), val );
        }

        /// @brief Inserts an uint32_t value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The uint32_t value to save
        /// @return true on successful add; false on error
        bool put ( const char * key, uint32_t val );

        /// @brief Inserts an uint32_t value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The uint32_t value to save
        /// @return true on successful add; false on error
        inline bool put ( const String & key, uint32_t val )
        {
            return put ( key.c_str(), val );
        }

        /// @brief Inserts an int64_t value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The int64_t value to save
        /// @return true on successful add; false on error
        bool put ( const char * key, int64_t val );

        /// @brief Inserts an int64_t value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The int64_t value to save
        /// @return true on successful add; false on error
        inline bool put ( const String & key, int64_t val )
        {
            return put ( key.c_str(), val );
        }

        /// @brief Inserts an uint64_t value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The uint64_t value to save
        /// @return true on successful add; false on error
        bool put ( const char * key, uint64_t val );

        /// @brief Inserts an uint64_t value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The uint64_t value to save
        /// @return true on successful add; false on error
        inline bool put ( const String & key, uint64_t val )
        {
            return put ( key.c_str(), val );
        }

        /// @brief Inserts a double value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The double value to save
        /// @return true on successful add; false on error
        bool put ( const char * key, double val );

        /// @brief Inserts a double value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The double value to save
        /// @return true on successful add; false on error
        inline bool put ( const String & key, double val )
        {
            return put ( key.c_str(), val );
        }

        /// @brief Inserts a string value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The string value to save
        /// @return true on successful add; false on error
        bool put ( const char * key, const String & val );

        /// @brief Inserts a string value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The string value to save
        /// @return true on successful add; false on error
        inline bool put ( const String & key, const String & val )
        {
            return put ( key.c_str(), val );
        }

        /// @brief Inserts a string value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The string value to save
        /// @return true on successful add; false on error
        bool put ( const char * key, const MemHandle & val );

        /// @brief Inserts a string value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The string value to save
        /// @return true on successful add; false on error
        inline bool put ( const String & key, const MemHandle & val )
        {
            return put ( key.c_str(), val );
        }

        /// @brief Inserts a null-terminated string value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The null-terminated string value to save
        /// @return true on successful add; false on error
        bool put ( const char * key, const char * val );

        /// @brief Inserts a null-terminated string value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The null-terminated string value to save
        /// @return true on successful add; false on error
        inline bool put ( const String & key, const char * val )
        {
            return put ( key.c_str(), val );
        }

        /// @brief Inserts an IP address value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The IpAddress value to save
        /// @return true on successful add; false on error
        bool put ( const char * key, const IpAddress & val );

        /// @brief Inserts an IP address value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The IpAddress value to save
        /// @return true on successful add; false on error
        inline bool put ( const String & key, const IpAddress & val )
        {
            return put ( key.c_str(), val );
        }

        /// @brief Inserts a Timestamp value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The Timestamp value to save
        /// @return true on successful add; false on error
        bool put ( const char * key, const Timestamp & val );

        /// @brief Inserts a Timestamp value into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The Timestamp value to save
        /// @return true on successful add; false on error
        inline bool put ( const String & key, const Timestamp & val )
        {
            return put ( key.c_str(), val );
        }

        /// @brief Inserts a JSON object/JSON array into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The JSON object/JSON array value to save
        /// @return true on successful add; false on error
        bool put ( const char * key, JsonCore & val );

        /// @brief Inserts a JSON object into the JSON object with the specified key
        /// @param [in] key The key to save this value as
        /// @param [in] val The JSON object/JSON array value to save
        /// @return true on successful add; false on error
        inline bool put ( const String & key, JsonCore & val )
        {
            return put ( key.c_str(), val );
        }

        /// @brief Clear this object
        void clear();

    protected:
        /// @brief Called by decode to check _jptr to ensure it is the correct type for this class
        /// @return True if this object is the correct type
        virtual bool checkType() const;

    private:
        /// @brief A helper function, that allows easy creation get() functions for different numeric types.
        /// @param [in] key The key to check
        /// @param [out] val The value; remains the same on failure.
        /// @tparam MinValue Minimum value allowed.
        /// @tparam MaxValue Maximum value allowed.
        /// @tparam T The type of the value to get.
        /// @return Result code.
        template<int64_t MinValue, int64_t MaxValue, typename T> inline JsonOpCode rangeGet (
            const char * key, T & val ) const
        {
            int64_t lVal = 0;
            JsonOpCode ret = get ( key, lVal );

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
        Json ( const Json & );

        /// @brief Not available at the moment.
        const Json & operator= ( const Json & );
};
}
