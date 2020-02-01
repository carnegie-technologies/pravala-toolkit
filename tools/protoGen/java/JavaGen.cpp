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
#include "JavaGen.hpp"

using namespace Pravala;

void JavaGenerator::hookPosition (
        Symbol * /* symbol */,
        JavaFile & /* implFile */,
        const JavaGenerator::PositionType & /* position */ )
{
    // implFile.a ( "// Hook " ).a ( ( int ) position ).a ( "; Symbol: " ).a ( symbol->getPath() ).e();
}

void JavaGenerator::init()
{
    LanguageGenerator::init();

    HashSet<String> & rn = _proto.reservedNames;

    rn.insert ( "abstract" );
    rn.insert ( "assert" );
    rn.insert ( "boolean" );
    rn.insert ( "break" );
    rn.insert ( "byte" );
    rn.insert ( "case" );
    rn.insert ( "catch" );
    rn.insert ( "char" );
    rn.insert ( "class" );
    rn.insert ( "const" );
    rn.insert ( "continue" );
    rn.insert ( "default" );
    rn.insert ( "do" );
    rn.insert ( "double" );
    rn.insert ( "else" );
    rn.insert ( "enum" );
    rn.insert ( "extends" );
    rn.insert ( "final" );
    rn.insert ( "finally" );
    rn.insert ( "float" );
    rn.insert ( "for" );
    rn.insert ( "goto" );
    rn.insert ( "if" );
    rn.insert ( "implements" );
    rn.insert ( "import" );
    rn.insert ( "instanceof" );
    rn.insert ( "int" );
    rn.insert ( "interface" );
    rn.insert ( "long" );
    rn.insert ( "native" );
    rn.insert ( "new" );
    rn.insert ( "package" );
    rn.insert ( "private" );
    rn.insert ( "protected" );
    rn.insert ( "public" );
    rn.insert ( "return" );
    rn.insert ( "short" );
    rn.insert ( "static" );
    rn.insert ( "strictfp" );
    rn.insert ( "super" );
    rn.insert ( "switch" );
    rn.insert ( "synchronized" );
    rn.insert ( "this" );
    rn.insert ( "throw" );
    rn.insert ( "throws" );
    rn.insert ( "transient" );
    rn.insert ( "try" );
    rn.insert ( "void" );
    rn.insert ( "volatile" );
    rn.insert ( "while" );
}

JavaGenerator::JavaGenerator ( ProtocolSpec & proto ):
    LanguageGenerator ( proto ), _fileExt ( "java" )
{
}

void JavaGenerator::procRegularSymbol ( Symbol * s )
{
    assert ( s != 0 );
    assert ( !s->isNamespace() );

    // Do we want to generate output?
    if ( !s->isGenerated() )
        return;

    JavaFile * impl = new JavaFile ( getFilePath ( s, ExtImpl ), _singleIndent );

    addFile ( impl );

    addDefaultImports ( s, *impl );

    impl->setHeaderSection();
    impl->genAutogenComment();
    impl->setPackageSection();

    StringList namespaces;
    StringList nComments;

    Symbol * tmpSym = s->getParent();

    while ( tmpSym != 0 )
    {
        if ( tmpSym->isNamespace() && tmpSym->getName().length() > 0 )
        {
            namespaces.prepend ( tmpSym->getName() );
            nComments.prepend ( tmpSym->getComment() );
        }

        tmpSym = tmpSym->getParent();
    }

    StringList prefList = _namespacePrefix.split ( "." );

    for ( size_t i = prefList.size(); i > 0; --i )
    {
        if ( prefList[ i - 1 ].length() > 0 )
        {
            namespaces.prepend ( prefList[ i - 1 ] );
            nComments.prepend ( String::EmptyString );
        }
    }

    if ( namespaces.size() < 1 )
        throw Error ( "Java language generator requires at least one namespace to be used (for file: ",
                      getFilePath ( s ), ")" );

    impl->a ( "package " ).a ( String::join ( namespaces, "." ).toLower() ).a ( ";" ).e().e();

    genRegularSymbol ( s, *impl );

    impl->setHeaderSection();

    impl->genImports();

    impl->setFooterSection();
    impl->e();
}

void JavaGenerator::genRegularSymbol ( Symbol * s, JavaFile & impl, bool nested )
{
    impl.setBodySection();

    if ( s->isEnum() )
    {
        genEnumClass ( s, impl, nested );
    }
    else if ( s->isMessageOrStruct() )
    {
        genMessageClass ( s, impl, nested );
    }
    else
    {
        throw Error ( "Unknown symbol type of: ", getFilePath ( s ) );
    }
}

void JavaGenerator::addDefaultImports ( Symbol * s, JavaFile & /* impl */ )
{
    ( void ) s;
    assert ( s != 0 );

    // impl.addImport ( getFilePath ( s, ExtNoExt ) );
}

String JavaGenerator::getClassPath ( Symbol * symbol )
{
    if ( !symbol )
        return String::EmptyString;

    String path;
    String nextPath;

    while ( symbol != 0 && symbol->getName().length() > 0 )
    {
        nextPath = symbol->getName();

        if ( symbol->isNamespace() )
        {
            nextPath = nextPath.toLower();
        }

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

String JavaGenerator::getFilePath ( Symbol * symbol, const JavaGenerator::ExtType & extType )
{
    StringList dirs;
    String fileName;

    // We are doing this instead of just using getPath, because we only
    // include namespace components, and a single, first, message component.
    // All messages that are declared within another message share the same file
    // as that outside message.
    // Also, getPath uses original names of the namespaces, but for Java we need to make them lowercase!
    while ( symbol != 0 && symbol->getName().length() > 0 )
    {
        if ( symbol->isNamespace() )
        {
            dirs.prepend ( symbol->getName().toLower() );
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

String JavaGenerator::getExtends ( Symbol * s )
{
    if ( s != 0 && s->getInheritance() != 0 )
        return getClassPath ( s->getInheritance() );

    return "";
}

StringList JavaGenerator::getImplements ( Symbol * /* symbol */ )
{
    return StringList();
}

String JavaGenerator::getVarName ( Element * e )
{
    assert ( e != 0 );
    return String ( "_" ).append ( e->getCamelCaseName ( "val" ) );
}

String JavaGenerator::getDefName ( Element * e )
{
    assert ( e != 0 );
    return String ( "DEF_" ).append ( e->name.toUpper() );
}

String JavaGenerator::getFieldIdName ( Element * e )
{
    assert ( e != 0 );
    return String ( "FIELD_ID_" ).append ( e->name.toUpper() );
}

Symbol * JavaGenerator::getEnumStorageSymbol()
{
    Symbol * root = _proto.getRoot();

    if ( !root )
        throw Error ( "Java language generator can not find the root of symbol space" );

    Symbol * enumSym = root->getInternalSymbols().value ( "int32" );

    if ( !enumSym )
        throw Error ( "Java language generator can not find symbol 'int32' to use for the internal enum's value" );

    return enumSym;
}

Symbol * JavaGenerator::getAliasStorageSymbol ( int numBits )
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
        throw Error ( "Java language generator can not find storage symbol for an alias with ",
                      String::number ( numBits ), " bits" );

    return ret;
}

String JavaGenerator::getIfaceType ( Element * e )
{
    assert ( e != 0 );

    if ( e->isRepeated() )
        return getListIfaceType ( e->typeSymbol );

    return getVarType ( e );
}

String JavaGenerator::getVarType ( Element * e )
{
    assert ( e != 0 );

    if ( e->aliasTarget != 0 )
    {
        assert ( !e->isRepeated() );

        int bitLength = e->getAliasPayloadBitLength();

        assert ( bitLength > 0 );

        if ( bitLength == 1 )
            return "Boolean";

        Symbol * s = getAliasStorageSymbol ( bitLength );

        assert ( s != 0 );

        return getRawVarType ( s );
    }

    assert ( !e->aliasTarget );

    if ( e->isRepeated() )
        return getListVarType ( e->typeSymbol );

    return getRawVarType ( e->typeSymbol );
}

String JavaGenerator::getVarLiteral ( Element * e, String value )
{
    if ( !e )
    {
        return value;
    }

    Symbol * s = ( e->aliasTarget != 0 ) ? e->aliasTarget->typeSymbol : e->typeSymbol;

    if ( s != 0 )
    {
        const String javaType = getRawVarType ( s );

        if ( javaType == "Float" )
        {
            return String ( "%1F" ).arg ( value );
        }
        else if ( javaType == "Double" )
        {
            return String ( "%1D" ).arg ( value );
        }
        else if ( javaType == "Long" )
        {
            // Only long / Long integer literals require the L suffix, all other integers (including byte/short)
            // don't require any type of suffix.
            return String ( "%1L" ).arg ( value );
        }
    }

    return value;
}

String JavaGenerator::getBitmask ( int numBits )
{
    assert ( numBits > 0 );

    int val = 0;

    while ( --numBits >= 0 )
    {
        val = ( val << 1 ) | 1;
    }

    // We add '0' in front of the one-digit values, because they look better (0x1 vs 0x01)
    // For larger ones it doesn't make much difference.
    return String ( "0x" ).append ( String::number ( val, String::Int_HEX, 2, true ) );
}
