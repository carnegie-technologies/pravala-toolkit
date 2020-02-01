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
/// @brief Class that should be inherited by all base messages.
/// The difference between SerializableMessage, and a "simple" Serializable object,
/// is that deserialized messages keep a reference to the original buffer
/// and can be used for deserializing more specific messages, that are lower
/// in the inheritance tree than the message object originally deserialized.
class SerializableMessage: public Serializable
{
    public:
        /// @brief Returns the original data buffer from which this object was deserialized
        /// @return the original data buffer from which this object was deserialized
        inline const MemHandle & getOrgBuffer() const
        {
            return _orgBuf;
        }

        /// @brief This functions clears the original buffer from which this object was deserialized
        void clearOrgBuffer();

        virtual ProtoError deserialize (
            const MemHandle & buf, size_t offset, size_t dataSize,
            ExtProtoError * extError = 0 );

        using Serializable::deserialize;

    protected:
        // @brief Deserializes using 'original buffer' stored in another object.
        ///
        /// For this to work, the other object has to still contain the original buffer.
        /// If it doesn't, the IncompleteData error is returned.
        ///
        /// It is protected and should be used by base message's deserialize ( message ) function,
        /// which should be testing defines.
        /// If it succeeds, a reference to the original buffer will be stored in this object as well.
        ///
        /// @param [in] other The other object to use
        /// @param [out] extError Pointer to extended error code if it should be used (only modified on error).
        /// @return The error code
        ProtoError deserializeFromBase ( const SerializableMessage & other, ExtProtoError * extError = 0 );

        /// @brief Should be called whenever the message object is modified.
        inline void messageModified()
        {
            clearOrgBuffer();
        }

    private:
        MemHandle _orgBuf; ///< Original data buffer
};
}
