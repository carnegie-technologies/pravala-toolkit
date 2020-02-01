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

#include "List.hpp"

using namespace Pravala;

// ListPriv::ListPriv() :
//     buffer ( 0 ), bufSize ( 0 ), dataSize ( 0 ), begIndex ( 0 ), endIndex ( 0 )
// {
//     assert ( ref.count() == 1 );
//     buffer = static_cast<SharedMemory::Pointer *> ( malloc ( sizeof ( SharedMemory::Pointer ) ) );
//     buffer[0] = 0;
// }

ListPriv::ListPriv ( size_t sizeHint ):
    buffer ( 0 ), bufSize ( sizeHint ), dataSize ( 0 ), begIndex ( 0 ), endIndex ( 0 )
{
    assert ( ref.count() == 1 );

    if ( bufSize < MinSize )
    {
        bufSize = MinSize;
    }

    assert ( bufSize > 0 );

    buffer = static_cast<SharedMemory::Pointer *> ( malloc ( bufSize * sizeof ( SharedMemory::Pointer ) ) );
}

ListPriv::~ListPriv()
{
    assert ( ref.count() == 0 );

    free ( buffer );
    buffer = 0;
    bufSize = dataSize = begIndex = endIndex = 0;
}

void ListPriv::swapContent ( ListPriv * other )
{
    assert ( other != 0 );

    SharedMemory::Pointer * orgBuffer = buffer;

    buffer = other->buffer;
    other->buffer = orgBuffer;

    size_t tmpSize = bufSize;
    bufSize = other->bufSize;
    other->bufSize = tmpSize;

    tmpSize = dataSize;
    dataSize = other->dataSize;
    other->dataSize = tmpSize;

    tmpSize = begIndex;
    begIndex = other->begIndex;
    other->begIndex = tmpSize;

    tmpSize = endIndex;
    endIndex = other->endIndex;
    other->endIndex = tmpSize;
}
