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

#include "../PosixTypes.hpp"

namespace Pravala
{
namespace AfRouteTypes
{
typedef PosixNetMgrTypes::Address Address;
typedef PosixNetMgrTypes::Route Route;

/// @brief Contains AF_ROUTE-specific data about a link.
struct Link: public PosixNetMgrTypes::Link
{
    uint64_t rxBytes; ///< The number of bytes received over the interface.
    uint64_t txBytes; ///< The number of bytes sent over the interface.

    /// @brief Default constructor.
    Link(): rxBytes ( 0 ), txBytes ( 0 )
    {
    }
};
}
}
