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

#include "basic/String.hpp"

namespace Pravala
{
/// @brief To be inherited by things that provide their ID for logging purposes to other objects.
class LogId
{
    public:
        /// @brief Returns the description of the object for logging.
        /// @param [in] extended If true, it should return "extended" (potentially longer) description.
        ///                      What it means depends on the object.
        /// @return The description of the object for logging.
        virtual String getLogId ( bool extended = false ) const = 0;

    protected:
        /// @brief Destructor.
        virtual ~LogId()
        {
        }
};
}
