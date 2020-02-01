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

#include "LogOutput.hpp"

namespace Pravala
{
/// @brief Class for logging text messages to files
class TextLogFileOutput: public TextLogOutput
{
    public:
        /// @brief Constructor.
        /// Creates a new TextLogFileOutput logging data to specified file descriptor
        /// @param [in] fd File descriptor to use
        /// @param [in] autoClose If set to true this file descriptor will be automatically closed
        ///                        when the TextLogFileOutput is destroyed
        TextLogFileOutput ( int fd, bool autoClose = false );

        /// @brief Constructor.
        /// Creates a new TextLogFileOutput logging data to a file specified with its file name.
        /// The file is automatically closed when the TextLogFileOutput is destroyed
        /// @param [in] fileName name of the file to use
        TextLogFileOutput ( const String & fileName );

        /// @brief Destructor.
        /// Closes the file if AutoClose is true.
        ~TextLogFileOutput();

        /// @brief Checks if the file is open.
        /// @return True if the file is currently open (if the file descriptor is valid); False otherwise.
        inline bool isOpen() const
        {
            return ( _myFd >= 0 );
        }

    protected:
        /// @brief Sends log data to the receiver.
        /// @param [in] logMessage The log message object
        /// @param [in,out] strMessage The string with text version of the message. An empty string is passed
        ///                         at the beginning, and as soon as one of the TextLogOutputs
        ///                         generates the string version, it can be reused by subsequent outputs.
        virtual void sendTextLog ( Log::TextMessage & logMessage, String & strMessage );

    private:
        /// @brief Internal file descriptor.
        int _myFd;

        /// @brief If set to true, the file is closed in the destructor.
        bool _autoClose;

        /// @brief No default constructor
        TextLogFileOutput();
};
}
