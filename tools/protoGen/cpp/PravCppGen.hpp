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

#include "CppGen.hpp"

namespace Pravala
{
/// @brief A class that adds the "glue" between the automatically generated C++ code and the rest of Pravala
///
/// It provides expressions and methods that heavily depend on data types and
/// functions provided by the rest of the system. It also sets up some simple
/// parameters used in the generator (like buffer's type)
class PravalaCppGenerator: public CppGenerator
{
    public:
        /// @brief Creates a new 'Pravala C++' language generator
        /// @param [in] proto The protocol specification object, it includes all the data read from
        ///                   the protocol description file (as a tree structure of symbols plus some other settings)
        PravalaCppGenerator ( ProtocolSpec & proto );

        virtual String getHelpText();
        virtual SetOptResult setOption ( char shortName, const String & longName, const String & value );

    protected:
        const Symbol * const _symString; ///< The symbol that represents the 'string' type
        const Symbol * const _symIpAddr; ///< The symbol that represents the 'IP address' type
        const Symbol * const _symTimestamp; ///< The symbol that represents the 'timestamp' type

        bool _enableJson; ///< Whether JSON support should be enabled.

        virtual void hookPosition (
            Symbol * symbol, CppFile & hdrFile, CppFile & implFile,
            const PositionType & position );
        virtual void addDefaultIncludes ( Symbol * symbol, CppFile & hdrFile, CppFile & implFile );

        virtual String getErrorCode ( const ErrorCode & errCode );
        virtual String getStdType ( const StdType & stdType );
        virtual String getListVarType ( CppFile & hdr, Symbol * intSymbol, VarUseType useType );
        virtual String getRawVarType ( CppFile & hdr, Symbol * symbol, VarUseType useType );

        virtual String exprProtoDecodeFieldValue (
            const String & bufVarName, const String & offset,
            const String & fieldSize, const String & wireType, const String & fieldVarName );

        virtual String exprProtoEncode (
            const String & bufVarName,
            const String & valueVarName,
            const String & valueCode );

        virtual String exprListAppend (
            Symbol * intSymbol, const String & listVarName,
            const String & appendVarName );

        virtual String exprListGetElemIdxRef (
            Symbol * intSymbol, const String & listVarName,
            const String & indexVarName );

        virtual String exprListVarSize ( Symbol * intSymbol, const String & varName );
        virtual String exprStringVarLength ( const String & strVarName );
        virtual String exprVarLenWireTypeCheck ( const String & wireTypeVarName );
        virtual String exprVarClear ( Element * elem );

        virtual void genObjectModified ( Symbol * symbol, CppFile & file, int indent );

        virtual void genSetupExtError (
            CppFile & file, int indent,
            const String & errCode,
            const String & errMessage,
            bool addPtrCheck = true,
            const String & extErrorPtrName = "extError" );

        virtual void genSerializeMessage (
            CppFile & file, int indent, Symbol * symbol, const String & varName,
            const String & bufVarName, const String & resultVarName,
            const String & extErrVarName );

        virtual void genDeserializeMessage (
            CppFile & file, int indent, Symbol * symbol, const String & varName,
            const String & bufVarName, const String & fieldSizeExpr,
            const String & offsetVarName, const String & resultVarName,
            const String & extErrVarName );

        virtual void genMsgSerializeFieldsMethod ( Symbol * symbol, CppFile & hdrFile, CppFile & implFile );

        virtual void genClassHeader ( Symbol * symbol, CppFile & hdrFile, CppFile & implFile );
        virtual void genTestBaseDefsFunc ( Symbol * symbol, CppFile & hdrFile, CppFile & implFile );
        virtual void genDumpFunc ( Symbol * symbol, CppFile & hdrFile, CppFile & implFile );
        virtual void genEnumHashGets ( CppFile & hdr );
};
}
