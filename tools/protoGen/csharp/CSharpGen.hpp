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
#include "CSharpFile.hpp"

namespace Pravala
{
/// @brief A class that generates the base (common) code for all CSharp generators
///
/// Some elements require more specific code, but this class generates a LOT of
/// generic CSharp code for dealing with the protocol.
///
class CSharpGenerator: public LanguageGenerator
{
    public:
        /// @brief Creates a new language generator
        /// @param [in] proto The protocol specification object, it includes all the data read from
        ///                   the protocol description file (as a tree structure of symbols plus some other settings)
        CSharpGenerator ( ProtocolSpec & proto );

        virtual void init();
        virtual void run();
        virtual String getHelpText();
        virtual SetOptResult setOption ( char shortName, const String & longName, const String & value );

    protected:
        /// @brief The type of the extension (when generating file names)
        enum ExtType
        {
            ExtNoExt, ///< No extension
            ExtImpl ///< Regular file extension
        };

        /// @brief Types of positions, used by hookPosition call
        /// They are in the same order they are generated.
        enum PositionType
        {
            /// @brief The class is about to be opened
            /// We are just before the 'class' keyword
            PosBeforeClass,

            /// @brief The class has been opened
            /// We are just after the first '{' of the class, before anything else in the class
            PosClassOpened,

            /// @brief The class is about to be closed
            /// We are just before the last '}' of the class
            PosBeforeClassClose,

            /// @brief The class has been generated (and closed)
            /// We are just after the last '}' of the class
            PosClassClosed
        };

        /// @brief Holds names of "standard" types for various elements
        struct StdTypes
        {
            String ReadBuffer; ///< The type to be used for buffer variable the data is deserialized from
            String FieldId; ///< The type to be used for field IDs
            String ReadOffset; ///< The type to be used for offset while reading
            String ReadPayloadSize; ///< The type to be used for payload size while reading
            String Enum; ///< The type to be used for enumerator codes
            String WireType; ///< The type to be used for wire type
            String Exception; ///< The exception type
            String IpAddress; ///< The IP address type
            String WriteBuffer; ///< The type to be used for buffer variable the data is serialized to
            String Serializable; ///< The serializable class to be inherited by regular messages
            String BaseSerializable; ///< The serializable class to be inherited by base messages

            /// @brief Constructor, sets default values
            StdTypes();
        } _type;

        /// @brief Holds names of some wire types
        struct SpecialWireTypes
        {
            String VarLenA; ///< The first var-len wire type
            String VarLenB; ///< The second var-len wire type

            /// @brief Constructor, sets default values
            SpecialWireTypes();
        } _wireType;

        /// @brief Names/paths to various methods
        struct StdMethods
        {
            String Encode; ///< Path to the static method to use for encoding
            String Decode; ///< Path to the static method to use for decoding
            String Clear; ///< Name of the 'clear' method
            String Validate; ///< Name of the 'validate' method
            String SetupDefines; ///< Name of the method that configures defines
            String SerializeFields; ///< Name of the method used for serializing fields
            String SerializeMessage; ///< Name of the method used for serializing messages
            String DeserializeField; ///< Name of the method used for deserializing single field
            String DeserializeMessage; ///< Name of the method used for deserializing messages
            String DeserializeFromBase; ///< Name of the method used for deserializing based on the base message

            /// @brief Constructor, sets default values
            StdMethods();
        } _method;

        /// @brief Holds names of "standard" errors
        struct StdErrors
        {
            String OK; ///< No errors
            String InvalidParam; ///< Invalid parameter
            String InvalidData; ///< Invalid data
            String RequiredFieldNotSet; ///< Required field is not set
            String FieldValueOutOfRange; ///< The value is not within allowed range
            String StringLengthOutOfRange; ///< String's length is not within allowed range
            String ListSizeOutOfRange; ///< List's size is not within allowed range
            String DefinedValueMismatch; ///< Some field has a value different than it should be defined to
            String ProtocolError; ///< There was some (other than described by other error codes) protocol error

            /// @brief Constructor, sets default values
            StdErrors();
        } _error;

        /// @brief If not empty, it enables a single implementation file mode
        /// (and this string contains that file's name)
        String _singleImplFilePath;

        const String _fileExt; ///< Extension to use for generated files.

        /// @brief In a single implementation file mode, this is the only file object used!
        CSharpFile * _singleImplFile;

        Symbol * _symString; ///< The symbol that represents the 'string' type
        Symbol * _symIpAddr; ///< The symbol that represents the 'IP address' type

        /// @brief Called for each regular type
        ///
        /// 'Regular' types are all messages (including base messages) and enumerators.
        /// This is not called for primitive types and namespaces.
        ///
        /// It generates some headers, ifdefs and namespace-related things,
        /// calls genRegularSymbol and then closes namespaces and adds some footers.
        ///
        /// @param [in] symbol The symbol to process
        virtual void procRegularSymbol ( Symbol * symbol );

        /// @brief Adds default CSharp imports to the implementation file for the given symbol
        ///
        /// @param [in] symbol The symbol for which the imports are added
        /// @param [in] file The implementation file
        virtual void addDefaultImports ( Symbol * symbol, CSharpFile & file );

        /// @brief Generates the code for all the regular symbols
        ///
        /// It calls either genEnumClass or genMessageClass()
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genRegularSymbol ( Symbol * symbol, CSharpFile & file );

        /// @brief Generates a single 'enum' class
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genEnumClass ( Symbol * symbol, CSharpFile & file );

        /// @brief Generates a single 'message' (or base message) class
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genMessageClass ( Symbol * symbol, CSharpFile & file );

        /// @brief Generates some standard message's methods (serialize, deserialize, etc.)
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genMsgStdMethods ( Symbol * symbol, CSharpFile & file );

        /// @brief Generates the actual fields for storing message's elements
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genMsgFields ( Symbol * symbol, CSharpFile & file );

        /// @brief Generates an 'alias' field
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] elem The alias element
        /// @param [in] file The implementation file
        virtual void genMsgAliasField ( Symbol * symbol, Element * elem, CSharpFile & file );

        /// @brief Generates a regular field
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] elem The element
        /// @param [in] file The implementation file
        virtual void genMsgRegularField ( Symbol * symbol, Element * elem, CSharpFile & file );

        /// @brief Called when we reach certain points in code generation.
        ///
        /// It allows specific language generators to append their code at specific points
        /// of file generation. The 'position' describes the point at which we are.
        /// For example 'just after opening the 'class' block'.
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        /// @param [in] position Describes the current position in the generated files.
        ///              The position actually really matters for the header file,
        ///              but content of both files can be generated at the same time.
        virtual void hookPosition (
            Symbol * symbol, CSharpFile & file,
            const PositionType & position );

        /// @brief Generates absolute 'class path' in CSharp format.
        ///
        /// It adds the 'package prefix' (if configured).
        ///
        /// @param [in] symbol The symbol object to generate the path for
        /// @return The absolute path of the symbol in CSharp format
        virtual String getClassPath ( Symbol * symbol );

        /// @brief Generates the path of the file (relative to the output directory) for the given symbol
        ///
        /// It generates a file path with or without a specified extension type
        ///
        /// @param [in] symbol The symbol object to generate the file path for
        /// @param [in] extType The type of extension to use
        /// @return The path to the file requested.
        virtual String getFilePath ( Symbol * symbol, const ExtType & extType = ExtNoExt );

        /// @brief Returns the symbol that should be used for storing values of enumerators
        /// @return The symbol that should be used for storing values of enumerators
        virtual Symbol * getEnumStorageSymbol();

        /// @brief Returns the symbol that should be used for storing the value of an alias
        /// @param [in] numBits The size of the alias (in the number of bits)
        /// @return The symbol that should be used for storing the value of an alias
        virtual Symbol * getAliasStorageSymbol ( int numBits );

        /// @brief The CSharp type for the element, when used as a variable
        /// @param [in] elem The element to examine
        /// @param [in] useNullable If set to true, the nullable type will be used
        ///                         (if it's different from primitive version).
        /// @return The CSharp type for the element. It takes into account
        ///   whether the element is an alias (and finds the appropriate type), and whether it is
        ///   repeated (and "wraps" it into a List) or not. It uses getRawVarType.
        virtual String getVarType ( const Element * elem, bool useNullable = false );

        /// @brief Returns the CSharp type to store elements of this symbol's type
        /// @param [in] symbol The symbol to return the type for
        /// @param [in] useNullable If set to true, the nullable type will be used
        ///                         (if it's different from primitive version).
        /// @return The CSharp type to store elements of this symbol's type.
        ///          Unlike the getVarType it doesn't care (and it can't) about repeated, aliases, etc.
        virtual String getRawVarType ( Symbol * symbol, bool useNullable = false );

        /// @brief Returns the CSharp type to store a numeric value given the number of bits
        /// @param [in] numBits The number of bits to store
        /// @param [in] isSigned Whether this value should be signed or not
        /// @param [in] useNullable If set to true, the nullable type will be used.
        /// @return The CSharp type to store elements of this symbol's type.
        virtual String getNumericType ( int numBits, bool isSigned, bool useNullable = false );

        /// @brief Returns true if the symbol is nullable (so messages, strings and IP addresses)
        /// It assumes that numeric types and enums are NOT nullable
        /// (so it ignores the fact that we could have int? bool? etc.)
        /// @param [in] symbol The symbol to check
        /// @return True if the symbol is nullable, false otherwise.
        virtual bool isNullable ( Symbol * symbol );

        /// @brief Returns the name of the field
        /// @param [in] elem Element to generate the name for
        /// @return The name of the field
        virtual String getFieldName ( const Element * elem );

        /// @brief Returns the name of the method for retrieving the value of a field (including '()')
        /// @param [in] elem Element to generate the name for
        /// @param [in] includeThis If true, the name will be prefixed with 'this.'
        /// @return The name of the property or method for retrieving the value of a field
        virtual String getGetName ( const Element * elem, bool includeThis = true );

        /// @brief Returns the name of the method for setting the value of a field (NOT including '()')
        /// @param [in] elem Element to generate the name for
        /// @param [in] includeThis If true, the name will be prefixed with 'this.'
        /// @return The name of the property or method for setting the value of a field
        virtual String getSetName ( const Element * elem, bool includeThis = true );

        /// @brief Returns the name of the storage variable for the element
        /// @param [in] elem Element to generate the name for
        /// @param [in] includeThis If true, the name will be prefixed with 'this.'
        /// @return The name of the variable in which this element's value will be stored.
        virtual String getVarName ( const Element * elem, bool includeThis = true );

        /// @brief Returns the name of the constant for the element
        /// @param [in] elem Element to generate the name for
        /// @return The name of the constant with the 'defined' value that will be assigned to specified element
        virtual String getDefName ( const Element * elem );

        /// @brief Returns the name of the method for checking presence of a field (including '()')
        /// @param [in] elem Element to generate the name for
        /// @param [in] includeThis If true, the name will be prefixed with 'this.'
        /// @return The name of the property or method for checking presence of a field
        virtual String getHasName ( const Element * elem, bool includeThis = true );

        /// @brief Returns the bitmask value to use to get the specified number of lower bits
        /// @param [in] numBits The number of bits we want to get using the bitmask
        /// @return The bitmask value to use to get the specified number of lower bits
        virtual String getBitmask ( int numBits );

        /// @brief Returns the name of a class to be extended
        ///
        /// @param [in] symbol The symbol for which we are examining the inheritance
        virtual String getExtends ( Symbol * symbol );

        /// @brief Generates 'remarks' part of the field's comment
        /// @param [in] elem The element to generate the comment for
        /// @param [in] file The implementation file
        /// @param [in] closeBlock If 'false' then the 'remarks' block will not be closed.
        virtual void genFieldRemarks ( Element * e, CSharpFile & file, bool closeBlock = true );

        /// @brief Generates a 'deserialize data' function
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genMsgDeserializeFieldMethod ( Symbol * symbol, CSharpFile & file );

        /// @brief Generates a 'serialize data' function
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genMsgSerializeFieldsMethod ( Symbol * symbol, CSharpFile & file );
};
}
