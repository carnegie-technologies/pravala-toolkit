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

extern "C"
{
// Using the C header since the C++ version of their header depends on std
#include "b64/cencode.h"
#include "b64/cdecode.h"
}

#include "Base64.hpp"

using namespace Pravala;

MemHandle Base64::encode ( const char * s, size_t len, bool addLineBreaks )
{
    base64_encodestate state;
    base64_init_encodestate ( &state );

    // Encoded string is larger than original, 2x length is safe, even if there are line breaks every 72 chars
    MemHandle mh ( 2 * len );

    char * const enc = mh.getWritable();

    if ( !enc || mh.size() < ( 2 * len ) )
    {
        mh.clear();
        return mh;
    }

    assert ( enc != 0 );

    size_t encLen = base64_encode_block ( s, len, enc, &state, addLineBreaks ? 1 : 0 );

    encLen += base64_encode_blockend ( enc + encLen, &state, addLineBreaks ? 1 : 0 );

    assert ( encLen <= mh.size() );

    mh.truncate ( encLen );

    return mh;
}

MemHandle Base64::decode ( const char * s, size_t len )
{
    base64_decodestate state;
    base64_init_decodestate ( &state );

    // Decoded string is smaller than original, but we don't know how much, so len is safe
    MemHandle mh ( len );

    char * const dec = mh.getWritable();

    if ( !dec || mh.size() < len )
    {
        mh.clear();

        return mh;
    }

    const size_t decLen = base64_decode_block ( s, len, dec, &state );

    assert ( decLen < mh.size() );

    mh.truncate ( decLen );

    return mh;
}
