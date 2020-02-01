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

namespace DBusC
{
extern "C" {
#include <dbus/dbus.h>
}
}

namespace Pravala
{
/// @brief A helper class that wraps a DBusError object.
/// It performs initialization and cleanup.
class DBusErrorWrapper: public DBusC::DBusError
{
    public:
        /// @brief Default constructor.
        /// It initializes the underlying D-Bus error structure.
        DBusErrorWrapper();

        /// @brief Destructor.
        /// @brief Clears the underlying D-Bus error structure.
        ~DBusErrorWrapper();

        /// @brief Clears the underlying D-Bus error structure.
        void clear();
};
}
