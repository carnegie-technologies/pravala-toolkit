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
#include "CppFile.hpp"

namespace Pravala
{
/// @brief A class that generates the base (common) code for all C++ generators
///
/// Some elements require more specific code, but this class generates a LOT of
/// generic C++ code for dealing with the protocol.
///
class CppGenerator: public LanguageGenerator
{
    public:
        /// @brief Creates a new C++ language generator
        ///
        /// It needs to be inherited by a more specific language generators,
        /// it only provides a common C++ part (which is still really big)
        ///
        /// @param [in] proto The protocol specification object, it includes all the data read from
        ///                   the protocol description file (as a tree structure of symbols plus some other settings)
        CppGenerator ( ProtocolSpec & proto );

        virtual void init();
        virtual void run();
        virtual String getHelpText();
        virtual SetOptResult setOption ( char shortName, const String & longName, const String & value );

    protected:

        /// @brief The type of the extension (when generating file names)
        enum ExtType
        {
            ExtNoExt, ///< No extension
            ExtHeader, ///< Header extension
            ExtImpl ///< Implementation extension
        };

        /// @brief Various error codes
        ///
        /// This is used by the CppGenerator to "ask" the specific C++ implementation
        /// what values should be used while reporting different types of errors.
        enum ErrorCode
        {
            ErrOK = 0, ///< No errors
            ErrRequiredFieldNotSet, ///< Required field is not set
            ErrFieldValueOutOfRange, ///< The value is not within allowed range
            ErrStringLengthOutOfRange, ///< String's length is not within allowed range
            ErrListSizeOutOfRange, ///< List's size is not within allowed range
            ErrDefinedValueMismatch, ///< Some field has a value different than it should be defined to
            ErrProtocolWarning, ///< There was protocol warning
            ErrProtocolError ///< There was some (other than described by other error codes) protocol error
        };

        /// @brief Types of standard data types
        ///
        /// This is used to "ask" specific C++ implementation generator for data types that should
        /// be used for different things.
        enum StdType
        {
            TypeErrorCode, ///< The type to be used for error codes
            TypeExtError, ///< The type to be used for extended errors
            TypeReadBuffer, ///< The type to be used for buffer variable the data is deserialized from
            TypeWriteBuffer, ///< The type to be used for buffer variable the data is serialized to
            TypeFieldId, ///< The type to be used for field IDs
            TypeWireType, ///< The wire type
            TypeEnum ///< The type to be used for enumerator codes
        };

        /// @brief Different access mode block in the class
        enum AccessType
        {
            AccessPublic, ///< Public
            AccessProtected, ///< Protected
            AccessPrivate ///< Private
        };

        /// @brief Types of positions, used by hookPosition call
        /// They are in the same order they are generated.
        enum PositionType
        {
            /// @brief The class is about to be opened
            /// We are just before the 'class' keyword
            PosBeforeClass,

            /// @brief The class has been opened
            /// We are just after the first '{' of the class, before the public block and the 'public:'
            PosClassOpened,

            /// @brief The 'public' section of the class has just been opened, we are just after 'public:'
            PosPublicBeg,

            /// @brief The 'public' section of the class has been generated and is about to be closed.
            /// We are at the end of it.
            PosPublicEnd,

            /// @brief The 'protected' section of the class has just been opened, we are just after 'protected:'
            PosProtectedBeg,

            /// @brief The 'protected' section of the class has been generated and is about to be closed.
            /// We are at the end of it.
            PosProtectedEnd,

            /// @brief The 'private' section of the class has just been opened, we are just after 'private:'
            PosPrivateBeg,

            /// @brief The 'private' section of the class has been generated and is about to be closed.
            /// We are at the end of it. This is also the last chance to put anything inside the body
            /// of this class.
            PosPrivateEnd,

            /// @brief The class has been generated (and closed)
            /// We are just after the last '};' of the class
            PosClassClosed
        };

        /// @brief Used for modifying the returned variable type, based on how it's used.
        enum VarUseType
        {
            VarUseStorage, ///< The basic type, used for storage.
            VarUseGetter,  ///< The type used when returning the field's value.
            VarUseSetter   ///< The type used when setting the field's value
        };

        /// @brief If not empty, it enables a single implementation file mode
        /// (and this string contains that file's path)
        String _singleImplFilePath;

        String _hdrExt; ///< The extension to use for header files.
        String _implExt; ///< The extension to use for implementation files.

        /// @brief Directory that will be added at the beginning of generated file paths.
        /// @note This does not apply to flag files, or to implementation file in "single implementation file" mode.
        String _dirPrefix;

        /// @brief If set to true, the base name of a protocol file will be added as a prefix directory
        ///        in a path for each file generated using that input file.
        /// @note This does not apply to flag files, or to implementation file in "single implementation file" mode.
        bool _useProtoFileAsPrefix;

        bool _genDebugSymbols; ///< Whether or not some additional debug code should be generated.

        bool _usePragmaOnce; ///< Whether 'pragma once' can be used in the generated code.

        /// @brief In a single implementation file mode, this is the only file object used!
        CppFile * _singleImplFile;

        List<Symbol *> _intEnumTypes; ///< The list of "pending" enum symbols (we need getHash functions for them)

        /// @brief Adds default C and C++ includes to the header and implementation files for the given symbol
        ///
        /// It adds some required standard includes.
        /// It also adds to the implementation file an include to the header file.
        ///
        /// @param [in] symbol The symbol for which the includes are added
        /// @param [in] hdrFile The header file
        /// @param [in] implFile The implementation file
        virtual void addDefaultIncludes ( Symbol * symbol, CppFile & hdrFile, CppFile & implFile );

        /// @brief Generates the code for all the regular symbols
        ///
        /// It calls either genEnumClass or genMessageClass()
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] hdrFile The header file
        /// @param [in] implFile The implementation file
        virtual void genRegularSymbol ( Symbol * symbol, CppFile & hdrFile, CppFile & implFile );

        /// @brief Generates a single 'enum' class
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] hdrFile The header file
        /// @param [in] implFile The implementation file
        virtual void genEnumClass ( Symbol * symbol, CppFile & hdrFile, CppFile & implFile );

        /// @brief Generates a single 'message' (or base message) class
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] hdrFile The header file
        /// @param [in] implFile The implementation file
        virtual void genMessageClass ( Symbol * symbol, CppFile & hdrFile, CppFile & implFile );

        /// @brief Generates a class 'header'
        ///
        /// The 'header' is the first line: 'class Foo: public Base'
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] hdrFile The header file
        /// @param [in] implFile The implementation file
        virtual void genClassHeader ( Symbol * symbol, CppFile & hdrFile, CppFile & implFile );

        /// @brief Generates standard methods for the symbol's fields
        ///
        /// It is called once for each access mode (public, protected and private)
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] hdrFile The header file
        /// @param [in] implFile The implementation file
        /// @param [in] accessType The access type for which the methods are generated.
        ///                It is set to currently generated block. If it is public,
        ///                all '(b)get' and 'has' functions are generated for all elements,
        ///                as well as all '(b)set' and 'mod' functions for public elements.
        ///                If it's not public, only '(b)set' and 'mod' functions are generated
        ///                for all the elements that have matching access mode.
        virtual void genMsgFieldMethods (
            Symbol * symbol, CppFile & hdrFile, CppFile & implFile,
            const AccessType & accessType );

        /// @brief Generates some standard message's methods (serialize, deserialize, etc.)
        ///
        /// It is run while the 'public' block is generated.
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] hdrFile The header file
        /// @param [in] implFile The implementation file
        virtual void genMsgStdMethods ( Symbol * symbol, CppFile & hdrFile, CppFile & implFile );

        /// @brief Generates some standard message's private helper methods (localClear, etc.)
        ///
        /// It is run while the 'private' block is generated.
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] hdrFile The header file
        /// @param [in] implFile The implementation file
        virtual void genMsgStdPrivMethods ( Symbol * symbol, CppFile & hdrFile, CppFile & implFile );

        /// @brief Generates the 'deserialize' method for the message
        ///
        /// It is run while the 'public' block is generated.
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] hdrFile The header file
        /// @param [in] implFile The implementation file
        virtual void genMsgDeserializeFieldMethod ( Symbol * symbol, CppFile & hdrFile, CppFile & implFile );

        /// @brief Generates the 'serialize' method for the message
        ///
        /// It is run while the 'public' block is generated.
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] hdrFile The header file
        /// @param [in] implFile The implementation file
        virtual void genMsgSerializeFieldsMethod ( Symbol * symbol, CppFile & hdrFile, CppFile & implFile );

        /// @brief Generates the actual fields for storing message's elements
        ///
        /// It is run while the 'private' block is generated.
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] hdrFile The header file
        /// @param [in] implFile The implementation file
        virtual void genMsgFields ( Symbol * symbol, CppFile & hdrFile, CppFile & implFile );

        /// @brief Called when we reach certain points in code generation.
        ///
        /// It allows specific language generators to append their code at specific points
        /// of file generation. The 'position' describes the point at which we are.
        /// For example 'just after opening the 'private' block in the class'.
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] hdrFile The header file
        /// @param [in] implFile The implementation file
        /// @param [in] position Describes the current position in the generated files.
        ///              The position actually really matters for the header file,
        ///              but content of both files can be generated at the same time.
        virtual void hookPosition (
            Symbol * symbol, CppFile & hdrFile, CppFile & implFile,
            const PositionType & position );

        /// @brief Generates absolute 'class path' in C++ format.
        ///
        /// It adds the 'namespace prefix' (if configured) and changes '.' from paths used
        /// in protocol description language to '::' used by C++.
        ///
        /// @param [in] symbol The symbol object to generate the path for
        /// @return The absolute path of the symbol in C++ format
        virtual String getClassPath ( Symbol * symbol );

        /// @brief Generates the path of the file (relative to the output directory) for the given symbol
        ///
        /// It generates a file path with or without a specified extension type
        ///
        /// @param [in] symbol The symbol object to generate the file path for
        /// @param [in] extType The type of extension to use
        /// @param [in] fromSymbol If used, the path generated will be relative to the location
        ///                         of fromSymbol symbol.
        /// @return The path to the file requested.
        virtual String getFilePath ( Symbol * symbol, const ExtType & extType = ExtNoExt, Symbol * fromSymbol = 0 );

        /// @brief Returns the size of the presence variable in bits
        /// @return the size of a single 'presence' variable (in BITS, for example for uint32_t it's 32)
        virtual int getPresVarSize();

        /// @brief Returns the shift of the presence bit for the given element index
        /// @param [in] elemIndex The index of the element
        /// @return The 'shift' (so a bit number) of the presence bit representing the element with the given index
        virtual int getPresVarShift ( int elemIndex );

        /// @brief Returns the type of the presence variable
        /// @return The type of the presence variable (for example uint32_t)
        virtual String getPresVarType();

        /// @brief Returns the name of the 'presence' variable for the element with the given index
        /// @param [in] elemIndex The index of the element
        /// @return The name of the 'presence' variable for the element with the given index
        virtual String getPresVarNameIdx ( int elemIndex );

        /// @brief Generates the name for the given presence variable.
        ///
        /// It is just for generating something like "_pres_var_1", "_pres_var_2", etc.
        ///
        /// @param [in] varNum the presence variable number
        /// @return The name of the presence variable with the given number
        virtual String getPresVarNameNum ( int varNum );

        /// @brief The C++ type for the element
        /// @param [in] hdr The header file being generated (additional includes may be added to it)
        /// @param [in] elem The element to examine
        /// @param [in] useType The context in which the variable is being used.
        /// @return The C++ type for the element. It takes into account
        ///   whether the element is an alias (and finds the appropriate type), and whether it is
        ///   repeated (and "wraps" it into a List template) or not. It uses getRawVarType.
        virtual String getVarType ( CppFile & hdr, Element * elem, VarUseType useType = VarUseStorage );

        /// @brief Returns the C++ type to store elements of this symbol's type
        /// @param [in] hdr The header file being generated (additional includes may be added to it)
        /// @param [in] symbol The symbol to return the type for
        /// @param [in] useType The context in which the variable is being used.
        /// @return The C++ type to store elements of this symbol's type.
        ///          Unlike the getVarType it doesn't care (and it can't) about repeated, aliases, etc.
        virtual String getRawVarType ( CppFile & hdr, Symbol * symbol, VarUseType useType = VarUseStorage );

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

        /// @brief Returns the expression for clearing an element variable
        /// This version returns an empty string for things it doesn't support
        /// @param [in] elem The element we need to clear
        /// @return The expression to be used to 'clear' the given element
        virtual String exprVarClear ( Element * elem );

        // Pure virtual functions:

        /// @brief Returns the name of the list type for storing specified element type
        /// @param [in] hdr The header file being generated (additional includes may be added to it)
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] useType The context in which the variable is being used.
        /// @return The name of the list type that can be used for storing elements of the given type (symbol)
        virtual String getListVarType ( CppFile & hdr, Symbol * intSymbol, VarUseType useType = VarUseStorage ) = 0;

        /// @brief Returns one of the "standard types"
        /// @param [in] stdType Which type should be returned
        /// @return The name of the type to use for several standard elements (types)
        virtual String getStdType ( const StdType & stdType ) = 0;

        /// @brief Returns the error code to be used for specific error
        /// @param [in] errCode The type of the error to return the code for
        /// @return The value to use as the given error code
        virtual String getErrorCode ( const ErrorCode & errCode ) = 0;

        /// @brief Returns the expression for reading string's length
        /// @param [in] strVarName The name of the variable
        /// @return The expression used to get the length of the string
        virtual String exprStringVarLength ( const String & strVarName ) = 0;

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

        /// @brief Returns the expression for getting a reference to specified element in the list
        /// @param [in] intSymbol The symbol (type) stored inside the list
        /// @param [in] listVarName The name of the list variable
        /// @param [in] indexVarName The name of the index variable
        /// @return The expression used for getting a reference to the element specified with its index in the list
        virtual String exprListGetElemIdxRef (
            Symbol * intSymbol, const String & listVarName,
            const String & indexVarName ) = 0;

        /// @brief Returns the expression for encoding data to the buffer
        /// @param [in] bufVarName The name of the write buffer variable
        /// @param [in] valueVarName The name of the variable to encode
        /// @param [in] valueCode The value/variable name with the field code (ID)
        /// @return The expression to append (serialize) the specified variable to the buffer
        virtual String exprProtoEncode (
            const String & bufVarName, const String & valueVarName,
            const String & valueCode ) = 0;

        /// @brief Returns the expression for reading field's size from the buffer
        /// @param [in] bufVarName The name of the read buffer variable
        /// @param [in] offset The offset within the buffer
        /// @param [in] fieldSize The size of the field (starting from offset)
        /// @param [in] wireType The type of encoding.
        /// @param [in] fieldVarName The field variable to decode the data to
        /// @return The expression to decode the data of the next field in the buffer (which DOES modify the offset)
        virtual String exprProtoDecodeFieldValue (
            const String & bufVarName, const String & offset,
            const String & fieldSize, const String & wireType, const String & fieldVarName ) = 0;

        /// @brief Returns an expression for checking if the wire type is one of variable length formats
        /// @param [in] wireTypeVarName The name of the wire type variable.
        virtual String exprVarLenWireTypeCheck ( const String & wireTypeVarName ) = 0;

        /// @brief Generates a code for configuring the extended error.
        /// @param [in] file The file in which the code should be generated
        /// @param [in] indent The base indentation to use
        /// @param [in] errCode The error code to set.
        /// @param [in] errMessage The error message to set.
        /// @param [in] addPtrCheck Whether a pointer validity check should be added or not.
        /// @param [in] extErrorPtrName The name of the extended error pointer.
        virtual void genSetupExtError (
            CppFile & file, int indent,
            const String & errCode,
            const String & errMessage,
            bool addPtrCheck = true,
            const String & extErrorPtrName = "extError" ) = 0;

        /// @brief Generates code for serializing a message
        /// @param [in] file The file in which the code should be generated
        /// @param [in] indent The base indentation to use
        /// @param [in] symbol The symbol of the message
        /// @param [in] varName The name of the variable to serialize
        /// @param [in] bufVarName The name of the write buffer variable
        /// @param [in] resultVarName The name of the variable in which the result error code should be put
        /// @param [in] extErrVarName The name of the variable with extended error
        virtual void genSerializeMessage (
            CppFile & file, int indent, Symbol * symbol, const String & varName,
            const String & bufVarName, const String & resultVarName,
            const String & extErrVarName ) = 0;

        /// @brief Generates code for deserializing a message
        /// @param [in] file The file in which the code should be generated
        /// @param [in] indent The base indentation to use
        /// @param [in] symbol The symbol of the message
        /// @param [in] varName The name of the variable to deserialize the data into
        /// @param [in] bufVarName The name of the read buffer variable
        /// @param [in] fieldSizeExpr The expression to calculate field size
        /// @param [in] offsetVarName The name of the variable with the offset
        /// @param [in] resultVarName The name of the variable in which the result error code should be put
        /// @param [in] extErrVarName The name of the variable with extended error
        virtual void genDeserializeMessage (
            CppFile & file, int indent, Symbol * symbol, const String & varName,
            const String & bufVarName, const String & fieldSizeExpr,
            const String & offsetVarName, const String & resultVarName,
            const String & extErrVarName ) = 0;

        /// @brief Generates functions to get hash values from enums.
        /// @param [in] hdr Header file
        virtual void genEnumHashGets ( CppFile & hdr ) = 0;

        /// @brief Generates code for modifying the object's state (clearing cache, etc) when it's modified.
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The file that contains the function
        /// @param [in] indent The indent level at which to generate this code
        virtual void genObjectModified ( Symbol * symbol, CppFile & file, int indent ) = 0;

        virtual void procRegularSymbol ( Symbol * symbol );
        virtual String getOutputFilePath ( FileObject * file );
};
}
