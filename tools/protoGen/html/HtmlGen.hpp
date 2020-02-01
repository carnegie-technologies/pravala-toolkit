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

#include "../LangGen.hpp"
#include "HtmlFile.hpp"

namespace Pravala
{
/// @brief A class that generates documentation from .proto files
///
/// We pretend HTML is a language you can generate, but it only generates
/// documentation and not an encoder/parser/etc.
///
class HtmlGenerator: public LanguageGenerator
{
    public:
        /// @brief Creates a new language generator
        /// @param [in] proto The protocol specification object, it includes all the data read from
        ///                   the protocol description file (as a tree structure of symbols plus some other settings)
        HtmlGenerator ( ProtocolSpec & proto );

        virtual void run();
        virtual String getHelpText();
        virtual SetOptResult setOption ( char shortName, const String & longName, const String & value );

    protected:
        /// @brief The documentation is always generated into a single file, so we don't have to
        //         worry about cross-file linking (and it's easier to print). This is the only file object used.
        HtmlFile * _singleImplFile;

        HashSet<String> _skipInheritedFields; ///< The set of symbol paths whose inherited fields to skip.

        String _outputFile; ///< The output file path.

        Symbol * _symString; ///< The symbol that represents the 'string' type
        Symbol * _symIpAddr; ///< The symbol that represents the 'IP address' type

        virtual void procRegularSymbol ( Symbol * symbol );
        virtual void procNamespaceSymbol ( Symbol * symbol );

        // @brief Get the name for a symbol, expanding internal symbols
        /// @param [in] symbol The symbol object to get the name of
        String getSymbolName ( Symbol * s );

        // @brief Get the anchor ID for a symbol
        /// @param [in] symbol The symbol object to get the anchor for
        String getSymbolAnchor ( Symbol * s );

        // @brief Get the anchor ID as a link for a symbol
        /// @param [in] symbol The symbol object to get the anchor link for
        String getSymbolAnchorLink ( Symbol * s );

        // @brief Render a namespace
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        void renderNamespace ( Symbol * symbol, HtmlFile & file );

        // @brief Render 'up to contents' link
        /// @param [in] file The implementation file
        void renderUpToContents ( HtmlFile & file );

        // @brief Render a message/struct
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        void renderMessageOrStruct ( Symbol * symbol, HtmlFile & file );

        // @brief Render an enum
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        void renderEnum ( Symbol * symbol, HtmlFile & file );

        // @brief Render the header for a type
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        void renderTypeHeader ( Symbol * symbol, HtmlFile & file );

        // @brief Render the fields for a struct/message
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        void renderStructFields ( Symbol * symbol, HtmlFile & file );
};
}
