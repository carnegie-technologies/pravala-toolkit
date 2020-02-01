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

#include "basic/Buffer.hpp"
#include "SerializableMessage.hpp"
#include "ProtocolCodec.hpp"

#include <cassert>

using namespace Pravala;

void SerializableMessage::clearOrgBuffer()
{
    _orgBuf.clear();
}

ProtoError SerializableMessage::deserialize (
        const MemHandle & buf, size_t offset, size_t dataSize, ExtProtoError * extError )
{
    _orgBuf.clear();

    const ProtoError ret = Serializable::deserialize ( buf, offset, dataSize, extError );

    if ( IS_OK ( ret ) )
    {
        // It went fine (unknown fields are still OK). We want to keep the buffer for later:
        _orgBuf = buf.getHandle ( offset, dataSize );
    }

    return ret;
}

ProtoError SerializableMessage::deserializeFromBase ( const SerializableMessage & other, ExtProtoError * extError )
{
    return deserialize ( other.getOrgBuffer(), 0, other.getOrgBuffer().size(), extError );
}
