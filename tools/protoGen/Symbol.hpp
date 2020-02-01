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
#include "ProtoSpec.hpp"
#include "Element.hpp"

namespace Pravala
{
class FileObject;

/// @brief A class representing a single protocol symbol (type)
///
/// In the protocol description file, there are two types of objects defined - symbols and elements.
///
/// They are somewhat similar to types and variables.
/// Each symbol represents a separate type, and each element represents a variable (field) - with some exceptions.
///
/// There are 4 basic types of symbols. The simplest one is a namespace symbol.
/// It is basically a container for other symbols. It cannot contain any elements, only other symbols.
/// Each symbol inside a namespace symbol can be either a regular symbol, or another namespace.
/// Namespace symbols can only be contained inside other namespaces.
/// Namespace symbols are also special in the way they are declared. It is not possible to declare the same symbol
/// twice, but when that happens to regular symbols an error is thrown. Namespaces, however, can be used in several
/// different files, and so "declaring" a namespace that already exists means that the existing symbol is
/// "reopened", and more symbols can be added inside.
///
/// The second type of the symbol is a 'basic symbol'. Those symbols represent primitive types, that are
/// not defined by the protocol, but rather provided externally - like basic numbers, strings, but also
/// IP addresses or some other types. It is possible for language generators to define their own, specific
/// primitive types by generating appropriate symbols.
///
/// Another type of a symbol is an 'enumerator' symbol. This kind of symbol represents a numeric value
/// that can story only a specific list of values. Each enumerator symbol can store several elements, each of
/// the elements has a value and represents a single 'enumerator code'. Enumerators can't inherit other
/// symbols or be inherited.
///
/// The most important types of symbols are 'message' and 'struct' symbols. Both messages and structures
/// can contain other message symbols, as well as elements. They cannot include namespaces.
/// Each element being a part of a struct/message is like a field inside a class.
///
/// The difference is that structures can only inherit/be inherited by other structures,
/// and messages are only compatible with other messages. Also, any message that does not inherit any other
/// message is considered a 'base message'. It is supposed to add additional logic to be able to be sent
/// over network, and allow other messages to be deserialized from it.
///
/// Typically the language generator adds more code to the base message, like a special 'length' field
/// that is required to interpret the whole message as whole, without worrying about the actual content.
///
class Symbol
{
    public:
        /// @brief Used to mark basic (primitive) types that have a special role
        enum SpecBasicType
        {
            SpecTypeDefault = 0, ///< No special role
            SpecTypeString = 1, ///< This symbol is a string symbol
            SpecTypeFloatingPoint = 2 ///< This symbol is a floating point number
        };

        /// @return True if the symbol is finished (its last '}' was found), false otherwise
        inline bool isFinished() const
        {
            return _isFinished;
        }

        /// @return True if this symbol is a root namespace symbol, false otherwise
        inline bool isRoot() const
        {
            return ( !_parent );
        }

        /// @return True if this symbol is a basic, primitive type, false otherwise
        inline bool isBasic() const
        {
            return ( _symType == SymBasic );
        }

        /// @return True if this symbol is a struct, false otherwise
        inline bool isStruct() const
        {
            return ( _symType == SymStruct );
        }

        /// @return True if this symbol is a message (regular or base), false otherwise
        inline bool isMessage() const
        {
            return ( _symType == SymMessage );
        }

        /// @return True if this symbol is a message (regular or base) or a struct, false otherwise
        inline bool isMessageOrStruct() const
        {
            return ( _symType == SymMessage || _symType == SymStruct );
        }

        /// @return True if this symbol is a base message, false otherwise
        inline bool isBaseMessage() const
        {
            return ( _symType == SymMessage && !_inheritance );
        }

        /// @return True if this symbol is a typedef, false otherwise
        inline bool isTypedef() const
        {
            return ( _symType == SymTypedef );
        }

        /// @return True if this symbol is a namespace, false otherwise
        inline bool isNamespace() const
        {
            return ( _symType == SymNamespace );
        }

        /// @return True if this symbol is an enumerator, false otherwise
        inline bool isEnum() const
        {
            return ( _symType == SymEnum );
        }

        /// @return True if this symbol is a primitive numeric type (integer), false otherwise.
        inline bool isInteger() const
        {
            return ( _bitLength > 0 );
        }

        /// @return True if this symbol is a float-point number, false otherwise.
        inline bool isFloatingPoint() const
        {
            return ( _specType == SpecTypeFloatingPoint );
        }

        /// @return True if output for this symbol should be generated, false otherwise
        inline bool isGenerated() const
        {
            return _generated;
        }

        /// @return The base name of the proto file that defined this symbol.
        ///         For a/b/c/foo_bar.proto it is 'foo_bar'.
        const String getProtoFileBaseName() const;

        /// @return The special role of this symbol.
        inline SpecBasicType getSpecBasicType() const
        {
            return _specType;
        }

        /// @return True if this symbol can be used as a storage type for aliases, false otherwise.
        ///         For the type (symbol) to be used as a storage type, it has to be an unsigned numeric type
        inline bool canBeAliased() const
        {
            return ( !_canBeNegative && _bitLength > 0 );
        }

        /// @return True if this symbol is a signed numeric type, false otherwise
        inline bool canBeNegative() const
        {
            return _canBeNegative;
        }

        /// @return The number of bits that can "fit" in this type. For example for the uin32 it is 32.
        inline int getBitLength() const
        {
            return _bitLength;
        }

        /// @return The name of the symbol
        inline const String & getName() const
        {
            return _name;
        }

        /// @return The path of this symbol, which consists of all the namespace and outer symbols that this symbol
        /// is declared inside, followed by the name of this symbol. It uses '.' as a name separator.
        inline const String & getPath() const
        {
            return _path;
        }

        /// @return A comment associated with this symbol (if any)
        inline const String & getComment() const
        {
            return _comment;
        }

        /// @return The parent symbol. The symbol inside of which this symbol is defined. Usually a namespace,
        ///    but could also be another message symbol. By following 'parent' symbols one would eventually
        ///    reach the "root namespace" symbol which has no parent.
        inline Symbol * getParent()
        {
            return _parent;
        }

        /// @return The most external parent message/struct symbol
        ///          It could be zero, if we have no parent, or our parent is a namespace
        Symbol * getOldestMessageOrStructParent();

        /// @return The symbol inherited by this symbol. If the symbol doesn't inherit anything it is 0.
        inline Symbol * getInheritance()
        {
            return _inheritance;
        }

        /// @return The oldest ancestor of this symbol. If the symbol doesn't inherit anything it is this symbol itself.
        Symbol * getBaseInheritance();

        /// @return The default enum element to be used by this Enum type (valid only for Enum symbols)
        Element * getEnumDefault();

        /// @return All the symbols declared inside this symbol
        inline const HashMap<String, Symbol *> & getInternalSymbols() const
        {
            return _internalSymbols;
        }

        /// @return The names of all the symbols declared inside this symbol - in order they were declared
        inline const StringList & getOrdInternalSymbols() const
        {
            return _ordIntSymbols;
        }

        /// @return All the elements contained in this symbol
        inline const HashMap<String, Element *> & getElements() const
        {
            return _elements;
        }

        /// @return The names of all the elements contained in this symbol - in order they were declared
        inline const StringList & getOrdElements() const
        {
            return _ordElements;
        }

        /// @return The mapping with the names and elements defined by this symbol (message)
        inline const HashMap<String, Element *> & getDefines() const
        {
            return _defines;
        }

        /// @brief Creates a new primitive type (at the root level)
        ///
        /// Can be used by language generators to add custom primitive types.
        ///
        /// If the symbol with the given name already exists it is returned.
        ///
        /// @param [in] name The name of the type (symbol) (which, since it is added at the root level,
        ///                   will also be its path)
        /// @param [in] specType Special role of this symbol (if any)
        /// @return The generated symbol
        Symbol * createBasicRootType (
            const String & name,
            SpecBasicType specType = SpecTypeDefault );

        /// @brief Creates a new numeric primitive type (at the root level)
        ///
        /// Can be used by language generators to add custom numeric primitive types.
        ///
        /// If the symbol with the given name already exists it is returned.
        ///
        /// @param [in] name The name of the type (symbol) (which, since it is added at the root level,
        ///                   will also be its path)
        /// @param [in] bitLength The number of bits this type can hold
        /// @param [in] canBeNegative Whether this type can store negative values or not (for aliasing purposes)
        /// @param [in] specType Special role of this symbol (if any)
        /// @return The generated symbol
        Symbol * createBasicRootType (
            const String & name,
            int bitLength,
            bool canBeNegative,
            SpecBasicType specType = SpecTypeDefault );

        /// @brief Parses given enum's code.
        /// It will check if the code uses extended syntax (with " " around the value),
        /// and if so, it will generate a simplified name and extended name.
        /// @param [in] code The enum's code.
        /// @param [out] name Sanitized name that should be used as the code's main name. Always set.
        /// @param [out] extName The original name (w/o " ") that should be used whenever a string is needed
        ///                      (when generating enum's description, or serializing to JSON).
        ///                      Only generated if extended syntax was used.
        static void parseEnumCode ( const String & code, String & name, String & extName );

    private:
        /// The types of the symbols
        enum SymType
        {
            SymUnknown, ///< Unknown type
            SymBasic, ///< Basic (primitive) type
            SymNamespace, ///< Namespace
            SymMessage, ///< Message
            SymStruct, ///< Struct
            SymEnum, ///< Enumerator
            SymTypedef ///< Typedef symbol
        };

        ProtocolSpec & _proto; ///< The protocol spec object (a context at which this symbol exists)

        /// @brief A pointer to the processing function that should be called upon receiving the next token
        ///
        /// Each symbol, while parsing, behaves like a state machine. Each of the processing functions
        /// is a potentional state change. So this variable stores the current state of this particular symbol.
        bool (* _procFunc) ( Symbol &, const String & );

        Symbol * _parent; ///< Parent of this symbol, which is the symbol this symbol is declared inside of.

        /// @brief Points to the symbol inherited by this symbol.
        /// If this is a 'typedef' symbol, the 'inherited' symbol is the one that should be used instead.
        Symbol * _inheritance;

        bool _canBeNegative; ///< True for numeric primitive types that can store negative values
        int _bitLength; ///< The size (in bits) of a numeric primitive type
        bool _isFinished; ///< True if the last '{' of this symbol has already been read (the symbol is "closed")
        SymType _symType; ///< The type of the symbol
        SpecBasicType _specType; ///< The special role of the primitive type
        bool _generated; ///< Whether output for this symbol should be generated or not

        const String _protoFilePath; ///< The path to the protocol file that defined this symbol.

        String _name; ///< The name of this symbol
        String _path; ///< The path of this symbol. Includes all outer symbols and namespaces as well as the name.
        String _comment; ///< A comment associated with this symbol

        HashMap<String, Symbol *> _internalSymbols; ///< Maps name:symbol of all symbols declared inside of this symbol
        HashMap<String, Symbol *> _typedefSymbols; ///< Maps name:symbol of all symbols typedefed inside of this symbol
        StringList _ordIntSymbols; ///< The list of names of internal symbols (in order in which they were declared)
        HashMap<String, Element *> _elements; ///< Maps name:element of all elements contained in this symbol
        StringList _ordElements; ///< The list of names of this symbol's elements in order they were declared in

        /// @brief The map element_name:element of all elements defined by this (message) symbol
        HashMap<String, Element *> _defines;

        /// @brief Contains all the field codes used in the inheritance tree that starts with this symbol
        ///
        /// When 'inheritance tree' scope is used for detecting field conflicts, we store all the codes
        /// used in any message that inherits this symbol directly or indirectly, to detect collisions.
        ///
        /// It contains field_code:symbol_in_which_the_code_was_used_path pairs.
        HashMap<uint32_t, String> _treeRootFieldCodes;

        // "Current" state:

        String _curComment; ///< The current comment associated with the token being processed

        /// @brief The internal symbol being processed
        ///
        /// This is set whenever we declare a symbol inside this symbol. Until that internal
        /// symbol is closed, all tokens will be passed to it. When this variable is set
        /// when we should be passing those tokens to an internal symbol. It is set to 0
        /// once the internal symbol is closed (and tokens are processed again by this symbol)
        Symbol * _curIntSymbol;

        Element * _curElement; ///< Currently processed element

        /// @brief Constructs a root node
        /// @param [in] protoSpec The protocol spec object - the context in which the symbol tree should work
        Symbol ( ProtocolSpec & protoSpec );

        /// @brief Constructs a symbol
        ///
        /// If a symbol is generated in 'generate' mode and its parent is a namespace,
        /// it will mark all parent namespaces as 'generated'.
        ///
        /// @param [in] parent The symbol inside of which the symbol should be created
        /// @param [in] symType The type of the symbol
        /// @param [in] name The name of the symbol
        /// @param [in] path The path of the symbol
        /// @param [in] comment Current comment for this symbol
        /// @param [in] specType The special type of the primitive symbol
        Symbol (
            Symbol & parent, SymType symType, const String & name,
            const String & path, const String & comment,
            SpecBasicType specType = SpecTypeDefault );

        /// @brief Destructor.
        ~Symbol();

        /// @brief Reopens a namespace symbol
        ///
        /// Each symbol can be declared only once. It is opened, modified, and closed.
        /// Trying to declare a symbol that already exists results in error.
        ///
        /// The only exception are namespace symbols. Once we "declare" a namespace that already
        /// exists it is actually "reopened" using this function.
        void reopenNamespace();

        /// @brief Appends next token to this symbol
        /// @param [in] token The token to append
        /// @param [in] lineComment The comment associated with this token
        ///              (around the line in which this token was found)
        /// @return True if adding the token resulted in completion of this symbol. This symbol
        ///  will not accept any new tokens (unless it is a namespace symbol, but it still has to be
        ///  reopened first)
        bool appendToken ( const String & token, const String & lineComment );

        /// @brief Performs some extra checks once the symbol is completed to make sure it's ok.
        ///
        /// An error is thrown when there is something wrong.
        void checkCompleteSymbol();

        /// @brief Converts a string to a symbol type
        /// @param [in] sType The string with description of the type
        /// @return The enum value for that type (or "unknown")
        static SymType getSymType ( const String & sType );

        /// @brief Converts a string to an element rule
        /// @param [in] sRule The string with description of the rule
        /// @return The enum value for that rule (or "unknown")
        static Element::ElemRule getElemRule ( const String & sRule );

        /// @brief Converts a string to an element access mode
        /// @param [in] sRule The string with description of the access mode
        /// @return The enum value for that acces mode (or "unknown")
        static Element::ElemAccess getElemAccess ( const String & sAccess );

        /// @brief A helper functions for processing expected tokens
        ///
        /// Sometimes there is only one possible and legal token. In those cases
        /// this function is used. It compares the token with the expected value.
        /// If it is the same, that the nextFunc function is set as the next processing function
        /// (so the state is modified). If the token is different, an error is thrown.
        ///
        /// @param [in] token The token to examine
        /// @param [in] expToken The token that is expected
        /// @param [in] nextFunc The next token processor to set if the token is correct
        /// @return Always false. The symbol is never closed when this function is used
        bool procExpSymbol (
            const String & token, const char * expToken,
            bool ( * nextFunc )( Symbol &, const String & ) );

        /// @brief A function called when current 'defined' element is completed
        /// It checks if everything is OK with the 'defined' element and throws an error when it's not
        void checkCurDefined();

        /// @brief A function called when current 'alias' element is completed
        /// It checks if everything is OK with the 'alias' element and throws an error when it's not
        void checkCurAlias();

        /// @brief A function called when current basic element is completed
        /// It checks if everything is OK with the element and throws an error when it's not
        void checkCurBasic();

        /// @brief Checks whether the provided string is a legal name
        ///
        /// It checks whether it contains illegal characters.
        /// If the name is used as a typename (message names, but also enum codes), then it is also
        /// compared against the list of reserved keywords.
        /// If the name is not correct an error is thrown.
        ///
        /// @param [in] pSpec The protocol spec object to use
        /// @param [in] name The name to check
        /// @param [in] isTypeName If true, the name will be checked against the list of reserved keywords as well.
        static void checkName ( const ProtocolSpec & pSpec, const String & name, bool isTypeName );

        /// @brief Creates a new internal symbol
        ///
        /// The name of the new symbol is checked (if we already contain a symbol with that
        /// name an error is thrown), the new symbol is added to the list of internal symbols
        /// (and the ordered list), and set as the _curIntSymbol.
        ///
        /// @param [in] name The name of the new symbol
        /// @param [in] symType The type of the symbol to create
        void createNewIntSymbol ( const String & name, SymType symType );

        /// @brief Returns a symbol reachable from the given symbol by following the provided path
        ///
        /// It will try to find a path of symbols to follow to "match" a string path given.
        /// The string path contains names of symbols, the returned symbol is the last one in the path,
        /// or 0 if the path does not match. The search starts from the given symbol
        /// and the first element of the path should be among this symbol's internal symbols
        /// (for a match to be possible)
        ///
        /// @param [in] fromSymbol The symbol from which the search should be started
        /// @param [in] path The list of names that form a path that we're looking for
        /// @return Symbol found using the path provided, or 0 if it could not be found.
        Symbol * tryPath ( Symbol * fromSymbol, const StringList & path );

        /// @brief Finds symbol with a given path/name
        ///
        /// It looks for a symbol using its name/path (A.B.C.D) (starting from this symbol)
        ///
        /// @param [in] path The name/path to the symbol we're looking for
        /// @return The symbol if found, 0 otherwise
        Symbol * findSymbol ( const String & path );

        /// @brief Finds usable symbol with a given path/name
        ///
        /// It is similar to findSymbol function, but it makes sure that the symbol
        /// exists and is  completely constructed (closed/finished).
        /// If it doesn't exist or is not closed an error is thrown.
        /// @param [in] path The name/path to the symbol we're looking for
        /// @return The symbol found
        Symbol * findUsableSymbol ( const String & path );

        /// @brief Processes the token given inside currently processed internal symbol
        ///
        /// The internal symbol has to be set.
        ///
        /// It is a "fall-through" function. Calls 'appendToken' in the current sub-symbol
        /// of this symbol. If the processing function returns True (which means
        /// "end of symbol"), the _curIntSymbol is set to 0 and the next state function is set to:
        ///
        /// Possible next state function(s):
        ///  procModifierOrTypeOrSymbolEnd()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        ///           Since this particular function always operate when internal symbol exists,
        ///           it never closes the current symbol (so it always returns false)
        static bool procInternalSymbol ( Symbol & self, const String & token );

        /// @brief Processes the token as a name of the internal 'struct' symbol
        ///
        /// Possible next state function(s):
        ///  procInternalSymbol()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procIntStructName ( Symbol & self, const String & token );

        /// @brief Processes the token as a name of the internal 'message' symbol
        ///
        /// Possible next state function(s):
        ///  procInternalSymbol()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procIntMessageName ( Symbol & self, const String & token );

        /// @brief Processes the token as a name of the internal 'typedef' symbol
        ///
        /// Possible next state function(s):
        ///  procInternalSymbol()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procIntTypedefName ( Symbol & self, const String & token );

        /// @brief Processes the token as a name of the internal 'namespace' symbol
        ///
        /// Possible next state function(s):
        ///  procInternalSymbol()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procIntNamespaceName ( Symbol & self, const String & token );

        /// @brief Processes the token as a name of the internal 'enum' symbol
        ///
        /// Possible next state function(s):
        ///  procInternalSymbol()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procIntEnumName ( Symbol & self, const String & token );

        /// @brief Expects the opening '(' for symbol's inheritance
        ///
        /// This is only used by the 'typedef' symbols!
        /// Possible next state function(s):
        ///  procInheritName()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procSymInheritBeg ( Symbol & self, const String & token );

        /// @brief Expects the opening '{' of the symbol, or '(' for symbol's inheritance
        ///
        /// Possible next state function(s):
        ///  procModifierOrTypeOrSymbolEnd()
        ///  procInheritName()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procSymOpenBracketOrInheritBeg ( Symbol & self, const String & token );

        /// @brief Processes the token as a name of inherited symbol
        ///
        /// Possible next state function(s):
        ///  procInheritEnd()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procInheritName ( Symbol & self, const String & token );

        /// @brief Expects the ')' token (as the end of symbol's inheritance string)
        ///
        /// Possible next state function(s):
        ///  procSymOpenBracket()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procInheritEnd ( Symbol & self, const String & token );

        /// @brief Expects the opening '{' of the symbol
        ///
        /// Possible next state function(s):
        ///  procModifierOrTypeOrSymbolEnd()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procSymOpenBracket ( Symbol & self, const String & token );

        /// @brief Expects a modifier (a rule, an acces mode), or element type, or '}' (as the end of the symbol)
        ///
        /// Possible next state function(s):
        ///  0 (if the symbol has just been closed)
        ///
        ///  procIntStructName()
        ///  procIntMessageName()
        ///  procIntNamespaceName()
        ///  procIntEnumName()
        ///
        /// Also, by calling procElementName() from this function:
        ///  procEnumEq()
        ///  procElementCol()
        ///
        /// Also, by calling procModifierOrElementType() from this function:
        ///  procModifierOrElementType()
        ///  procDefinedName()
        ///  procElementName()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procModifierOrTypeOrSymbolEnd ( Symbol & self, const String & token );

        /// @brief Expects a ';' as the end of the 'typedef' symbol
        ///
        /// Possible next state function(s):
        ///  0 (if the symbol has just been closed)
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procTypedefEnd ( Symbol & self, const String & token );

        /// @brief Expects a modifier (a rule, an access mode), or element type
        ///
        /// Possible next state function(s):
        ///  procModifierOrElementType()
        ///  procDefinedName()
        ///  procElementName()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procModifierOrElementType ( Symbol & self, const String & token );

        /// @brief Processes the token as a name of the element
        ///
        /// Possible next state function(s):
        ///  procElementCol()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procElementName ( Symbol & self, const String & token );

        /// @brief Processes the token as a name of enum's code
        ///
        /// Possible next state function(s):
        ///  procEnumEq()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procEnumCodeName ( Symbol & self, const String & token );

        /// @brief Processes the token as a name of the 'defined' element (the target to be defined)
        ///
        /// Possible next state function(s):
        ///  procDefinedEqOrAsIn()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procDefinedName ( Symbol & self, const String & token );

        /// @brief Expects a '=' character (in 'enum_code = code_value;' expression)
        ///
        /// Possible next state function(s):
        ///  procEnumValue()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procEnumEq ( Symbol & self, const String & token );

        /// @brief Expects a ':' character
        ///
        /// It expects a ':' character from 'element_name : field_code;' expression,
        /// or 'alias_name : alias_target [ range_from = range_to ];' expression
        ///
        /// Possible next state function(s):
        ///  procAliasTarget()
        ///  procElementCode()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procElementCol ( Symbol & self, const String & token );

        /// @brief Expects a '=' or 'as_in' token (that follows the name of 'defined' element)
        ///
        /// Possible next state function(s):
        ///  procDefinedValue()
        ///  procDefinedAsInTarget()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procDefinedEqOrAsIn ( Symbol & self, const String & token );

        /// @brief Treats the token as enum's code value
        ///
        /// It is used after '-' token, and modifies the next (so current) token to include that '-'.
        /// This is because we treat '-' sign as a separate token, and not as a part
        /// of the value that follows.
        ///
        /// It prepends the token with '-' and passes it to setEnumValue()
        ///
        /// Possible next state function(s) - by calling setEnumValue():
        ///  procElementSemiColOrOptBeg()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procEnumNegValue ( Symbol & self, const String & token );

        /// @brief Expects a '-' token, or enum's code value
        ///
        /// Possible next state function(s):
        ///  procEnumNegValue()
        ///
        /// Also, by calling setEnumValue():
        ///  procElementSemiColOrOptBeg()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procEnumValue ( Symbol & self, const String & token );

        /// @brief It treats the token as enum code's value
        ///
        /// It checks the validity of the data.
        ///
        /// Possible next state function(s):
        ///  procElementSemiColOrOptBeg()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool setEnumValue ( Symbol & self, const String & token );

        /// @brief Processes the token as element's code
        ///
        /// Possible next state function(s):
        ///  procElementSemiColOrOptBeg()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procElementCode ( Symbol & self, const String & token );

        /// @brief Processes the name of the storage target of the 'alias' element
        ///
        /// Possible next state function(s):
        ///  procAliasRangeBeg()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procAliasTarget ( Symbol & self, const String & token );

        /// @brief Expects a '[' token (in expression 'alias abc : storage [ from - to ];')
        ///
        /// Possible next state function(s):
        ///  procAliasRangeFrom()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procAliasRangeBeg ( Symbol & self, const String & token );

        /// @brief Processes the token as the beginning of the alias' bit range
        ///
        /// Possible next state function(s):
        ///  procAliasRangeEndOrDash()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procAliasRangeFrom ( Symbol & self, const String & token );

        /// @brief Expects a ']' or '-' character
        ///
        /// It is looking for ']' in 'alias abc : storage [ single_bit ];' expression,
        /// or for '-' in 'alias abc : storage [ from - to ];' experession
        ///
        /// Possible next state function(s):
        ///  procAliasRangeTo()
        ///
        /// Also, by calling procAliasRangeEnd():
        ///  procElementSemiColOrOptBeg()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procAliasRangeEndOrDash ( Symbol & self, const String & token );

        /// @brief Processes the token as the end of alias' bit range
        ///
        /// Possible next state function(s):
        ///  procAliasRangeEnd()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procAliasRangeTo ( Symbol & self, const String & token );

        /// @brief Expects a ']' token (in expression 'alias abc : storage [ from - to ];')
        ///
        /// Possible next state function(s):
        ///  procElementSemiColOrOptBeg()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procAliasRangeEnd ( Symbol & self, const String & token );

        /// @brief Treats the token as the value of 'defined' target
        ///
        /// It is used after '-' token, and modifies the next (so current) token to include that '-'.
        /// This is because we treat '-' sign as a separate token, and not as a part
        /// of the value that follows.
        ///
        /// It prepends the token with '-' and passes it to setDefinedValue()
        ///
        /// Possible next state function(s) - by calling setDefinedValue():
        ///  procElementSemiCol()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procDefinedNegValue ( Symbol & self, const String & token );

        /// @brief Expects a '-' token, or defined target's value
        ///
        /// Possible next state function(s):
        ///  procDefinedNegValue()
        ///
        /// Also, by calling setDefinedValue():
        ///  procElementSemiCol()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procDefinedValue ( Symbol & self, const String & token );

        /// @brief Checks whether the defined value can be accepted by the target
        ///
        /// It checks validity of the data (range), but also checks if that value has
        /// already been used (if 'unique' rule is used for the target element)
        ///
        /// @param [in] symbolName The name of the symbol that 'defines' this value
        /// @param [in] element The 'defined' target, that needs to be able to accept the value used
        /// @param [in] defValue The value to be checked
        static void checkSetDefinedValue (
            const String & symbolName,
            Element * element, const String & defValue );

        /// @brief It treats the token as the value that the target is defined to have
        ///
        /// It checks the validity of the data using checkSetDefinedValue()
        ///
        /// Possible next state function(s):
        ///  procElementSemiCol()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool setDefinedValue ( Symbol & self, const String & token );

        /// @brief Processes the token as a name of the symbol to be used for 'defined as_in' expression
        ///
        /// This symbol will define the 'defined' target to the same value the symbol
        /// specified by this token does. This is a way to go around the 'unique' restriction
        /// on targets, and still be able to have two (or more) different symbols defining it to the same value.
        /// The 'unique' symbol is supposed to protect from accidentally reusing the same value
        /// between different symbols. However, we want to allow this to be possible, it just
        /// needs to be explicitly specified (using 'as_in' syntax). Also, the actual value is provided
        /// only once, in the symbol that originally defined the target, here we don't specify that
        /// value anymore, we just say it's 'the same' as the value used by the other symbol.
        ///
        /// Possible next state function(s):
        ///  procElementSemiCol()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procDefinedAsInTarget ( Symbol & self, const String & token );

        /// @brief It expects a ';' or '[' token
        ///
        /// The token should either be the ';' meaning the end of element specification,
        /// or the '[' token, which means 'beginning of element's options block'
        ///
        /// Possible next state function(s):
        ///  procElementOptName()
        ///
        /// Also, by calling procElementSemiCol():
        ///  procModifierOrTypeOrSymbolEnd()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procElementSemiColOrOptBeg ( Symbol & self, const String & token );

        /// @brief Processes the token as a name of the next option of this element
        ///
        /// It also checks whether the option is valid for this element type
        ///
        /// Possible next state function(s):
        ///  procElementOptComOrEnd() (in case this is an enum symbols)
        ///  procElementOptEq() (in other cases)
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procElementOptName ( Symbol & self, const String & token );

        /// @brief Treats the token as the value of element's option
        ///
        /// It is used after '-' token, and modifies the next (so current) token to include that '-'.
        /// This is because we treat '-' sign as a separate token, and not as a part
        /// of the value that follows.
        ///
        /// It prepends the token with '-' and passes it to setElementOptValue()
        ///
        /// Possible next state function(s) - by calling setElementOptValue():
        ///  procElementOptComOrEnd()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procElementOptNegValue ( Symbol & self, const String & token );

        /// @brief  Expects a '-' token, or the value of current element's option
        ///
        /// Possible next state function(s):
        ///  procElementOptNegValue()
        ///
        /// Also, by calling setElementOptValue():
        ///  procElementOptComOrEnd()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procElementOptValue ( Symbol & self, const String & token );

        /// @brief It treats the token as the value of current element's option
        ///
        /// It also checks the validity of the data
        ///
        /// Possible next state function(s):
        ///  procElementOptComOrEnd()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool setElementOptValue ( Symbol & self, const String & token );

        /// @brief It expects a '=' token (in '[ option_name = value ]' expression)
        ///
        /// Possible next state function(s):
        ///  procElementOptValue()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procElementOptEq ( Symbol & self, const String & token );

        /// @brief It expects a ',' or ']' token
        ///
        /// Token ',' means that there will be another option,
        /// and ']' means the end of options' block
        ///
        /// Possible next state function(s):
        ///  procElementOptName()
        ///  procElementSemiCol()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procElementOptComOrEnd ( Symbol & self, const String & token );

        /// @brief It expects ';' token
        ///
        /// This function expects only a ';' token, which means the end of the specification
        /// of the current element. It performs various checks, and also adds the element to
        /// the list of elements in this symbol (or defines, if the current element is a 'defined' element)
        ///
        /// Possible next state function(s):
        ///  procModifierOrTypeOrSymbolEnd()
        ///
        /// @param [in] self The symbol object in context of which this function should be executed
        /// @param [in] token The token to process
        /// @return True if the 'self' symbol has just been closed and false otherwise
        static bool procElementSemiCol ( Symbol & self, const String & token );

        /// @brief Returns a unified name
        ///
        /// It takes the name provided, removes all '_' characters, and returns
        /// the lowercase copy of it. It is used as a key in all mappings. This way we can detect
        /// name conflicts regardless of the letter case that was used and the number and position
        /// of '_' characters. These characters may be removed by the language generator to obtain
        /// a nice, camel case (or any other, depending on the language) names.
        ///
        /// @param [in] name The name to unify
        /// @return The unified version of the name
        static String unifiedName ( const String & name );

        friend class ProtocolSpec;
};
}
