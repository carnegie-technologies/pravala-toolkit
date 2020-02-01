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

#include "WebSocketFrame.hpp"

using namespace Pravala;

WebSocketFrame::WebSocketFrame ( WebSocketFrameHeader::OpCode opCode, bool isFin ):
    _data1 ( sizeof ( WebSocketHeader ) ) // No payload, we can use the minimum header
{
    assert ( !_data1.isEmpty() );

    if ( _data1.isEmpty() )
    {
        return;
    }

    WebSocketFrameHeader * hdr = reinterpret_cast<WebSocketFrameHeader *> ( _data1.getWritable() );

    if ( !hdr )
        return;

    const uint8_t hdrSize = hdr->setupWebSocketFrame ( opCode, isFin, 0, false );

    // Since there's no payload and no mask, this should be the same
    assert ( hdrSize == _data1.size() );

    ( void ) hdrSize;
}

WebSocketFrame::WebSocketFrame (
        WebSocketFrameHeader::OpCode opCode, bool isFin, bool mask,
        const MemHandle & payload )
{
    if ( mask )
    {
        // If we need to mask it, we'll need to copy it
        setupCopyPayload ( opCode, isFin, mask, payload.get(), payload.size() );
        return;
    }

    // Start with the maximum header size (which isn't very big anyways)
    _data1 = MemHandle ( sizeof ( WebSocketFrameHeader ) );

    assert ( !_data1.isEmpty() );

    if ( _data1.isEmpty() )
    {
        return;
    }

    WebSocketFrameHeader * hdr = reinterpret_cast<WebSocketFrameHeader *> ( _data1.getWritable() );

    if ( !hdr )
        return;

    const uint8_t hdrSize = hdr->setupWebSocketFrame ( opCode, isFin, payload.size(), mask );

    assert ( hdrSize <= _data1.size() );

    _data1.truncate ( hdrSize );

    _data2 = payload;
}

WebSocketFrame::WebSocketFrame (
        WebSocketFrameHeader::OpCode opCode, bool isFin, bool mask,
        const char * payload, size_t payloadLen )
{
    setupCopyPayload ( opCode, isFin, mask, payload, payloadLen );
}

void WebSocketFrame::setupCopyPayload (
        WebSocketFrameHeader::OpCode opCode, bool isFin, bool mask,
        const char * payload, size_t payloadLen )
{
    // Start with the maximum header size (which isn't very big anyways) + payload size
    _data1 = MemHandle ( sizeof ( WebSocketFrameHeader ) + payloadLen );

    assert ( !_data1.isEmpty() );

    char * data = _data1.getWritable();

    if ( !data || _data1.isEmpty() )
    {
        return;
    }

    assert ( data != 0 );

    WebSocketFrameHeader * hdr = reinterpret_cast<WebSocketFrameHeader *> ( data );

    const uint8_t hdrSize = hdr->setupWebSocketFrame ( opCode, isFin, payloadLen, mask );

    assert ( hdrSize <= _data1.size() );

    // data now points to the start of the payload portion
    data += hdrSize;

    if ( mask )
    {
        hdr->maskAndCopy ( data, payload, payloadLen );
    }
    else
    {
        memcpy ( data, payload, payloadLen );
    }

    _data1.truncate ( hdrSize + payloadLen );

    // We never put anything here, so this should be empty!
    assert ( _data2.isEmpty() );
}
