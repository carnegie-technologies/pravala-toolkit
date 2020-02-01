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
/// @brief Helper class for any object which uses COM.
/// Initializes and uninitalizes COM properly.
class ComOwner
{
    protected:
        /// @brief Constructor.
        /// Initializes COM and stores the result.
        ComOwner();

        /// @brief Destructor.
        /// Deinitializes COM if necessary.
        ~ComOwner();

        /// @brief Check if COM initialized properly or not.
        /// If COM is ready, further COM operations can be performed.
        /// If it isn't, then COM wasn't initializated properly and your code is probably wrong
        /// (i.e. you manually initialized COM with a different threading model).
        /// @return true if COM initialized properly; false otherwise.
        bool isComReady() const;

    private:
        const long _initRet; ///< Return code from the COM initialization.
};
}
