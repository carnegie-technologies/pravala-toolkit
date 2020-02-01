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

#include "basic/HashMap.hpp"
#include "basic/HashSet.hpp"
#include "basic/String.hpp"

namespace Pravala
{
class Symbol;

/// @brief An object that holds all the data about the protocol (read from the protocol description file(s))
class ProtocolSpec
{
    public:
        static const char * KW_DEFAULT; ///< Keyword 'default'
        static const char * KW_MIN; ///< Keyword 'min'
        static const char * KW_MAX; ///< Keyword 'max'
        static const char * KW_MIN_LENGTH; ///< Keyword 'min_length'
        static const char * KW_MAX_LENGTH; ///< Keyword 'max_length'
        static const char * KW_MIN_LIST_SIZE; ///< Keyword 'min_list_size'
        static const char * KW_MAX_LIST_SIZE; ///< Keyword 'max_list_size'
        static const char * KW_DEFINED_AS_IN; ///< Keyword 'as_in'
        static const char * KW_IMPORT; ///< Keyword 'import'
        static const char * KW_PRAGMA; ///< Keyword 'pragma'
        static const char * KW_UNIQ; ///< Keyword 'unique'
        static const char * KW_DEFINED; ///< Keyword 'defined'
        static const char * KW_ALIAS; ///< Keyword 'alias'
        static const char * KW_SALIAS; ///< Keyword 'salias'
        static const char * KW_NAMESPACE; ///< Keyword 'namespace'
        static const char * KW_ENUM; ///< Keyword 'enum'
        static const char * KW_MESSAGE; ///< Keyword 'message'
        static const char * KW_STRUCT; ///< Keyword 'struct'
        static const char * KW_TYPEDEF; ///< Keyword 'typedef'
        static const char * KW_PUBLIC; ///< Keyword 'public'
        static const char * KW_PROTECTED; ///< Keyword 'protected'
        static const char * KW_PRIVATE; ///< Keyword 'private'
        static const char * KW_OPTIONAL; ///< Keyword 'optional'
        static const char * KW_REQUIRED; ///< Keyword 'required'
        static const char * KW_REPEATED; ///< Keyword 'repeated'

        /// @brief Selected field ID scope
        enum FieldIdScope
        {
            IdScopeBranch, ///< Branch scope (IDs unique within a single inheritance branch).
            IdScopeTree,   ///< Tree scope (IDs unique within a single inheritance tree).
            IdScopeGlobal  ///< All field IDs have to be unique.
        };

        /// @brief Default constructor
        ProtocolSpec();

        /// @brief Destructor.
        ~ProtocolSpec();

        /// @return The root of this protocol specification
        /// It is an empty namespace symbol.
        inline Symbol * getRoot()
        {
            return _root;
        }

        /// @brief Called by the token file reader with every token read from the protocol description file
        ///
        /// @param [in] token The token read from the file
        /// @param [in] lineComment The protocol-level comment read from the protocol description file
        ///                         for this line (either at the end of it, or just before the line)
        void appendToken ( const String & token, const String & lineComment );

        /// @brief Checks whether the protocol is in 'closed' state
        /// @return True if the protocol is in 'closed' state; false if there still are some
        ///         unfinished symbols.
        bool isClosed() const;

        /// @brief Returns whether the protocol is in 'generate' mode
        /// @return true if the protocol is in 'generate' mode; false otherwise
        bool isGenerateMode() const;

        /// @brief Sets the 'generate' flag
        /// @param [in] generate Whether output should be generated for the symbols being parsed
        void setGenerateMode ( bool generate );

        /// @brief Sets the path to the protocol file being processed.
        /// @param [in] path The path to the protocol file being processed.
        void setProtoFilePath ( const String & path );

        /// @brief Sets the field ID scope.
        /// @param [in] scope The scope; One of: branch, tree, global.
        /// @return True if the scope was accepted; False if it was invalid.
        bool setIdScope ( const String & scope );

        /// @brief Returns the path to the protocol file being processed.
        /// @return the path to the protocol file being processed.
        const String & getProtoFilePath() const;

        /// @brief Performs global-level checks once the protocol is completely read.
        ///
        /// There are no checks at this time.
        ///
        void checkGlobal();

        /// Valid options supported. Only option names in this set
        /// can be used when declaring elements.
        HashSet<String> validOptions;

        /// A set containing all of the keywords reserved by all known languages.
        /// Each language generator adds its own reserved keywords to this set.
        /// It doesn't matter which one is actually used for generating output,
        /// all of them are loaded and can update this set.
        HashSet<String> reservedNames;

        /// This map contains field_code:symbol_path pairs. It is used
        /// when the system is configured to use 'global' level of field
        /// code uniqueness, for making sure that no codes are reused.
        HashMap<uint32_t, String> globalFieldCodes;

        /// @brief Exposes the field ID scope being used.
        /// @return The field ID scope being used.
        inline FieldIdScope getIdScope() const
        {
            return _idScope;
        }

    private:
        /// @brief Whether the protocol is in 'generate' mode.
        /// Symbols created in this mode WILL be written to disk.
        /// This does not affect namespaces.
        bool _generate;

        String _protoFilePath; ///< The path of the proto file being processed.

        Symbol * _root; ///< The root namespace symbol.

        FieldIdScope _idScope; ///< Field ID scope.
};
}
