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

#include "basic/MemVector.hpp"
#include "basic/SockAddr.hpp"
#include "error/Error.hpp"
#include "log/TextLog.hpp"

namespace Pravala
{
/// @brief Core functionality for different PacketWriter implementations.
class CorePacketWriter: private NoCopy
{
    public:
        /// @brief The type of the writer.
        enum WriterType
        {
            BasicWriter,  ///< Writes using 'write' family of functions.
            SocketWriter  ///< Writes using 'send' family of functions.
        };

        /// @brief Makes the writer perform writes on a separate thread (if supported by the platform).
        static const uint16_t FlagThreaded = ( 1 << 0 );

        /// @brief Makes the writer try to send multiple packets at the same time (if supported by the platform).
        static const uint16_t FlagMultiWrite = ( 1 << 1 );

        /// @brief All core flags.
        static const uint16_t CoreFlags = ( FlagThreaded | FlagMultiWrite );

        const WriterType Type; ///< Configured type of this writer.

        /// @brief Checks if this writer is valid.
        /// @return True if this writer has a valid file descriptor; False otherwise.
        inline bool isValid() const
        {
            return ( _fd >= 0 );
        }

    protected:
        static TextLogLimited _log; ///< Log stream.

        int _fd; ///< File descriptor to write to.

        /// @brief Constructor.
        /// @param [in] wType The type of the writer.
        CorePacketWriter ( WriterType wType );

        /// @brief Destructor.
        /// @note It expects the file descriptor to be invalid!
        ~CorePacketWriter();

        /// @brief Internal function that does the writing.
        /// @param [in] addr The address to send the data to.
        ///                  In basic mode it is always ignored.
        ///                  In socket mode it is only used if it is valid.
        /// @param [in] data The data to write/send.
        /// @param [in] dataSize The size of the data.
        /// @return Standard error code.
        ERRCODE doWrite ( const SockAddr & addr, const char * data, size_t dataSize ) const;
};
}
