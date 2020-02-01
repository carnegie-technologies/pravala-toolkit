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

#include <cassert>

#include "ProtoSpec.hpp"
#include "basic/String.hpp"
#include "Symbol.hpp"
#include "Error.hpp"

using namespace Pravala;

const char * ProtocolSpec::KW_DEFAULT ( "default" );
const char * ProtocolSpec::KW_MIN ( "min" );
const char * ProtocolSpec::KW_MAX ( "max" );
const char * ProtocolSpec::KW_MIN_LENGTH ( "min_length" );
const char * ProtocolSpec::KW_MAX_LENGTH ( "max_length" );
const char * ProtocolSpec::KW_MIN_LIST_SIZE ( "min_list_size" );
const char * ProtocolSpec::KW_MAX_LIST_SIZE ( "max_list_size" );
const char * ProtocolSpec::KW_DEFINED_AS_IN ( "as_in" );
const char * ProtocolSpec::KW_IMPORT ( "import" );
const char * ProtocolSpec::KW_PRAGMA ( "pragma" );
const char * ProtocolSpec::KW_UNIQ ( "unique" );
const char * ProtocolSpec::KW_DEFINED ( "defined" );
const char * ProtocolSpec::KW_ALIAS ( "alias" );
const char * ProtocolSpec::KW_SALIAS ( "salias" );
const char * ProtocolSpec::KW_NAMESPACE ( "namespace" );
const char * ProtocolSpec::KW_ENUM ( "enum" );
const char * ProtocolSpec::KW_MESSAGE ( "message" );
const char * ProtocolSpec::KW_STRUCT ( "struct" );
const char * ProtocolSpec::KW_TYPEDEF ( "typedef" );
const char * ProtocolSpec::KW_PUBLIC ( "public" );
const char * ProtocolSpec::KW_PROTECTED ( "protected" );
const char * ProtocolSpec::KW_PRIVATE ( "private" );
const char * ProtocolSpec::KW_OPTIONAL ( "optional" );
const char * ProtocolSpec::KW_REQUIRED ( "required" );
const char * ProtocolSpec::KW_REPEATED ( "repeated" );

ProtocolSpec::ProtocolSpec():
    _generate ( false ),
    _idScope ( IdScopeTree )
{
    _root = new Symbol ( *this );

    _root->createBasicRootType ( "bool", 1, false );
    _root->createBasicRootType ( "int8", 8, true );
    _root->createBasicRootType ( "uint8", 8, false );
    _root->createBasicRootType ( "int16", 16, true );
    _root->createBasicRootType ( "uint16", 16, false );
    _root->createBasicRootType ( "int32", 32, true );
    _root->createBasicRootType ( "uint32", 32, false );
    _root->createBasicRootType ( "int64", 64, true );
    _root->createBasicRootType ( "uint64", 64, false );
    _root->createBasicRootType ( "float", Symbol::SpecTypeFloatingPoint );
    _root->createBasicRootType ( "double", Symbol::SpecTypeFloatingPoint );

    validOptions.insert ( KW_DEFAULT );
    validOptions.insert ( KW_MIN );
    validOptions.insert ( KW_MAX );
    validOptions.insert ( KW_MIN_LIST_SIZE );
    validOptions.insert ( KW_MAX_LIST_SIZE );
    validOptions.insert ( KW_MIN_LENGTH );
    validOptions.insert ( KW_MAX_LENGTH );

    reservedNames.insert ( KW_DEFAULT );
    reservedNames.insert ( KW_DEFINED_AS_IN );
    reservedNames.insert ( KW_IMPORT );
    reservedNames.insert ( KW_PRAGMA );
    reservedNames.insert ( KW_UNIQ );
    reservedNames.insert ( KW_DEFINED );
    reservedNames.insert ( KW_ALIAS );
    reservedNames.insert ( KW_SALIAS );
    reservedNames.insert ( KW_NAMESPACE );
    reservedNames.insert ( KW_ENUM );
    reservedNames.insert ( KW_MESSAGE );
    reservedNames.insert ( KW_STRUCT );
    reservedNames.insert ( KW_TYPEDEF );
    reservedNames.insert ( KW_PUBLIC );
    reservedNames.insert ( KW_PROTECTED );
    reservedNames.insert ( KW_PRIVATE );
    reservedNames.insert ( KW_OPTIONAL );
    reservedNames.insert ( KW_REQUIRED );
    reservedNames.insert ( KW_REPEATED );

    for ( HashMap<String, Symbol *>::Iterator it ( _root->_internalSymbols ); it.isValid(); it.next() )
    {
        reservedNames.insert ( it.key() );
    }
}

ProtocolSpec::~ProtocolSpec()
{
    if ( _root != 0 )
    {
        delete _root;
        _root = 0;
    }
}

bool ProtocolSpec::setIdScope ( const String & scope )
{
    String val = scope.toLower();

    if ( val == "branch" )
    {
        _idScope = IdScopeBranch;
    }
    else if ( val == "tree" )
    {
        _idScope = IdScopeTree;
    }
    else if ( val == "global" )
    {
        _idScope = IdScopeGlobal;
    }
    else
    {
        return false;
    }

    return true;
}

void ProtocolSpec::appendToken ( const String & token, const String & lineComment )
{
    // Root node "wants" to be closed. This is an error (too many '}')!
    if ( _root->appendToken ( token, lineComment ) )
    {
        throw Error ( "Unexpected '}'" );
    }
}

void ProtocolSpec::checkGlobal()
{
    /// @todo anything?
}

bool ProtocolSpec::isClosed() const
{
    return ( !_root->_curIntSymbol );
}

bool ProtocolSpec::isGenerateMode() const
{
    return _generate;
}

void ProtocolSpec::setGenerateMode ( bool generate )
{
    _generate = generate;
}

void ProtocolSpec::setProtoFilePath ( const String & path )
{
    _protoFilePath = path;
}

const String & ProtocolSpec::getProtoFilePath() const
{
    return _protoFilePath;
}
