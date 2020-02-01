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

#include "wifimgr/WifiMgrControl.hpp"

#include "log/TextLog.hpp"

namespace Pravala
{
/// @brief Used for testing WifiMgrControl by sending commands
class WifiMgrCtrlTest
{
    public:
        /// @brief Constructor
        WifiMgrCtrlTest();

        /// @brief Destructor
        ~WifiMgrCtrlTest();

        /// @brief Start the test
        /// @param [in] ctrlName The wifi control info
        void start ( const String & ctrlName );

    private:
        static TextLog _log; ///< Log
};
}
