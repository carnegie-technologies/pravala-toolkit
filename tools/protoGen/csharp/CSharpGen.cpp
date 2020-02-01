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

#include "../Error.hpp"
#include "CSharpGen.hpp"

#define PROTO_PACKAGE    "Pravala.Protocol"
#define ERRCODE_CLASS    "Pravala.Protocol.ProtoException.ErrorCodes"

using namespace Pravala;

CSharpGenerator::StdTypes::StdTypes():
    ReadBuffer ( "byte[]" ),
    FieldId ( "uint" ),
    ReadOffset ( "uint" ),
    ReadPayloadSize ( "uint" ),
    Enum ( "int" ),
    WireType ( PROTO_PACKAGE ".Codec.WireTypes" ),
    Exception ( PROTO_PACKAGE ".ProtoException" ),
    IpAddress ( PROTO_PACKAGE ".IpAddress" ),
    WriteBuffer ( PROTO_PACKAGE ".Buffer" ),
    Serializable ( PROTO_PACKAGE ".Serializable" ),
    BaseSerializable ( PROTO_PACKAGE ".BaseSerializable" )
{
}

CSharpGenerator::SpecialWireTypes::SpecialWireTypes():
    VarLenA ( PROTO_PACKAGE ".Codec.WireTypes.TypeVariableLengthA" ),
    VarLenB ( PROTO_PACKAGE ".Codec.WireTypes.TypeVariableLengthB" )
{
}

CSharpGenerator::StdMethods::StdMethods():
    Encode ( PROTO_PACKAGE ".Codec.AppendValue" ),
    Decode ( PROTO_PACKAGE ".Codec.Decode" ),
    Clear ( "ClearMessage" ),
    Validate ( "ValidateMessage" ),
    SetupDefines ( "SetupMessageDefines" ),
    SerializeFields ( "SerializeMessageFields" ),
    SerializeMessage ( "SerializeMessageData" ),
    DeserializeField ( "DeserializeMessageField" ),
    DeserializeMessage ( "DeserializeMessageData" ),
    DeserializeFromBase ( "DeserializeFromBaseMessage" )
{
}

CSharpGenerator::StdErrors::StdErrors():
    OK ( ERRCODE_CLASS ".Success" ),
    InvalidParam ( ERRCODE_CLASS ".InvalidParameter" ),
    InvalidData ( ERRCODE_CLASS ".InvalidData" ),
    RequiredFieldNotSet ( ERRCODE_CLASS ".RequiredFieldNotSet" ),
    FieldValueOutOfRange ( ERRCODE_CLASS ".FieldValueOutOfRange" ),
    StringLengthOutOfRange ( ERRCODE_CLASS ".StringLengthOutOfRange" ),
    ListSizeOutOfRange ( ERRCODE_CLASS ".ListSizeOutOfRange" ),
    DefinedValueMismatch ( ERRCODE_CLASS ".DefinedValueMismatch" ),
    ProtocolError ( ERRCODE_CLASS ".ProtocolError" )
{
}

String CSharpGenerator::getHelpText()
{
    String text
        = "    A generator that generates C# code.\n"
          "    Options:\n"
          "      -f output_file, --output-file=output_file\n"
          "          All of the code will be generated in a file created in the path provided.\n\n";

    text.append ( LanguageGenerator::getHelpText() );

    return text;
}

LanguageGenerator::SetOptResult CSharpGenerator::setOption (
        char shortName, const String & longName, const String & value )
{
    if ( shortName == 'f' || longName == "output-file" )
    {
        if ( value.isEmpty() )
            return OptErrMissingValue;

        _singleImplFilePath = value;
    }
    else
    {
        return LanguageGenerator::setOption ( shortName, longName, value );
    }

    return OptOkValueConsumed;
}

void CSharpGenerator::run()
{
    if ( _singleImplFilePath.isEmpty() )
    {
        LanguageGenerator::run();
        return;
    }

    procSymbol ( _proto.getRoot() );

    if ( !_singleImplFile )
        throw Error ( "No output to generate!" );

    writeFile ( _singleImplFilePath, _singleImplFile );

    generateFlagFiles();
}

void CSharpGenerator::hookPosition (
        Symbol * /* symbol */,
        CSharpFile & /* implFile */,
        const CSharpGenerator::PositionType & /* position */ )
{
    // implFile.a ( "// Hook " ).a ( ( int ) position ).a ( "; Symbol: " ).a ( symbol->getPath() ).e();
}

void CSharpGenerator::init()
{
    LanguageGenerator::init();

    HashSet<String> & rn = _proto.reservedNames;

    rn.insert ( "abstract" );
    rn.insert ( "as" );
    rn.insert ( "base" );
    rn.insert ( "bool" );
    rn.insert ( "break" );
    rn.insert ( "byte" );
    rn.insert ( "case" );
    rn.insert ( "catch" );
    rn.insert ( "char" );
    rn.insert ( "checked" );
    rn.insert ( "class" );
    rn.insert ( "const" );
    rn.insert ( "continue" );
    rn.insert ( "decimal" );
    rn.insert ( "default" );
    rn.insert ( "delegate" );
    rn.insert ( "do" );
    rn.insert ( "double" );
    rn.insert ( "else" );
    rn.insert ( "enum" );
    rn.insert ( "event" );
    rn.insert ( "explicit" );
    rn.insert ( "extern" );
    rn.insert ( "false" );
    rn.insert ( "finally" );
    rn.insert ( "fixed" );
    rn.insert ( "float" );
    rn.insert ( "for" );
    rn.insert ( "foreach" );
    rn.insert ( "goto" );
    rn.insert ( "if" );
    rn.insert ( "implicit" );
    rn.insert ( "in" );
    rn.insert ( "int" );
    rn.insert ( "interface" );
    rn.insert ( "internal" );
    rn.insert ( "is" );
    rn.insert ( "lock" );
    rn.insert ( "long" );
    rn.insert ( "namespace" );
    rn.insert ( "new" );
    rn.insert ( "null" );
    rn.insert ( "object" );
    rn.insert ( "operator" );
    rn.insert ( "out" );
    rn.insert ( "override" );
    rn.insert ( "params" );
    rn.insert ( "private" );
    rn.insert ( "protected" );
    rn.insert ( "public" );
    rn.insert ( "readonly" );
    rn.insert ( "ref" );
    rn.insert ( "return" );
    rn.insert ( "sbyte" );
    rn.insert ( "sealed" );
    rn.insert ( "short" );
    rn.insert ( "sizeof" );
    rn.insert ( "stackalloc" );
    rn.insert ( "static" );
    rn.insert ( "string" );
    rn.insert ( "struct" );
    rn.insert ( "switch" );
    rn.insert ( "this" );
    rn.insert ( "throw" );
    rn.insert ( "true" );
    rn.insert ( "try" );
    rn.insert ( "typeof" );
    rn.insert ( "uint" );
    rn.insert ( "ulong" );
    rn.insert ( "unchecked" );
    rn.insert ( "unsafe" );
    rn.insert ( "ushort" );
    rn.insert ( "using" );
    rn.insert ( "virtual" );
    rn.insert ( "void" );
    rn.insert ( "volatile" );
    rn.insert ( "while" );

    rn.insert ( _method.Clear );
    rn.insert ( _method.Validate );
    rn.insert ( _method.SetupDefines );
    rn.insert ( _method.SerializeFields );
    rn.insert ( _method.SerializeMessage );
    rn.insert ( _method.DeserializeField );
    rn.insert ( _method.DeserializeMessage );
    rn.insert ( _method.DeserializeFromBase );
}

CSharpGenerator::CSharpGenerator ( ProtocolSpec & proto ):
    LanguageGenerator ( proto ), _fileExt ( "cs" ), _singleImplFile ( 0 ), _symString ( 0 ), _symIpAddr ( 0 )
{
    _symString = _proto.getRoot()->createBasicRootType ( "string", Symbol::SpecTypeString );
    _symIpAddr = _proto.getRoot()->createBasicRootType ( "ip_addr" );
}

void CSharpGenerator::procRegularSymbol ( Symbol * s )
{
    assert ( s != 0 );
    assert ( !s->isNamespace() );

    // Do we want to generate output?
    if ( !s->isGenerated() )
        return;

    CSharpFile * impl = _singleImplFile;

    if ( !impl )
    {
        String path = _singleImplFilePath;

        if ( path.isEmpty() )
            path = getFilePath ( s, ExtImpl );

        impl = new CSharpFile ( path, _singleIndent );

        impl->setHeaderSection();
        impl->genAutogenComment();
        impl->e();

        addFile ( impl );

        addDefaultImports ( s, *impl );
    }

    assert ( impl != 0 );

    impl->setBodySection();

    if ( !_singleImplFile && !_singleImplFilePath.isEmpty() )
        _singleImplFile = impl;

    StringList namespaces;

    Symbol * tmpSym = s->getParent();

    while ( tmpSym != 0 )
    {
        if ( tmpSym->isNamespace() && tmpSym->getName().length() > 0 )
        {
            namespaces.prepend ( tmpSym->getName() );
        }

        tmpSym = tmpSym->getParent();
    }

    StringList prefList = _namespacePrefix.split ( "." );

    for ( size_t i = prefList.size(); i > 0; --i )
    {
        if ( prefList[ i - 1 ].length() > 0 )
        {
            namespaces.prepend ( prefList[ i - 1 ] );
        }
    }

    if ( namespaces.size() < 1 )
        throw Error ( "CSharp language generator requires at least one namespace to be used (for file: ",
                      getFilePath ( s ), ")" );

    impl->a ( "namespace " ).a ( String::join ( namespaces, "." ) ).e().a ( "{" ).e();

    genRegularSymbol ( s, *impl );

    impl->e().a ( "}" ).e().e();
}

void CSharpGenerator::genRegularSymbol ( Symbol * s, CSharpFile & impl )
{
    impl.setBodySection();

    if ( s->isEnum() )
    {
        genEnumClass ( s, impl );
    }
    else if ( s->isMessageOrStruct() )
    {
        genMessageClass ( s, impl );
    }
    else
    {
        throw Error ( "Unknown symbol type of: ", getFilePath ( s ) );
    }
}

void CSharpGenerator::addDefaultImports ( Symbol * s, CSharpFile & impl )
{
    ( void ) s;
    assert ( s != 0 );

    impl.addImport ( "System" );

    // impl.addImport ( getFilePath ( s, ExtNoExt ) );
}

String CSharpGenerator::getClassPath ( Symbol * symbol )
{
    if ( !symbol )
        return String::EmptyString;

    String path;
    String nextPath;

    while ( symbol != 0 && symbol->getName().length() > 0 )
    {
        nextPath = symbol->getName();

        if ( path.length() > 0 )
        {
            nextPath.append ( "." );
            nextPath.append ( path );
        }

        path = nextPath;

        symbol = symbol->getParent();
    }

    nextPath = _namespacePrefix;

    if ( nextPath.length() > 0 )
        nextPath.append ( "." );

    nextPath.append ( path );
    path = nextPath;

    // We want to remove multiple '.' signs.
    // It shouldn't happen so it doesn't really need to be efficient.
    while ( path.find ( ".." ) >= 0 )
    {
        path = path.replace ( "..", "." );
    }

    return path;
}

String CSharpGenerator::getFilePath ( Symbol * symbol, const CSharpGenerator::ExtType & extType )
{
    StringList dirs;
    String fileName;

    // We are doing this instead of just using getPath, because we only
    // include namespace components, and a single, first, message component.
    // All messages that are declared within another message share the same file
    // as that outside message.
    // Also, getPath uses original names of the namespaces, but for CSharp we need to make them lowercase!
    while ( symbol != 0 && symbol->getName().length() > 0 )
    {
        if ( symbol->isNamespace() )
        {
            dirs.prepend ( symbol->getName() );
        }
        else
        {
            fileName = symbol->getName();
        }

        symbol = symbol->getParent();
    }

    // And we want to add extra namespaces in the front
    StringList tmpList = _namespacePrefix.split ( "." );

    for ( size_t i = tmpList.size(); i > 0; --i )
    {
        dirs.prepend ( tmpList[ i - 1 ] );
    }

    String path = String::join ( dirs, "/" );
    path.append ( "/" );

    for ( size_t i = 0; i < _skipLeadingDirs.size(); ++i )
    {
        if ( path.startsWith ( _skipLeadingDirs[ i ] ) )
        {
            path = path.substr ( _skipLeadingDirs[ i ].length() );

            // We break, once we find a matching entry we don't try again
            break;
        }
    }

    path.append ( fileName );

    if ( extType == ExtImpl )
    {
        path.append ( String ( ".%1" ).arg ( _fileExt ) );
    }

    // Cleanup if something is weird.
    while ( path.startsWith ( "/" ) )
    {
        path = path.substr ( 1 );
    }

    while ( path.find ( "//" ) >= 0 )
    {
        path = path.replace ( "//", "/" );
    }

    return path;
}

String CSharpGenerator::getExtends ( Symbol * s )
{
    if ( s != 0 )
    {
        if ( s->isBaseMessage() )
        {
            assert ( !s->getInheritance() );

            return _type.BaseSerializable;
        }
        else if ( s->isMessageOrStruct() )
        {
            if ( s->getInheritance() != 0 )
            {
                return getClassPath ( s->getInheritance() );
            }
            else
            {
                return _type.Serializable;
            }
        }
    }

    return "";
}

String CSharpGenerator::getGetName ( const Element * e, bool includeThis )
{
    assert ( e != 0 );

    String ret ( includeThis ? "this." : "" );

    ret.append ( e->getCamelCaseName ( "get" ) );
    ret.append ( "()" );

    return ret;
}

String CSharpGenerator::getSetName ( const Element * e, bool includeThis )
{
    assert ( e != 0 );

    String ret ( includeThis ? "this." : "" );

    ret.append ( e->getCamelCaseName ( "set" ) );

    return ret;
}

String CSharpGenerator::getVarName ( const Element * e, bool includeThis )
{
    assert ( e != 0 );

    if ( e->definedTarget != 0 )
        return getVarName ( e->definedTarget );

    if ( e->aliasTarget != 0 )
        throw Error ( "Aliases don't have their own storage variables!" );

    String ret ( includeThis ? "this." : "" );

    ret.append ( "value" );
    ret.append ( e->getCamelCaseName ( "", true ) );

    return ret;
}

String CSharpGenerator::getFieldName ( const Element * e )
{
    assert ( e != 0 );

    return e->getCamelCaseName ( "", true );
}

String CSharpGenerator::getDefName ( const Element * e )
{
    assert ( e != 0 );

    return e->getCamelCaseName ( "def" );
}

String CSharpGenerator::getHasName ( const Element * e, bool includeThis )
{
    assert ( e != 0 );

    String ret ( includeThis ? "this." : "" );

    ret.append ( e->getCamelCaseName ( "has" ) );
    ret.append ( "()" );

    return ret;
}

Symbol * CSharpGenerator::getEnumStorageSymbol()
{
    Symbol * root = _proto.getRoot();

    if ( !root )
        throw Error ( "CSharp language generator cannot find the root of symbol space" );

    Symbol * enumSym = root->getInternalSymbols().value ( "int32" );

    if ( !enumSym )
        throw Error ( "CSharp language generator cannot find symbol 'int32' to use for the internal enum's value" );

    return enumSym;
}

Symbol * CSharpGenerator::getAliasStorageSymbol ( int numBits )
{
    assert ( _proto.getRoot() != 0 );

    Symbol * ret = 0;

    if ( numBits > 1 )
    {
        int minFound = 0x7FFFFFFF;

        for ( HashMap<String, Symbol * >::Iterator iter ( _proto.getRoot()->getInternalSymbols() );
              iter.isValid(); iter.next() )
        {
            Symbol * s = iter.value();

            assert ( s != 0 );

            int bLen = s->getBitLength();

            if ( s->canBeAliased() && bLen < minFound && bLen >= numBits )
            {
                minFound = bLen;
                ret = s;
            }
        }
    }

    if ( !ret )
        throw Error ( "CSharp language generator can not find storage symbol for an alias with ",
                      String::number ( numBits ), " bits" );

    return ret;
}

String CSharpGenerator::getVarType ( const Element * e, bool useNullable )
{
    assert ( e != 0 );

    if ( e->aliasTarget != 0 )
    {
        assert ( !e->isRepeated() );

        int bitLength = e->getAliasPayloadBitLength();

        assert ( bitLength > 0 );

        if ( bitLength == 1 )
            return ( useNullable ? "bool?" : "bool" );

        Symbol * s = getAliasStorageSymbol ( bitLength );

        assert ( s != 0 );

        return getRawVarType ( s, useNullable );
    }

    assert ( !e->aliasTarget );

    if ( e->isRepeated() )
    {
        // We always prefer non-nullable types inside lists:
        return String ( "System.Collections.Generic.List<%1>" ).arg ( getRawVarType ( e->typeSymbol, false ) );
    }

    return getRawVarType ( e->typeSymbol, useNullable );
}

String CSharpGenerator::getRawVarType ( Symbol * symbol, bool useNullable )
{
    assert ( symbol != 0 );

    const String & name = symbol->getName();

    if ( symbol == _symString
         || symbol == _symIpAddr
         || ( !symbol->isBasic() && !symbol->isEnum() ) )
    {
        // Ignore 'use nullable'

        if ( symbol == _symString )
            return "string";

        if ( symbol == _symIpAddr )
            return _type.IpAddress;

        return getClassPath ( symbol );
    }

    String ret;

    if ( symbol->isEnum() )
    {
        ret = getClassPath ( symbol );

        if ( useNullable )
            ret.append ( "?" );

        return ret;
    }

    if ( name == "bool" || name == "double" || name == "float" )
    {
        ret = name;

        if ( useNullable )
            ret.append ( "?" );

        return ret;
    }

    bool ok = false;
    int numBits = 0;
    bool isSigned = false;

    if ( name.startsWith ( "int", false ) )
    {
        numBits = name.substr ( 3 ).toInt32 ( &ok );
        isSigned = true;
    }
    else if ( name.startsWith ( "uint", false ) )
    {
        numBits = name.substr ( 4 ).toInt32 ( &ok );
    }

    if ( ok )
        ret = getNumericType ( numBits, isSigned, useNullable );

    if ( ret.isEmpty() )
        throw Error ( "CSharp language generator does not support '", name, "' type" );

    return ret;
}

String CSharpGenerator::getNumericType ( int numBits, bool isSigned, bool useNullable )
{
    String val;

    switch ( numBits )
    {
        case 1:
            val = "bool";
            break;

        case 8:
            val = ( isSigned ? "sbyte" : "byte" );
            break;

        case 16:
            val = ( isSigned ? "short" : "ushort" );
            break;

        case 32:
            val = ( isSigned ? "int" : "uint" );
            break;

        case 64:
            val = ( isSigned ? "long" : "ulong" );
            break;

        default:
            return "";
            break;
    }

    if ( useNullable )
        val.append ( "?" );

    return val;
}

bool CSharpGenerator::isNullable ( Symbol * symbol )
{
    assert ( symbol != 0 );

    if ( !symbol )
        return false;

    return ( symbol == _symIpAddr || symbol == _symString || symbol->isMessageOrStruct() );
}

String CSharpGenerator::getBitmask ( int numBits )
{
    assert ( numBits > 0 );

    uint64_t val = 0;

    int n = numBits;

    while ( --n >= 0 )
    {
        val = ( val << 1 ) | 1;
    }

    // We add '0' in front of the one-digit values, because they look better (0x1 vs 0x01)
    // For larger ones it doesn't make much difference.
    String str = String ( "0x" ).append ( String::number ( val, String::Int_HEX, 2, true ) ).append ( "U" );

    if ( numBits > 32 )
        str.append ( "L" );

    return str;
}
