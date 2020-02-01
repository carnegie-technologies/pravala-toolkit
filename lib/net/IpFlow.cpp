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
#include "IpFlow.hpp"

using namespace Pravala;

// MSVC doesn't like static const integrals defined in implementation files.
// It doesn't follow C++ spec (9.4.2/4), but there is not much we can do about it...
#ifndef _MSC_VER
const uint8_t IpFlow::DefaultDescType;
#endif

IpFlow::IpFlow(): _next ( 0 )
{
    _flowDesc.clear();
}

IpFlow::IpFlow ( const FlowDesc & flowDesc ): _flowDesc ( flowDesc ), _next ( 0 )
{
}

IpFlow::~IpFlow()
{
    assert ( !_next );
}

void IpFlow::setDefaultFlowDesc ( const FlowDesc & flowDesc )
{
    // We must NOT be a part of a map.
    // We cannot check that for sure, but if _next is set, we are a part of a map...
    assert ( !_next );

    _flowDesc = flowDesc;
}

bool IpFlow::isExpired()
{
    return false;
}

void IpFlow::flowRemoved()
{
}

bool IpFlow::conflictsWith ( IpFlow * otherFlow )
{
    return ( _flowDesc.isValid() && otherFlow != 0 && otherFlow->matchFlow ( _flowDesc, DefaultDescType ) );
}

bool IpFlow::matchFlow ( const FlowDesc & flowDesc, uint8_t /*descType*/ ) const
{
    return ( flowDesc.isValid() && flowDesc == _flowDesc );
}

bool IpFlow::mapInsert ( IpFlowMap & flowMap )
{
    return flowMap.flowInsert ( this, _flowDesc );
}

void IpFlow::mapRemove ( IpFlowMap & flowMap )
{
    flowMap.flowRemove ( this, _flowDesc );

    // It should have been cleared, or never set:
    assert ( !_next );
}
