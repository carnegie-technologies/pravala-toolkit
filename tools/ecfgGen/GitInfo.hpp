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
#include "error/Error.hpp"

namespace Pravala
{
/// @brief Class used for extracting GIT data from the GIT repository
class GitInfo
{
    public:
        /// @brief Reads GIT's HEAD revision
        /// @param [in] projectRoot The root directory of the project
        /// @param [in] shortRev Whether the short revision should be read (a full rev otherwise)
        /// @param [out] output The revision information, or empty string on error
        /// @return Standard error code
        static ERRCODE readGitRev (
            const String & projectRoot, bool shortRev,
            String & output );

        /// @brief Reads GIT's branch
        /// @param [in] projectRoot The root directory of the project
        /// @param [out] output The branch name, or empty string on error
        /// @return Standard error code
        static ERRCODE readGitBranch (
            const String & projectRoot,
            String & output );

        /// @brief Reads GIT's root path
        /// @param [in] startingDir The directory from which to start looking
        /// @return The path to GIT's root directory (including '/' at the end), or empty on error
        static String getGitDirectory ( const String & startingDir );
};
}
