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

#include <cstddef>

#include "basic/MemHandle.hpp"
#include "basic/Buffer.hpp"

#include "ProtoError.hpp"
#include "ExtProtoError.hpp"

namespace Pravala
{
class Json;

/// @brief Class that should be inherited by all protocol messages
class Serializable
{
    public:
        /// @brief The max length of the length header.
        /// (the length field's header plus the actual length).
        static const size_t MaxLengthHeaderSize;

        /// @brief The field ID of the 'length' field.
        static const uint32_t LengthVarFieldId;

        /// @brief The type of the length variable.
        /// It uses signed type for compatibility with Java.
        typedef int32_t LengthVarType;

        /// @brief Serializes content of the object to the buffer
        ///
        /// First it calls setupDefines(). Next it verifies the validity of the object by calling validate().
        /// Then it calls serializeFields().
        ///
        /// It appends serialized data to the buffer. This function does not encode message's length.
        ///
        /// @param [in] buf The buffer to serialize the data to
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return The error code
        virtual ProtoError serialize ( Buffer & buf, ExtProtoError * extError = 0 );

        /// @brief Serializes content of the object to the buffer.
        /// Convenience function, it internally creates a buffer to append to, and generates a MemHandle based on it.
        /// This function does not encode message's length.
        ///
        /// @param [out] data This MemHandle will be set to contain the serialized content of the object.
        /// @param [in] preAllocateMemory The number of bytes to pre-allocate in the buffer.
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return The error code
        inline ProtoError serialize (
            MemHandle & data, size_t preAllocateMemory = 0, ExtProtoError * extError = 0 )
        {
            Buffer buf ( preAllocateMemory );
            const ProtoError eCode = serialize ( buf, extError );

            if ( IS_OK ( eCode ) )
            {
                data = buf.getHandle();
            }

            return eCode;
        }

        /// @brief Serializes content of the object to a Json object.
        ///
        /// First it calls setupDefines(). Next it verifies the validity of the object by calling validate().
        /// Then it calls serializeFields().
        ///
        /// @note It only works if the protocol implementation was generated with Json output enabled.
        /// @param [out] json The Json object to serialize the data into.
        ///                    It will be cleared before inserting elements to it.
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return The error code
        virtual ProtoError serialize ( Json & json, ExtProtoError * extError = 0 );

        /// @brief Serializes content of the object to the buffer
        ///
        /// It calls serialize() and also encodes the total payload's length.
        /// This version appends to an existing buffer. The drawback is that the payload length
        /// is always encoded using the maximum number of bytes.
        ///
        /// @param [in] buf The buffer to serialize the data to
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return The error code
        ProtoError serializeWithLength ( Buffer & buf, ExtProtoError * extError = 0 );

        /// @brief Serializes content of the object to the buffer
        ///
        /// It calls serialize() and also encodes the total payload's length.
        /// It returns new buffer with the serialized content of the object.
        /// This version creates a new buffer instead of appending to an existing one.
        /// The advantage over the other version is that the payload length is encoded in more
        /// efficient method, and instead of always using the maximum number of bytes for that
        /// field it encodes it using smaller number of bytes that depends on the actual length.
        ///
        /// @param [out] errCode The pointer to an error code. If it is non-zero the error code
        ///                      is placed there.
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return A new buffer with serialized content of the message, or 0 on error.
        MemHandle serializeWithLength ( ProtoError * errCode = 0, ExtProtoError * extError = 0 );

        /// @brief Deserializes data from the buffer
        ///
        /// It uses deserializeField() to deserialize each of the fields in the message.
        /// Then it checks validity of the data read using validate() function.
        ///
        /// @param [in] buf The buffer to deserialize the data from.
        /// @param [in] offset Offset in the buffer.
        /// @param [in] dataSize The size of data (starting at given offset).
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return The error code
        virtual ProtoError deserialize (
            const MemHandle & buf, size_t offset, size_t dataSize,
            ExtProtoError * extError = 0 );

        /// @brief Deserializes data from the buffer
        ///
        /// It uses deserializeField() to deserialize each of the fields in the message.
        /// Then it checks validity of the data read using validate() function.
        ///
        /// @param [in] buf The buffer to deserialize the data from (the entire buffer is used).
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return The error code
        inline ProtoError deserialize ( const MemHandle & buf, ExtProtoError * extError = 0 )
        {
            return deserialize ( buf, 0, buf.size(), extError );
        }

        /// @brief Deserializes data from the buffer.
        ///
        /// It detects the length of the message by reading the 'length field' that should be included in the buffer.
        /// Then it calls deserialize().
        ///
        /// @param [in] buf The buffer to deserialize the data from
        /// @param [in,out] offset Offset in the buffer to start from.
        ///                         It is modified (only if the message is deserialized properly).
        /// @param [out] missingBytes If used, the number of missing bytes is placed there.
        ///                            It is only set when 'Error::IncompleteData' is returned.
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return The error code
        ProtoError deserializeWithLength (
            const MemHandle & buf, size_t & offset, size_t * missingBytes = 0,
            ExtProtoError * extError = 0 );

        /// @brief Deserializes data from the buffer and consumes that data in the buffer.
        ///
        /// It is a convenience wrapper around deserializeWithLength ( MemHandle, ... ) function.
        /// It creates a MemHandle based on the entire buffer, clears the buffer and then
        /// deserializes the message. If there are any bytes that were not used - data that belongs
        /// to the next message, or all of the data in case there were any errors while deserializing,
        /// all the data is put back in the buffer.
        ///
        /// There should be no performance difference between this approach and using RwBuffer::consumeData,
        /// since re-adding all of the data back to the buffer doesn't require creating copies.
        /// On the other hand, if any data was consumed, memory reallocation and copying data happens in both cases
        /// (simply moving the data within existing memory - which would be slightly faster - is not possible,
        /// because - since we succeeded - there is a reference to the original data in SerializableMessage::_orgBuf).
        ///
        /// The preferred method, however, is to use MemHandle in a loop.
        /// Once there is a buffer with some data in it, a MemHandle should be created and the buffer cleared.
        /// Next the deserializeWithLength ( MemHandle ) version should be used and, on success,
        /// any remaining memory should be put in a new MemHandle using MemHandle::getHandle
        /// and deserializeWithLength ( MemHandle ) should be called on the new handle.
        /// Once there is an error (more data is needed), the remaining MemHandle should be appended to
        /// the original (now empty) buffer.
        ///
        /// @param [in] buf The buffer to deserialize the data from.
        /// @param [out] missingBytes If used, the number of missing bytes is placed there.
        ///                            It is only set when 'Error::IncompleteData' is returned.
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return The error code.
        ProtoError deserializeWithLength ( Buffer & buf, size_t * missingBytes = 0, ExtProtoError * extError = 0 );

        /// @brief Clears the content.
        ///
        /// All fields will either be set to their default values (or 0 if not set) or their clear()
        /// method will be called and they will be set as not present.
        virtual void clear() = 0;

        /// @brief Checks validity of the data
        ///
        /// Returns success if all required fields in this and all inherited objects (if any)
        /// are present and have legal values. If this is used by external code on messages
        /// or structures that are to be sent (NOT on received ones!)
        /// it is probably a good idea to call setupDefines() first.
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return Standard error code
        virtual ProtoError validate ( ExtProtoError * extError = 0 ) const = 0;

        /// @brief Sets the values of all the fields 'defined' by this and all inherited objects (if any)
        virtual void setupDefines() = 0;

        /// @brief Virtual destructor
        virtual ~Serializable();

    protected:
        /// @brief Serializes all fields to the buffer
        /// @param [in] buf The buffer to serialize the data to
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return The error code
        virtual ProtoError serializeFields ( Buffer & buf, ExtProtoError * extError ) = 0;

        /// @brief Serializes all fields of the object to a Json object.
        /// @note The default implementation returns 'Unsupported' error.
        /// @param [out] json The Json object to serialize the data into.
        ///                    It will be cleared before inserting elements to it.
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return The error code
        virtual ProtoError serializeFields ( Json & json, ExtProtoError * extError );

        /// @brief Deserializes a single field from the buffer
        ///
        /// @param [in] fieldId The ID of the field.
        /// @param [in] wireType Encoding type.
        /// @param [in] buf The buffer to deserialize the data from
        /// @param [in] offset Offset in the buffer.
        /// @param [in] fieldSize The size of this field (starting at given offset).
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return Standard error code; ProtocolWarning is treated as success and may mean that this message
        ///          didn't know the field, OR it was a valid object field, which, in turn,
        ///          experienced ProtocolWarning at some point.
        virtual ProtoError deserializeField (
            uint32_t fieldId, uint8_t wireType, const MemHandle & buf,
            size_t offset, size_t fieldSize,
            ExtProtoError * extError ) = 0;
};
}
