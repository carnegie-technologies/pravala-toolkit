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

#include "ProtoError.hpp"

using namespace Pravala;

#define CASE_CODE( c ) \
    case ProtoError::c: \
        return #c; break

const char * ProtoError::toString() const
{
    switch ( _code )
    {
        CASE_CODE ( Success );
        CASE_CODE ( ProtocolWarning );
        CASE_CODE ( InvalidParameter );
        CASE_CODE ( InternalError );
        CASE_CODE ( MemoryError );
        CASE_CODE ( TooMuchData );
        CASE_CODE ( IncompleteData );
        CASE_CODE ( ProtocolError );
        CASE_CODE ( InvalidDataSize );
        CASE_CODE ( TooBigValue );
        CASE_CODE ( RequiredFieldNotSet );
        CASE_CODE ( DefinedValueMismatch );
        CASE_CODE ( FieldValueOutOfRange );
        CASE_CODE ( StringLengthOutOfRange );
        CASE_CODE ( ListSizeOutOfRange );
        CASE_CODE ( Unsupported );

        case ProtoError::Unknown:
            break;
    }

    return "Unknown";
}
