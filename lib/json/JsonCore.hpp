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

#include "basic/MemHandle.hpp"
#include "basic/Buffer.hpp"
#include "basic/NoCopy.hpp"

struct json_t;

namespace Pravala
{
/// @brief A forward declaration of "JsonError"
/// Because json_error_t is a typedef for an unnamed structure, we can't forward-declare it directly.
/// This structure simply inherits json_error_t and doesn't add anything.
/// So a pointer of this type can be treated as a pointer to json_error_t in the code that needs to care.
struct JsonError;

/// @brief A wrapper class for the Jansson JSON library.
/// @note Right now this object cannot be copied (so Json and JsonArray cannot be copied as well).
///       This is because simply creating a new reference the json_t pointer would not copy the data,
///       which means that modifying either of those "copies" would modify both.
///       Once a proper copy constructor or assignment operator is needed, that functionality may be added
///       (by performing a deep copy of the json object).
class JsonCore: public NoCopy
{
    public:

        /// @brief Compares two JsonCore objects to determine equality.
        /// @param [in] other The second JsonCore to compare
        /// @return A value indicating whether the two objects are equal
        bool operator== ( const JsonCore & other ) const;

        /// @brief Compares two JsonCore objects to determine inequality.
        /// @param [in] other The second JsonCore to compare
        /// @return A value indicating whether the two objects are inequal
        inline bool operator!= ( const JsonCore & other ) const
        {
            return !operator== ( other );
        }

        /// @brief Checks if this is a JSON array or not
        /// @return true if an array; false otherwise
        bool isArray() const;

        /// @brief Checks if this is a JSON object or not
        /// @return true if an object, false otherwise
        bool isObject() const;

        /// @brief Decode some payload. This will remove any existing JSON entities and start from scratch
        /// @param [in] payload The null terminated string to decode
        /// @return True on success; False on error
        ///          The error message can later be obtained using getLastErrorMessage().
        bool decode ( const char * payload );

        /// @brief Decode some payload. This will remove any existing JSON entities and start from scratch
        /// @param [in] data The pointer to the data to decode
        /// @param [in] len The length of the data to decode
        /// @return True on success; False on error
        ///          The error message can later be obtained using getLastErrorMessage().
        bool decode ( const char * data, size_t len );

        /// @brief Decode some payload. This will remove any existing JSON entities and start from scratch
        /// @param [in] payload The string to decode
        /// @return True on success; False on error
        ///          The error message can later be obtained using getLastErrorMessage().
        bool decode ( const String & payload );

        /// @brief Decode some payload. This will remove any existing JSON entities and start from scratch
        /// @param [in] payload The buffer to decode.
        /// @return True on success; False on error
        ///          The error message can later be obtained using getLastErrorMessage().
        bool decode ( const MemHandle & buf );

        /// @brief Encode this object into a string. This will clear any value in payload by default
        /// @param [out] payload The encoded string.
        /// @param [in] resetPayload If true, call clear() on the string before serializing.
        /// @return true on success; false if JSON object is empty
        bool encode ( String & payload, bool resetPayload = true ) const;

        /// @brief Encode this object into a buffer. This will clear the buffer by default
        /// @param [out] buf The buffer to store the content in.
        ///                  In case of error some data may still be written to this buffer.
        /// @param [in] resetPayload If true, call clear() on the buffer before serializing.
        /// @return true on success; false if JSON object is empty
        bool encode ( Buffer & buf, bool resetBuffer = true ) const;

        /// @brief Encode this object into a buffer.
        /// @param [out] buf The buffer to store the content in.
        ///                  It is not modified on error.
        ///                  On success, the MemHandle will be replaced with a new one (with a new content).
        /// @return true on success; false if JSON object is empty
        bool encode ( MemHandle & buf ) const;

        /// @brief Returns the error message for the last JSON decoding error
        /// @note The error is not cleared after successful calls.
        /// @return The description of the last JSON decoding error, or an empty string if there is no error stored
        String getLastErrorMessage() const;

        /// @brief Returns the string representation of this JSON object
        /// If there is encoding error, the generated string will be empty.
        /// @return The string representation of this JSON object
        inline String toString() const
        {
            String buffer;
            encode ( buffer );
            return buffer;
        }

        /// @brief Returns the string representation of this JSON object as a MemHandle object.
        /// If there is encoding error, the generated handle will be empty.
        /// @return The string representation of this JSON object as a MemHandle object.
        inline MemHandle toMemHandle() const
        {
            MemHandle mh;
            encode ( mh );
            return mh;
        }

        /// @brief Clears the last error stored in this object.
        void clearError();

    protected:
        /// @brief Constructor
        JsonCore();

        /// @brief Destructor
        virtual ~JsonCore();

        /// @brief Set the JSON pointer to a (potentially) new value
        /// @note This function manages the references for the pointers correctly -- i.e. increments
        /// the reference counter on jptr before setting _jptr to it, and decrementing the counter on
        /// _jptr before unsetting it.
        /// @warning This creates a reference to the original object, not an actual copy!
        ///           Changes in the new object will affect the original object as well!
        /// @param [in] jptr JSON pointer to set
        void setJptr ( json_t * jptr );

        /// @brief Called by decode to check _jptr to ensure it is the correct type for this class
        /// (e.g. checks if _jptr is a JSON array if the implementing class is JsonArray)
        /// @return True if this object is the correct type
        virtual bool checkType() const = 0;

        /// @brief A pointer to the actual Jansson data structure
        /// @note This pointer can be used as a parameter in Jansson's get and set type functions,
        /// but it should not be cleared or changed, and incref/decref should not be called on it,
        /// outside of functions in this class.
        json_t * _jptr;

        /// @brief A pointer to the last JSON error
        JsonError * _jerr;

        friend class Json;
        friend class JsonArray;
};
}
