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
#include "DualIpFlow.hpp"

using namespace Pravala;

// MSVC doesn't like static const integrals defined in implementation files.
// It doesn't follow C++ spec (9.4.2/4), but there is not much we can do about it...
#ifndef _MSC_VER
const uint8_t DualIpFlow::SecondaryDescType;
#endif

DualIpFlow::DualIpFlow()
{
    _secondaryDesc.clear();
}

DualIpFlow::DualIpFlow ( const FlowDesc & defaultFlowDesc ):
    IpFlow ( defaultFlowDesc )
{
    _secondaryDesc.clear();
}

DualIpFlow::DualIpFlow ( const FlowDesc & defaultFlowDesc, const FlowDesc & secFlowDesc ):
    IpFlow ( defaultFlowDesc ),
    _secondaryDesc ( secFlowDesc )
{
}

DualIpFlow::~DualIpFlow()
{
}

void DualIpFlow::setSecondaryFlowDesc ( const FlowDesc & flowDesc )
{
    // We must NOT be a part of a map.
    // We cannot check that for sure, but if _next is set, we are a part of a map...
    assert ( !getNext() );

    _secondaryDesc = flowDesc;
}

bool DualIpFlow::conflictsWith ( IpFlow * otherFlow )
{
    return IpFlow::conflictsWith ( otherFlow )
           || ( _secondaryDesc.isValid()
                && otherFlow != 0
                && otherFlow->matchFlow ( _secondaryDesc, SecondaryDescType ) );
}

bool DualIpFlow::matchFlow ( const FlowDesc & flowDesc, uint8_t descType ) const
{
    if ( descType == SecondaryDescType && _secondaryDesc.isValid() )
    {
        return ( flowDesc.isValid() && flowDesc == _secondaryDesc );
    }

    return IpFlow::matchFlow ( flowDesc, descType );
}

bool DualIpFlow::mapInsert ( IpFlowMap & flowMap )
{
    if ( !IpFlow::mapInsert ( flowMap ) )
    {
        return false;
    }

    if ( !_secondaryDesc.isValid() || flowMap.flowInsert ( this, _secondaryDesc ) )
    {
        // When the secondary descriptor is not valid, we don't want to use it.
        return true;
    }

    // We failed to insert using the secondary descriptor.
    // We don't want "partial inserts", so let's remove the first one:

    IpFlow::mapRemove ( flowMap );
    return false;
}

void DualIpFlow::mapRemove ( IpFlowMap & flowMap )
{
    if ( !_secondaryDesc.isValid() )
    {
        IpFlow::mapRemove ( flowMap );
        return;
    }

    // Regular remove simply replaces the 'next' pointer in previous
    // object with the 'next' value fro mthe object being removed.
    // This object has two "incoming" pointers, one for each of the descriptors.
    // Each object after this one in the list is there beacause it should "follow" one of those pointers.
    // We don't know which one, so we have to re-insert all of them.
    // To do that, we take over the remainder of the list, and then re-insert all objects in it,
    // one by one.

    IpFlow * next = stealNext();

    assert ( !getNext() );

    IpFlow::mapRemove ( flowMap );

    flowMap.flowRemove ( this, _secondaryDesc );

    while ( next != 0 )
    {
        IpFlow * const ptr = next;
        next = ptr->stealNext();

        const bool ret = ptr->mapInsert ( flowMap );

        // Re-inserting really should work, since all those objects were in the flow map already!

        ( void ) ret;
        assert ( ret );
    }
}
