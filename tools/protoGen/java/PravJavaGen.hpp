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

#include "JavaGen.hpp"

namespace Pravala
{
/// @brief Class that is the "glue" between the automatically generated protocol classes and the rest of the system
///
/// It provides expressions and methods that heavily depend on data types and
/// functions provided by the rest of the system. It also sets up some simple
/// parameters used in the generator (like buffer's type)
///
class PravalaJavaGenerator: public JavaGenerator
{
    public:
        /// @brief Creates a new language generator
        /// @param [in] proto The protocol specification object, it includes all the data read from
        ///                   the protocol description file (as a tree structure of symbols plus some other settings)
        PravalaJavaGenerator ( ProtocolSpec & proto );

    protected:
        Symbol * _symString; ///< The symbol that represents the 'string' type
        Symbol * _symIpAddr; ///< The symbol that represents the 'IP address' type

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

        /// @brief Returns the name of a class to be extended
        ///
        /// @param [in] symbol The symbol for which we are examining the inheritance
        virtual String getExtends ( Symbol * symbol );

        /// @brief Returns the error code to be used for specific error
        /// @param [in] errCode The type of the error to return the code for
        /// @return The value to use as the given error code
        virtual String getErrorCode ( const ErrorCode & errCode );

        /// @brief Returns one of the "standard types"
        /// @param [in] stdType Which type should be returned
        /// @return The name of the type to use for several standard elements (types)
        virtual String getStdType ( const StdType & stdType );

        /// @brief Returns the name of the list type for storing specified element type when used as an 'interface'
        /// @param [in] symbol The symbol object to generate the code for
        /// @return The name of the list type that can be used for storing elements of the given type (symbol)
        virtual String getListIfaceType ( Symbol * intSymbol );

        /// @brief Returns the name of the list type for storing specified element type when used as a variable
        /// @param [in] symbol The symbol object to generate the code for
        /// @return The name of the list type that can be used for storing elements of the given type (symbol)
        virtual String getListVarType ( Symbol * intSymbol );

        /// @brief Returns the Java type to store elements of this symbol's type
        /// @param [in] symbol The symbol to return the type for
        /// @return The Java type to store elements of this symbol's type.
        ///          Unlike the getVarType it doesn't care (and it can't) about repeated, aliases, etc.
        virtual String getRawVarType ( Symbol * symbol );

        /// @brief Returns the primitive Java type to store elements of this symbol's type
        /// @param [in] symbol The symbol to return the type for
        /// @return The primitive Java type to store elements of this symbol's type.
        ///          Unlike the getVarType it doesn't care (and it can't) about repeated, aliases, etc.
        virtual String getRawPrimitiveVarType ( Symbol * symbol );

        /// @brief Returns the expression for throwing an error exception
        /// @param [in] errCode The error code to throw
        /// @return The expression for throwing an error exception
        virtual String exprThrowException ( const JavaGenerator::ErrorCode & errCode );

        /// @brief Returns the expression for throwing an error exception
        /// @param [in] errCodeVarName The name of the variable with the error code to throw
        /// @return The expression for throwing an error exception
        virtual String exprThrowException ( const String & errCodeVarName );

        /// @brief Returns the expression for clearing an element variable
        /// @param [in] elem The element we need to clear
        /// @return The expression to be used to 'clear' the given element
        virtual String exprVarClear ( Element * elem );

        /// @brief Returns the expression for reading string's length
        /// @param [in] strVarName The name of the variable
        /// @return The expression used to get the length of the string
        virtual String exprStringVarLength ( const String & listVarName );

        /// @brief Returns the expression for appending to the list
        /// @param [in] intSymbol The symbol (type) stored inside the list
        /// @param [in] listVarName The name of the list variable
        /// @param [in] appendVarName The name of the variable that should be added to the list
        /// @return The expression for adding an element to the list
        virtual String exprListAppend (
            Symbol * intSymbol, const String & listVarName,
            const String & appendVarName );

        /// @brief Returns the expression for getting a specified element in the list
        /// @param [in] intSymbol The symbol (type) stored inside the list
        /// @param [in] listVarName The name of the list variable
        /// @param [in] indexVarName The name of the index variable
        /// @return The expression used for getting an element specified with its index in the list
        virtual String exprListGetElemIdx (
            Symbol * intSymbol, const String & listVarName,
            const String & indexVarName );

        /// @brief Returns the expression for reading list's size
        /// @param [in] intSymbol The symbol (type) stored inside the list
        /// @param [in] varName The name of the list variable
        /// @return The expression used to get the size of the list
        virtual String exprListVarSize ( Symbol * intSymbol, const String & varName );

        /// @brief Returns the expression for de-initializing a write buffer
        /// @param [in] bufVarName The name of the variable to use
        /// @return The expression for de-initializing the write buffer
        virtual String exprDeleteWriteBuf ( const String & bufVarName );

        /// @brief Returns the expression for declaring and initializing a new write buffer
        /// @param [in] bufVarName The name of the variable to use
        /// @return The expression for declaring and initializing a new write buffer
        virtual String exprDeclareAndCreateWriteBuf ( const String & bufVarName );

        /// @brief Returns the expression for encoding data to the buffer
        /// @param [in] bufVarName The name of the write buffer variable
        /// @param [in] valueVarName The name of the variable to encode
        /// @param [in] valueCode The value/variable name with the field code (ID)
        /// @return The expression to append (serialize) the specified variable to the buffer
        virtual String exprProtoEncode (
            const String & bufVarName,
            const String & valueVarName,
            const String & valueCode );

        virtual String exprProtoDecodeFieldValue (
            Symbol * symbolType,
            const String & hdrDescVarName,
            const String & bufDescVarName,
            const String & fieldVarName );

        /// @brief Generates a 'serialize data' function
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genMsgSerializeMethod ( Symbol * symbol, JavaFile & file );

        /// @brief Generates a 'deserialize data' function
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genMsgDeserializeMethod ( Symbol * symbol, JavaFile & file );

        /// @brief Generates a 'serialize enum' function
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genEnumSerializeMethod ( Symbol * symbol, JavaFile & file );

        /// @brief Generates a 'deserialize enum' function
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genEnumDeserializeMethod ( Symbol * symbol, JavaFile & file );

        /// @brief Generates a 'deserialize from the base message' function
        ///
        /// This works for both regular and base messages.
        /// The code generated performs some basic tests on the base message (compares the
        /// values of those 'defined' fields that are 'defined' in this message and stored
        /// in the base message). This way it can exit early with an error (when something is not matching)
        /// before even trying to deserialize anything. This is just faster way of determining whether
        /// this message could potentially be deserialized from the buffer stored in the base message.
        /// The actual deserialization might still fail, but if some of the defined fields are stored in the base
        /// message and something in out inheritance structure defines them we should be able to quickly
        /// detect most of the errors, without having to do the actual deserialization, that is much more expensive.
        ///
        /// @param [in] symbol The symbol object to generate the code for
        /// @param [in] file The implementation file
        virtual void genDeserFromBaseFunc ( Symbol * symbol, JavaFile & file );
};
}
