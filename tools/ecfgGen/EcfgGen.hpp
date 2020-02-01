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
#include "basic/HashSet.hpp"
#include "config/ConfigString.hpp"
#include "config/ConfigStringMultiSetList.hpp"

namespace Pravala
{
class EcfgGenerator
{
    public:
        static ConfigString optProductName; ///< Configures the name of the product
        static ConfigString optProjectDir; ///< Configures the path to the root directory of the project
        static ConfigString optInputFile; ///< Configures the path to the ECFG file to use
        static ConfigString optOutputFile; ///< Configures the path to the output file
        static ConfigString optUsername; ///< Configures build user
        static ConfigString optHostname; ///< Configures build host
        static ConfigString optBuildType; ///< Configures build type
        static ConfigString optRevSuffix; ///< Configures revision suffix
        static ConfigString optVersion; ///< Configures version number
        static ConfigString optVersionFile; ///< Configures the file to read the version from
        static ConfigString optVersionJson; ///< Configures the JSON file and field to read the version from
        static ConfigStringMultiSetList optTags; ///< Configures build tags

        /// @brief Default Constructor
        EcfgGenerator();

        /// @brief Runs the EcfgGenerator
        /// @return True on success, false on error
        bool run();

    private:
        /// @brief Regular options (key: value_list)
        HashMap<String, StringList> _options;

        /// @brief Automatic options  (key: value).
        /// These are built-related and cannot be overloaded using ecfg files.
        HashMap<String, String> _automaticOptions;

        /// @brief Tags to use
        HashSet<String> _tags;

        /// @brief Reads the ECFG file
        /// @return True on success; False on error
        bool readEcfgFile();

        /// @brief Adds build info to ECFG
        ///
        /// At the moment it adds product name, and some data from GIT.
        /// @return True on success; False on error
        bool addBuildInfo();
};
}
