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

#include "BinLog.hpp"
#include "TextLog.hpp"
#include "auto/log/Log/Diagnostic.hpp"

namespace Pravala
{
/// @brief Diagnostics manager, basically just a key-value store that also updates a BinLog stream
class Diagnostics
{
    public:
        /// @brief Gets the static instance of Diagnostics
        /// @return The static instance of Diagnostics
        static Diagnostics & get();

        /// @brief Set a diagnostic for the current time
        /// @param key Key of diagnostic
        /// @param value Text of diagnostic
        void set ( String key, String value );

        /// @brief Remove the diagnostic with the specified key
        /// @param key The key to remove
        void remove ( String key );

        /// @brief Get the map of all diagnostics
        /// @return The map of all diagnostics
        const HashMap<String, Log::Diagnostic> & getDiagnostics() const;

    private:
        static BinLog _diagLog; ///< Diagnostic BinLog stream
        static TextLog _log; ///< Regular, text log stream

        /// @brief Constructor
        /// @note Diagnostics is a singleton
        Diagnostics();

        /// @brief Map of diagnostic key to the diagnostic object
        HashMap<String, Log::Diagnostic> _diagnostics;
};
}
