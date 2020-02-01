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

#include "IpFlowMap.hpp"

using namespace Pravala;

IpFlowMap::IpFlowMap ( uint8_t bitSize ):
    _bitSize ( bitSize ),
    _mapSize ( 1 << _bitSize ),
    _bitMask ( _mapSize - 1 ),
    _flows ( new IpFlow *[ _mapSize ] ),
    _usedBuckets ( 0 )
{
    assert ( _bitSize >= 8 );
    assert ( _bitSize <= 30 );
    assert ( _bitMask < _mapSize );
    assert ( _bitMask + 1 == _mapSize );

    memset ( _flows, 0, sizeof ( IpFlow * ) * _mapSize );
}

IpFlowMap::~IpFlowMap()
{
    clearMap();

#ifndef NDEBUG
    for ( size_t i = 0; i < _mapSize; ++i )
    {
        assert ( !_flows[ i ] );
    }
    assert ( _usedBuckets == 0 );
#endif

    delete[] _flows;
}

bool IpFlowMap::flowInsert ( IpFlow * flow, const FlowDesc & flowDesc )
{
    if ( !flow || !flowDesc.isValid() )
    {
        return false;
    }

    const uint32_t index = getIndex ( flowDesc );

    assert ( index < _mapSize );

    IpFlow * ptr = _flows[ index ];

    if ( !ptr )
    {
        _flows[ index ] = flow;

        // This bucket was empty. Now it isn't.
        assert ( _flows[ index ] != 0 );

        ++_usedBuckets;

        return true;
    }

    while ( true )
    {
        if ( ptr == flow )
        {
            // This object is already in the list...
            return true;
        }

        if ( ptr->conflictsWith ( flow ) || flow->conflictsWith ( ptr ) )
        {
            // These two flows conflict with each other.
            // They cannot be in the map at the same time.
            return false;
        }

        if ( ptr->setNext ( flow ) )
        {
            return true;
        }

        ptr = ptr->getNext();

        // Otherwise 'setNext' would have worked:
        assert ( ptr != 0 );
    }

    assert ( false );

    return false;
}

void IpFlowMap::flowRemove ( IpFlow * flow, const FlowDesc & flowDesc )
{
    if ( !flow || !flowDesc.isValid() )
    {
        return;
    }

    const uint32_t index = getIndex ( flowDesc );

    assert ( index < _mapSize );

    IpFlow * prev = 0;
    IpFlow * ptr = _flows[ index ];

    while ( ptr != 0 )
    {
        if ( ptr != flow )
        {
            prev = ptr;
            ptr = ptr->getNext();
            continue;
        }

        IpFlow * const next = ptr->stealNext();

        if ( !prev )
        {
            assert ( _flows[ index ] == ptr );

            _flows[ index ] = next;

            if ( !next )
            {
                // This bucket was non-empty. Now it is empty.
                assert ( _usedBuckets > 0 );

                --_usedBuckets;
            }
        }
        else
        {
            // We need to unset 'next' in the previous object, to be able to update it.
            IpFlow * const tmp = prev->stealNext();

            ( void ) tmp;
            assert ( tmp == ptr );

            const bool ret = prev->setNext ( next );

            ( void ) ret;
            assert ( ret );
        }

        return;
    }
}

IpFlow * IpFlowMap::findFlow ( const FlowDesc & flowDesc, uint8_t descType, ExpiryMode expMode )
{
    if ( !flowDesc.isValid() )
    {
        return 0;
    }

    const uint32_t index = getIndex ( flowDesc );

    assert ( index < _mapSize );

    for ( IpFlow * ptr = _flows[ index ]; ptr != 0; )
    {
        if ( expMode == ExpireFlows && ptr->isExpired() )
        {
            // There is a flow in the list that is now expired.
            // Let's clear this bucket, and start over.
            // Also, we only want to do this once, so we switch to "don't expire" mode.

            expMode = DontExpireFlows;
            cleanupFlows ( index );
            ptr = _flows[ index ];

            continue;
        }

        if ( ptr->matchFlow ( flowDesc, descType ) )
        {
            return ptr;
        }

        ptr = ptr->getNext();
    }

    return 0;
}

const IpFlow * IpFlowMap::findFlow ( const FlowDesc & flowDesc, uint8_t descType ) const
{
    if ( !flowDesc.isValid() )
    {
        return 0;
    }

    const uint32_t index = getIndex ( flowDesc );

    assert ( index < _mapSize );

    for ( const IpFlow * ptr = _flows[ index ]; ptr != 0; ptr = ptr->getNext() )
    {
        if ( ptr->matchFlow ( flowDesc, descType ) )
        {
            return ptr;
        }
    }

    return 0;
}

void IpFlowMap::cleanupFlows ( uint32_t index )
{
    if ( index >= _mapSize )
    {
        assert ( false );
        return;
    }

    if ( !_flows[ index ] )
    {
        // Nothing to do...
        return;
    }

    // To perform flow cleanup, we remove the entire list from the map.
    // Then we will inspect flows one by one, and re-insert them if needed.

    IpFlow * ptr = _flows[ index ];
    _flows[ index ] = 0;

    assert ( _usedBuckets > 0 );

    // We just emptied one of the buckets.
    --_usedBuckets;

    while ( ptr != 0 )
    {
        // We want to unset 'next' pointer.
        // This way removing or re-inserting flows won't affect this list.
        IpFlow * const next = ptr->stealNext();

        if ( ptr->isExpired() )
        {
            // There may be other pointers to this flow:
            ptr->mapRemove ( *this );
            ptr->flowRemoved();
        }
        else
        {
            // Not expired yet, let's re-insert it:
            ptr->mapInsert ( *this );
        }

        ptr = next;
    }
}

void IpFlowMap::clearMap()
{
    if ( _usedBuckets < 1 )
    {
        return;
    }

    for ( uint32_t index = 0; index < _mapSize; ++index )
    {
        IpFlow * ptr = _flows[ index ];
        _flows[ index ] = 0;

        if ( !ptr )
        {
            continue;
        }

        assert ( _usedBuckets > 0 );

        // We just emptied one of the buckets.
        --_usedBuckets;

        while ( ptr != 0 )
        {
            // We want to unset 'next' pointer.
            // This way removing or re-inserting flows won't affect this list.
            IpFlow * const next = ptr->stealNext();

            ptr->flowRemoved();
            ptr = next;
        }
    }
}
