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

namespace Pravala
{
struct ProtoError
{
    /// @brief Different protocol errors.
    /// 0 is a success; Errors should use <0 values; >0 values are non-critical warnings.
    enum Code
    {
        /// @brief Operation was successful (no error).
        Success = 0,

        /// @brief Protocol "warning". It means that the encoding is correct, but the value cannot be used.
        ///        This status code will be used if there were any fields whose IDs are not known,
        ///        or when some of the enum codes received cannot be used by the specific enums
        ///        (and the enum was set to its default value). If it is returned by the message's deserializer,
        ///        it means that the message validates properly despite the problems.
        ProtocolWarning = 1,

        InvalidParameter = -1,     ///< Invalid operation argument.
        InternalError = -2,     ///< There has been some internal error (this should NOT happen).
        MemoryError = -3,     ///< Memory could not be allocated.
        TooMuchData = -4,     ///< Too much data to fit in the channel/storage/etc.
        IncompleteData = -5,     ///< Data is incomplete, need more data to continue.
        ProtocolError = -6,     ///< Protocol error - data received does not make sense.
        InvalidDataSize = -7,     ///< Received data size has incorrect (different than expected) size.
        TooBigValue = -8,     ///< Value cannot be processed because it is too big.
        RequiredFieldNotSet = -9,     ///< The data field specified as 'required' by the protocol has not been set.

        /// @brief The value defined by the protocol to have certain value has different value.
        DefinedValueMismatch = -10,

        FieldValueOutOfRange = -11,     ///< The value is out of allowed (by the protocol) range.
        StringLengthOutOfRange = -12,     ///< The string length is out of allowed (by the protocol) range.
        ListSizeOutOfRange = -13,     ///< The list size is out of allowed (by the protocol) range.

        Unsupported = -14, ///< Operation is not supported.

        Unknown = -99     ///< Default, unknown error code.
    };

    /// @brief Default constructor.
    /// Sets this error code to 'Unknown'
    inline ProtoError(): _code ( ProtoError::Unknown )
    {
    }

    /// @brief Copy constructor.
    /// @param [in] code The code to copy.
    inline ProtoError ( const Code & code ): _code ( code )
    {
    }

    /// @brief 'equal' operator
    /// @param [in] code The enum value to compare this object to
    /// @return True if this object contains matching enum value, false otherwise
    inline bool operator== ( Code code ) const
    {
        return ( _code == code );
    }

    /// @brief 'not equal' operator
    /// @param [in] code The enum value to compare this object to
    /// @return False if this object contains matching enum value, true otherwise
    inline bool operator!= ( Code code ) const
    {
        return ( _code != code );
    }

    /// @brief Exposes the internal error code value.
    /// @return internal error code value.
    inline Code getCode() const
    {
        return _code;
    }

    /// @brief Returns the name of the error code.
    /// @return The name of this error.
    const char * toString() const;

    private:
        Code _code; ///< The internal error code.
};

/// @brief Returns true if the code passed should be considered a 'success'.
/// All not negative codes are considered 'successes'. The typical success code is 'Success', which is 0.
/// Other positive values also mean that the operation succeeded, but may have special meanings.
/// @param [in] error Error code to check.
/// @return True if the code passed is a success code; False otherwise
inline bool IS_OK ( const ProtoError & error )
{
    return ( error.getCode() >= ProtoError::Success );
}

/// @brief Returns true if the code passed should be considered an 'error'.
/// All negative codes are considered errors. Zero and positive values are not.
/// @param [in] error Error code to check.
/// @return True if the code passed is an error code; False otherwise
inline bool NOT_OK ( const ProtoError & error )
{
    return ( error.getCode() < ProtoError::Success );
}
}
