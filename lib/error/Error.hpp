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

#include "proto/ProtoError.hpp"

#include "auto/error/ErrorCode.hpp"

#define UNTIL_ERROR( errCode, expression ) \
    do { \
        if ( IS_OK ( errCode ) ) errCode = ( expression ); \
    } while ( false )

#define WHILE_ERROR( errCode, expression ) \
    do { \
        if ( NOT_OK ( errCode ) ) errCode = ( expression ); \
    } while ( false )

#define UNTIL_FALSE( isOk, expression ) \
    do { \
        if ( isOk ) isOk = ( expression ); \
    } while ( false )

#define WHILE_FALSE( isOk, expression ) \
    do { \
        if ( !isOk ) isOk = ( expression ); \
    } while ( false )

namespace Pravala
{
class ExtProtoError;
typedef ErrorCode Error;

/// @brief A wrapper around auto-generated ErrorCode protocol enum.
/// It allows ProtoError values to be converted to ErrorCode values.
struct ERRCODE: public ErrorCode
{
    /// @brief Default constructor.
    /// Sets the error to 'InvalidError' value.
    inline ERRCODE()
    {
    }

    /// @brief Copy constructor.
    /// @param [in] other The ErrorCode to copy.
    inline ERRCODE ( const ErrorCode & other ): ErrorCode ( other )
    {
    }

    /// @brief Copy constructor.
    /// @param [in] other The ErrorCode's internal enum value to copy.
    inline ERRCODE ( const ErrorCode::_EnumType & other ): ErrorCode ( other )
    {
    }

    /// @brief Copy constructor.
    /// @param [in] other The ProtoError value to copy. This value will be mapped to one of ErrorCode's values.
    inline ERRCODE ( const ProtoError::Code & code ): ErrorCode ( protoToErrCode ( code ) )
    {
    }

    /// @brief Copy constructor.
    /// @param [in] other The ProtoError to copy. This value will be mapped to one of ErrorCode's values.
    inline ERRCODE ( const ProtoError & other ): ErrorCode ( protoToErrCode ( other.getCode() ) )
    {
    }

    /// @brief Maps the ProtoError value to ErrorCode value.
    /// @param [in] protoCode A ProtoError value to map.
    /// @return Corresponding ErrorCode enum value.
    static ErrorCode::_EnumType protoToErrCode ( const ProtoError::Code & protoCode );
};

/// @brief Returns true if the code passed should be considered a 'success'.
/// All not negative codes are considered 'successes'. The typical success code is 'Success', which is 0.
/// Other positive values also mean that the operation succeeded, but may have special meanings.
/// @param [in] error Error code to check.
/// @return True if the code passed is a success code; False otherwise
inline bool IS_OK ( const ERRCODE & error )
{
    return ( error.value() >= ErrorCode::Success );
}

/// @brief Returns true if the code passed should be considered an 'error'.
/// All negative codes are considered errors. Zero and positive values are not.
/// @param [in] error Error code to check.
/// @return True if the code passed is an error code; False otherwise
inline bool NOT_OK ( const ERRCODE & error )
{
    return ( error.value() < ErrorCode::Success );
}
}
