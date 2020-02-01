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
#include "JavaFile.hpp"

namespace Pravala
{
/// @brief A class that generates the base (common) code for all Java generators
///
/// Some elements require more specific code, but this class generates a LOT of
/// generic Java code for dealing with the protocol.
///
class JavaGenerator: public LanguageGenerator
{
    public:
        /// @brief Creates a new language generator
        /// @param [in] proto The protocol specification object, it includes all the data read from
        ///                   the protocol description file (as a tree structure of symbols plus some other settings)
        JavaGenerator ( ProtocolSpec & proto );

        virtual void init();

    protected:

        const String _fileExt; ///< Extension to use for generated files.

        /// @brief The type of the extension (when generating file names)
        enum ExtType
        {
            ExtNoExt, ///< No extension
            ExtImpl ///< Regular file extension
        };

        /// @brief Various error codes
        ///
        /// This is used by the the generator to "ask" the specific Java implementation
        /// what values should be used while reporting different types of errors.
        enum ErrorCode
        {
            ErrOK = 0, ///< No errors
            ErrInvalidParam, ///< Invalid parameter
            ErrInvalidData, ///< Invalid data
            ErrRequiredFieldNotSet, ///< Required field is not set
            ErrFieldValueOutOfRange, ///< The value is not within allowed range
            ErrStringLengthOutOfRange, ///< String's length is not within allowed range
            ErrListSizeOutOfRange, ///< List's size is not within allowed range
            ErrDefinedValueMismatch, ///< Some field has a value different than it should be defined to
            ErrProtocolError///< There was some (other than described by other error codes) protocol error
        };

        /// @brief Types of standard data types
        ///
        /// This is used to "ask" specific C++ implementation generator for data types that should
        /// be used for different things.
        enum StdType
        {
            TypeErrorCode, ///< The type to be used for error codes
            TypeReadBuffer, ///< The type to be used for buffer variable the data is deserialized from
            TypeWriteBuffer, ///< The type to be used for buffer variable the data is serialized to
            TypeProtoException, ///< The type to be used for protocol exception
            TypeFieldId ///< The type to be used for field IDs
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

        /// @brief Adds default Java imports to the implementation file for the given symbol
        ///
        /// @param [in] symbol The symbol for which the imports are added
        /// @param [in] file The implementation file
        virtual void addDefaultImports ( Symbol * symbol, JavaFile & file );

        /// @brief Generates the code for all the regular symbols
        ///
        /// It calls either genEnumClass or genMessageClass()
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        /// @param [in] nested Whether the class for symbol 'symbol' is nested inside another class or not
        ///               (in which case it's top-level symbol). Nested classes need to use 'static' keyword.
        virtual void genRegularSymbol ( Symbol * symbol, JavaFile & file, bool nested = false );

        /// @brief Generates a single 'enum' class
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        /// @param [in] nested Whether the class for symbol 'symbol' is nested inside another class or not
        ///               (in which case it's top-level symbol). Nested classes need to use 'static' keyword.
        virtual void genEnumClass ( Symbol * symbol, JavaFile & file, bool nested = false );

        /// @brief Generates a single 'message' (or base message) class
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        /// @param [in] nested Whether the class for symbol 'symbol' is nested inside another class or not
        ///               (in which case it's top-level symbol). Nested classes need to use 'static' keyword.
        virtual void genMessageClass ( Symbol * symbol, JavaFile & file, bool nested = false );

        /// @brief Generates standard methods for the symbol's fields
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genMsgFieldMethods ( Symbol * symbol, JavaFile & file );

        /// @brief Generates some standard message's methods (serialize, deserialize, etc.)
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genMsgStdMethods ( Symbol * symbol, JavaFile & file );

        /// @brief Generates the actual fields for storing message's elements
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genMsgFields ( Symbol * symbol, JavaFile & file );

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
            Symbol * symbol, JavaFile & file,
            const PositionType & position );

        /// @brief Generates absolute 'class path' in Java format.
        ///
        /// It adds the 'package prefix' (if configured).
        ///
        /// @param [in] symbol The symbol object to generate the path for
        /// @return The absolute path of the symbol in Java format
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

        /// @brief The Java type for the element, when used as an 'interface'
        /// @param [in] elem The element to examine
        /// @return The Java type for the element. It takes into account
        ///   whether the element is an alias (and finds the appropriate type), and whether it is
        ///   repeated (and "wraps" it into a List) or not. It uses getRawVarType.
        virtual String getIfaceType ( Element * elem );

        /// @brief The Java type for the element, when used as a variable
        /// @param [in] elem The element to examine
        /// @return The Java type for the element. It takes into account
        ///   whether the element is an alias (and finds the appropriate type), and whether it is
        ///   repeated (and "wraps" it into a List) or not. It uses getRawVarType.
        virtual String getVarType ( Element * elem );

        /// @brief The Java literal for the element and a given value
        /// @param [in] elem The element to get a literal value for
        /// @param [in] value The value to format as a literal
        /// @return The Java literal for the element. It uses the Symbol from the element to determine
        ///   whether any prefixes or suffixes need to be added to the value.
        virtual String getVarLiteral ( Element * elem, String value );

        /// @brief Returns the name of the variable for the element
        /// @param [in] elem Element to generate the name for
        /// @return The name of the variable in which this element's value will be stored.
        virtual String getVarName ( Element * elem );

        /// @brief Returns the name of the constant for the element
        /// @param [in] elem Element to generate the name for
        /// @return The name of the constant with the 'defined' value that will be assigned to specified element
        virtual String getDefName ( Element * elem );

        /// @brief Returns the name of the field ID constant for the element
        /// @param [in] elem Element to generate the name for
        /// @return The name of the constant with the field ID value for the specified element
        virtual String getFieldIdName ( Element * elem );

        /// @brief Returns the bitmask value to use to get the specified number of lower bits
        /// @param [in] numBits The number of bits we want to get using the bitmask
        /// @return The bitmask value to use to get the specified number of lower bits
        virtual String getBitmask ( int numBits );

        /// @brief Returns the name of a class to be extended
        ///
        /// @param [in] symbol The symbol for which we are examining the inheritance
        virtual String getExtends ( Symbol * symbol );

        /// @brief Returns the name(s) of interfaces to implement
        ///
        /// @param [in] symbol The symbol which we are examining
        virtual StringList getImplements ( Symbol * symbol );

        // Pure virtual functions:

        /// @brief Generates a 'deserialize data' function
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genMsgDeserializeMethod ( Symbol * symbol, JavaFile & file ) = 0;

        /// @brief Generates a 'deserialize enum' function
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genEnumDeserializeMethod ( Symbol * symbol, JavaFile & file ) = 0;

        /// @brief Generates a 'serialize data' function
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genMsgSerializeMethod ( Symbol * symbol, JavaFile & file ) = 0;

        /// @brief Generates a 'serialize enum' function
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genEnumSerializeMethod ( Symbol * symbol, JavaFile & file ) = 0;

        /// @brief Returns the Java type to store elements of this symbol's type
        /// @param [in] symbol The symbol to return the type for
        /// @return The Java type to store elements of this symbol's type.
        ///          Unlike the getVarType it doesn't care (and it can't) about repeated, aliases, etc.
        virtual String getRawVarType ( Symbol * symbol ) = 0;

        /// @brief Returns the primitive Java type to store elements of this symbol's type
        /// @param [in] symbol The symbol to return the type for
        /// @return The primitive Java type to store elements of this symbol's type.
        ///          Unlike the getVarType it doesn't care (and it can't) about repeated, aliases, etc.
        virtual String getRawPrimitiveVarType ( Symbol * symbol ) = 0;

        /// @brief Returns the name of the list type for storing specified element type when used as an 'interface'
        /// @param [in] symbol The symbol object to generate the code for
        /// @return The name of the list type that can be used for storing elements of the given type (symbol)
        virtual String getListIfaceType ( Symbol * intSymbol ) = 0;

        /// @brief Returns the name of the list type for storing specified element type when used as a variable
        /// @param [in] symbol The symbol object to generate the code for
        /// @return The name of the list type that can be used for storing elements of the given type (symbol)
        virtual String getListVarType ( Symbol * intSymbol ) = 0;

        /// @brief Returns one of the "standard types"
        /// @param [in] stdType Which type should be returned
        /// @return The name of the type to use for several standard elements (types)
        virtual String getStdType ( const StdType & stdType ) = 0;

        /// @brief Returns the error code to be used for specific error
        /// @param [in] errCode The type of the error to return the code for
        /// @return The value to use as the given error code
        virtual String getErrorCode ( const ErrorCode & errCode ) = 0;

        /// @brief Returns the expression for throwing an error exception
        /// @param [in] errCode The error code to throw
        /// @return The expression for throwing an error exception
        virtual String exprThrowException ( const ErrorCode & errCode ) = 0;

        /// @brief Returns the expression for throwing an error exception
        /// @param [in] errCodeVarName The name of the variable with the error code to throw
        /// @return The expression for throwing an error exception
        virtual String exprThrowException ( const String & errCodeVarName ) = 0;

        /// @brief Returns the expression for declaring and initializing a new write buffer
        /// @param [in] bufVarName The name of the variable to use
        /// @return The expression for declaring and initializing a new write buffer
        virtual String exprDeclareAndCreateWriteBuf ( const String & bufVarName ) = 0;

        /// @brief Returns the expression for reading string's length
        /// @param [in] strVarName The name of the variable
        /// @return The expression used to get the length of the string
        virtual String exprStringVarLength ( const String & listVarName ) = 0;

        /// @brief Returns the expression for clearing an element variable
        /// @param [in] elem The element we need to clear
        /// @return The expression to be used to 'clear' the given element
        virtual String exprVarClear ( Element * elem ) = 0;

        /// @brief Returns the expression for reading list's size
        /// @param [in] intSymbol The symbol (type) stored inside the list
        /// @param [in] varName The name of the list variable
        /// @return The expression used to get the size of the list
        virtual String exprListVarSize ( Symbol * intSymbol, const String & varName ) = 0;

        /// @brief Returns the expression for appending to the list
        /// @param [in] intSymbol The symbol (type) stored inside the list
        /// @param [in] listVarName The name of the list variable
        /// @param [in] appendVarName The name of the variable that should be added to the list
        /// @return The expression for adding an element to the list
        virtual String exprListAppend (
            Symbol * intSymbol, const String & listVarName,
            const String & appendVarName ) = 0;

        /// @brief Returns the expression for getting a specified element in the list
        /// @param [in] intSymbol The symbol (type) stored inside the list
        /// @param [in] listVarName The name of the list variable
        /// @param [in] indexVarName The name of the index variable
        /// @return The expression used for getting an element specified with its index in the list
        virtual String exprListGetElemIdx (
            Symbol * intSymbol, const String & listVarName,
            const String & indexVarName ) = 0;

        /// @brief Returns the expression for encoding data to the buffer
        /// @param [in] bufVarName The name of the write buffer variable
        /// @param [in] valueVarName The name of the variable to encode
        /// @param [in] valueCode The value/variable name with the field code (ID)
        /// @return The expression to append (serialize) the specified variable to the buffer
        virtual String exprProtoEncode (
            const String & bufVarName,
            const String & valueVarName,
            const String & valueCode ) = 0;

        /// @brief Returns the expression for reading field's size from the buffer
        /// @param [in] symbolType The symbol (type) which we're decoding.
        /// @param [in] bufVarName The name of the read buffer variable
        /// @param [in] hdrDescVarName The name of the variable with field header.
        /// @param [in] fieldVarName The field variable to decode the data to
        /// @return The expression to decode the data of the next field in the buffer (which DOES modify the offset)
        virtual String exprProtoDecodeFieldValue (
            Symbol * symbolType,
            const String & hdrDescVarName,
            const String & bufDescVarName,
            const String & fieldVarName ) = 0;
};
}
