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

#include <cassert>

#include "HashMap.hpp"

#define NEW_BUF_SIZE( n )    ( ( n ) * 2 )
#define MIN_BUF_SIZE    8

using namespace Pravala;

// HashMapPriv::HashMapPriv() :
//     buffer ( 0 ), bufSize ( 1 ), elementCount ( 0 )
// {
//     assert ( ref.count() == 1 );
//     buffer = new PointerPair[1];
//     buffer[0].keys = 0;
//     buffer[0].values = 0;
// }

HashMapPriv::~HashMapPriv()
{
    assert ( ref.count() == 0 );

    if ( ref.count() < 1 )
    {
        assert ( ref.count() == 0 );

        delete[] buffer;
        buffer = 0;

        elementCount = bufSize = 0;
    }
}

HashMapPriv::HashMapPriv ( HashMapPriv * other, bool increaseSize ):
    buffer ( 0 ), bufSize ( 0 ), elementCount ( 0 )
{
    // This doesn't unreference!
    // It can't unreference because it doesn't copy the data.
    // And it doesn't copy the data, because it doesn't know the type.

    assert ( ref.count() == 1 );

    size_t oSize = 0;

    if ( other != 0 )
        bufSize = oSize = other->bufSize;

    if ( bufSize < MIN_BUF_SIZE )
        bufSize = MIN_BUF_SIZE;

    assert ( oSize == 0 || bufSize == oSize );

    if ( other != 0 && increaseSize )
    {
        bufSize = NEW_BUF_SIZE ( bufSize );

        if ( bufSize < MIN_BUF_SIZE )
            bufSize = MIN_BUF_SIZE;

        if ( bufSize < oSize + 2 )
            bufSize = oSize + 2;
    }

    assert ( bufSize > 0 );
    assert ( bufSize >= oSize );

    elementCount = 0;

    buffer = new PointerPair[ bufSize ];

    memset ( buffer, 0, sizeof ( PointerPair ) * bufSize );
}
