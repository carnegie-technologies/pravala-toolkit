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

#include "LangGen.hpp"

namespace Pravala
{
/// @brief A 'null' language generator.
///
/// This is a special generator that doesn't do anything.
/// It can be used to have the compiler parse and process the protocol description
/// (and show any errors) without actually generating anything.
/// It overloads the run() function, so it doesn't require any arguments,
/// or a directory path to run. It doesn't do anything and doesn't throw any errors.
///
class NullGenerator: public LanguageGenerator
{
    public:
        /// @brief Creates a new language generator
        /// @param [in] proto The protocol specification object, it includes all the data read from
        ///                   the protocol description file (as a tree structure of symbols plus some other settings)
        NullGenerator ( ProtocolSpec & proto );

        virtual void run();
        virtual String getHelpText();
        virtual SetOptResult setOption ( char shortName, const String & longName, const String & value );

    protected:
        /// @brief Called for each regular type
        ///
        /// 'Regular' types are all messages (including base messages) and enumerators.
        /// This is not called for primitive types and namespaces.
        ///
        /// Since the 'null' language generator does nothing, this function just returns.
        ///
        /// @param [in] symbol The symbol to process
        virtual void procRegularSymbol ( Symbol * symbol );
};
}
