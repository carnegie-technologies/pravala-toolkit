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
#include "CppGen.hpp"

using namespace Pravala;

void CppGenerator::hookPosition (
        Symbol * /* symbol */, CppFile & /* hdrFile */,
        CppFile & /* implFile */, const CppGenerator::PositionType & /* position */ )
{
    // hdrFile.a ( "// Hook " ).a ( ( int ) position ).a ( "; Symbol: " ).a ( symbol->getPath() ).e();
    // implFile.a ( "// Hook " ).a ( ( int ) position ).a ( "; Symbol: " ).a ( symbol->getPath() ).e();
}

void CppGenerator::init()
{
    LanguageGenerator::init();

    HashSet<String> & rn = _proto.reservedNames;

    rn.insert ( "and" );
    rn.insert ( "and_eq" );
    rn.insert ( "asm" );
    rn.insert ( "assert" );
    rn.insert ( "auto" );
    rn.insert ( "bitand" );
    rn.insert ( "bitor" );
    rn.insert ( "bool" );
    rn.insert ( "break" );
    rn.insert ( "case" );
    rn.insert ( "catch" );
    rn.insert ( "char" );
    rn.insert ( "cin" );
    rn.insert ( "class" );
    rn.insert ( "compl" );
    rn.insert ( "const" );
    rn.insert ( "const_cast" );
    rn.insert ( "continue" );
    rn.insert ( "cout" );
    rn.insert ( "default" );
    rn.insert ( "define" );
    rn.insert ( "delete" );
    rn.insert ( "do" );
    rn.insert ( "double" );
    rn.insert ( "dynamic_cast" );
    rn.insert ( "else" );
    rn.insert ( "endl" );
    rn.insert ( "enum" );
    rn.insert ( "explicit" );
    rn.insert ( "extern" );
    rn.insert ( "false" );
    rn.insert ( "float" );
    rn.insert ( "for" );
    rn.insert ( "friend" );
    rn.insert ( "goto" );
    rn.insert ( "if" );
    rn.insert ( "include" );
    rn.insert ( "inline" );
    rn.insert ( "int" );
    rn.insert ( "INT_MAX" );
    rn.insert ( "INT_MIN" );
    rn.insert ( "iomanip" );
    rn.insert ( "iostream" );
    rn.insert ( "long" );
    rn.insert ( "main" );
    rn.insert ( "MAX_RAND" );
    rn.insert ( "mutable" );
    rn.insert ( "namespace" );
    rn.insert ( "new" );
    rn.insert ( "not" );
    rn.insert ( "not_eq" );
    rn.insert ( "npos" );
    rn.insert ( "null" );
    rn.insert ( "NULL" );
    rn.insert ( "operator" );
    rn.insert ( "or" );
    rn.insert ( "or_eq" );
    rn.insert ( "pragma" );
    rn.insert ( "private" );
    rn.insert ( "protected" );
    rn.insert ( "public" );
    rn.insert ( "register" );
    rn.insert ( "reinterpret_cast" );
    rn.insert ( "return" );
    rn.insert ( "short" );
    rn.insert ( "signed" );
    rn.insert ( "sizeof" );
    rn.insert ( "static" );
    rn.insert ( "static_cast" );
    rn.insert ( "std" );
    rn.insert ( "struct" );
    rn.insert ( "switch" );
    rn.insert ( "template" );
    rn.insert ( "this" );
    rn.insert ( "throw" );
    rn.insert ( "true" );
    rn.insert ( "try" );
    rn.insert ( "typedef" );
    rn.insert ( "typeid" );
    rn.insert ( "typename" );
    rn.insert ( "union" );
    rn.insert ( "unsigned" );
    rn.insert ( "using" );
    rn.insert ( "virtual" );
    rn.insert ( "void" );
    rn.insert ( "volatile" );
    rn.insert ( "wchar_t" );
    rn.insert ( "while" );
    rn.insert ( "xor" );
    rn.insert ( "xor_eq" );
}

CppGenerator::CppGenerator ( ProtocolSpec & proto ):
    LanguageGenerator ( proto ),
    _hdrExt ( "hpp" ),
    _implExt ( "cpp" ),
    _useProtoFileAsPrefix ( false ),
    _genDebugSymbols ( false ),
    _usePragmaOnce ( true ),
    _singleImplFile ( 0 )
{
}

String CppGenerator::getHelpText()
{
    String text
        = "    A generator that generates C++ code.\n"
          "    Options:\n"
          "      -f output_file, --output-file=output_file\n"
          "          Only a single implementation file will be generated (in the path provided).\n\n"
          "      --ext-header=extension\n"
          "          The extension to be used for generated header files.\n\n"
          "      --ext-impl=extension\n"
          "          The extension to be used for generated implementation files (if not in a single-file mode).\n\n"
          "      -d, --debug\n"
          "          If enabled, additional debugging code will be generated.\n\n"
          "      --use-proto-file-as-dir-prefix\n"
          "          If enabled, the base name of a protocol file will be added as a prefix directory\n"
          "          in a path for each file generated using that input file.\n"
          "          This does not apply to flag files or to implementation file when 'output-file' option is used.\n\n"
          "      --dir-prefix=dir\n"
          "          A directory that will be added at the beginning of generated file paths.\n"
          "          It will be used in front of the directory added by use-proto-file-as-dir-prefix option,\n"
          "          if they are both used at the same time.\n"
          "          This does not apply to flag files or to implementation file when 'output-file' option is used.\n\n"
          "      --no-pragma-once\n"
          "          If enabled, traditional ifndef/define/endif will be used in generated header files\n"
          "          instead of 'pragma once'.\n\n";

    text.append ( LanguageGenerator::getHelpText() );

    return text;
}

LanguageGenerator::SetOptResult CppGenerator::setOption (
        char shortName, const String & longName, const String & value )
{
    if ( shortName == 'f' || longName == "output-file" )
    {
        if ( value.isEmpty() )
            return OptErrMissingValue;

        _singleImplFilePath = value;
    }
    else if ( longName == "ext-header" )
    {
        if ( value.isEmpty() )
            return OptErrMissingValue;

        _hdrExt = value;
    }
    else if ( longName == "ext-impl" )
    {
        if ( value.isEmpty() )
            return OptErrMissingValue;

        _implExt = value;
    }
    else if ( longName == "dir-prefix" )
    {
        if ( value.isEmpty() )
            return OptErrMissingValue;

        _dirPrefix = value.simplified();

        while ( _dirPrefix.startsWith ( "/" ) )
        {
            _dirPrefix = _dirPrefix.substr ( 1 );
        }

        while ( _dirPrefix.endsWith ( "/" ) )
        {
            _dirPrefix = _dirPrefix.substr ( 0, _dirPrefix.length() - 1 );
        }
    }
    else if ( longName == "no-pragma-once" )
    {
        _usePragmaOnce = false;
        return OptOkValueIgnored;
    }
    else if ( shortName == 'd' || longName == "debug" )
    {
        _genDebugSymbols = true;
        return OptOkValueIgnored;
    }
    else if ( longName == "use-proto-file-as-dir-prefix" )
    {
        _useProtoFileAsPrefix = true;
        return OptOkValueIgnored;
    }
    else
    {
        return LanguageGenerator::setOption ( shortName, longName, value );
    }

    return OptOkValueConsumed;
}

void CppGenerator::run()
{
    if ( !_singleImplFilePath.isEmpty() )
    {
        _singleImplFile = new CppFile ( _singleImplFilePath, _singleIndent );

        addFile ( _singleImplFile );
    }

    LanguageGenerator::run();
}

String CppGenerator::getOutputFilePath ( FileObject * file )
{
    if ( _singleImplFile != 0 && file == _singleImplFile )
    {
        // If we are operating in a single-file mode, we don't want that file inside the output directory.
        return file->getPath();
    }

    return LanguageGenerator::getOutputFilePath ( file );
}

void CppGenerator::procRegularSymbol ( Symbol * s )
{
    assert ( s != 0 );
    assert ( !s->isNamespace() );

    // Do we want to generate output?
    if ( !s->isGenerated() )
        return;

    CppFile * impl = _singleImplFile;

    if ( !impl )
    {
        impl = new CppFile ( getFilePath ( s, ExtImpl ), _singleIndent );

        addFile ( impl );
    }

    CppFile * hdr = new CppFile ( getFilePath ( s, ExtHeader ), _singleIndent );

    addFile ( hdr );

    addDefaultIncludes ( s, *hdr, *impl );

    hdr->setHeaderSection();

    String defineGuard;

    if ( _usePragmaOnce )
    {
        hdr->ae ( "#pragma once" );
    }
    else
    {
        defineGuard = String ( "_AUTO_GENERATED_%1_H_" ).arg ( s->getPath().replace ( ".", "_" ) );

        hdr->ae ( String ( "#ifndef %1" ).arg ( defineGuard ) );
        hdr->ae ( String ( "#define %1" ).arg ( defineGuard ) );
    }

    hdr->e();

    hdr->setNamespaceSection();

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

    StringList prefNamespaces = _namespacePrefix.split ( "." );

    if ( prefNamespaces.size() > 0 )
    {
        for ( size_t i = 0; i < prefNamespaces.size(); ++i )
        {
            hdr->a ( "namespace " ).a ( prefNamespaces[ i ] ).e();
            hdr->a ( "{" ).e().e();
        }
    }

    if ( namespaces.size() > 0 )
    {
        assert ( namespaces.size() == nComments.size() );

        for ( size_t i = 0; i < namespaces.size(); ++i )
        {
            if ( nComments[ i ].length() > 0 )
            {
                hdr->c ( "@brief " ).a ( nComments[ i ] ).e();
            }

            hdr->a ( "namespace " ).a ( namespaces[ i ] ).e();
            hdr->a ( "{" ).e().e();
        }
    }

    genRegularSymbol ( s, *hdr, *impl );

    hdr->setFooterSection();

    genEnumHashGets ( *hdr );

    if ( namespaces.size() > 0 )
    {
        assert ( namespaces.size() == nComments.size() );

        // We close namespaces in reverse order!
        for ( size_t i = namespaces.size(); i > 0; --i )
        {
            hdr->e();
            hdr->a ( "} // namespace " ).a ( namespaces[ i - 1 ] ).e();
        }
    }

    if ( prefNamespaces.size() > 0 )
    {
        for ( size_t i = prefNamespaces.size(); i > 0; --i )
        {
            hdr->e();
            hdr->a ( "} // namespace " ).a ( prefNamespaces[ i - 1 ] ).e();
        }
    }

    hdr->e();

    if ( !defineGuard.isEmpty() )
    {
        hdr->ae ( String ( "#endif // %1" ).arg ( defineGuard ) ).e();
    }
}

void CppGenerator::genRegularSymbol ( Symbol * s, CppFile & hdr, CppFile & impl )
{
    hdr.setBodySection();
    impl.setBodySection();

    if ( s->isEnum() )
    {
        genEnumClass ( s, hdr, impl );
    }
    else if ( s->isMessageOrStruct() )
    {
        genMessageClass ( s, hdr, impl );
    }
    else
    {
        throw Error ( "Unknown symbol type of: ", getFilePath ( s ) );
    }
}

void CppGenerator::addDefaultIncludes ( Symbol * s, CppFile & hdr, CppFile & impl )
{
    assert ( s != 0 );

    impl.addCppInclude ( getFilePath ( s, ExtHeader, ( !_singleImplFile ) ? s : 0 ), CppFile::IncludeOwn );
    impl.addCppInclude ( "cassert", CppFile::IncludeGlobal );

    hdr.addCInclude ( "stdint.h", CppFile::IncludeGlobal );
}

String CppGenerator::getClassPath ( Symbol * symbol )
{
    if ( !symbol )
    {
        return String::EmptyString;
    }

    String path ( _namespacePrefix );

    if ( path.length() > 0 && !path.endsWith ( "." ) )
    {
        path.append ( "." );
    }

    path.append ( symbol->getPath() );

    // We want to remove multiple '.' signs.
    // It shouldn't happen so it doesn't really need to be efficient.
    while ( path.find ( ".." ) >= 0 )
    {
        path = path.replace ( "..", "." );
    }

    return path.replace ( ".", "::" );
}

void CppGenerator::genClassHeader ( Symbol * s, CppFile & hdr, CppFile & /* impl */ )
{
    assert ( s != 0 );

    // This doesn't work for enums!
    assert ( !s->isEnum() );
    assert ( s->isMessageOrStruct() );

    if ( s->getInheritance() != 0 )
    {
        hdr.a ( "class " ).a ( s->getName() ).a ( ": public " ).a ( getClassPath ( s->getInheritance() ) ).e();

        // We inherit some symbol which we may need to include.
        // However, if our oldest (most external) parent symbol and our inherited symbol's
        // oldest parent are the same (and not 0),
        // it means we are in the same file and there is no need to include anything.

        if ( !s->getOldestMessageOrStructParent()
             || s->getOldestMessageOrStructParent() != s->getInheritance()->getOldestMessageOrStructParent() )
        {
            // These two symbols have different parents, or at least one of them has no message parent at all.
            // We need this include:

            hdr.addCppInclude ( getFilePath ( s->getInheritance(), ExtHeader, s ), CppFile::IncludeProto );
        }
    }
    else
    {
        hdr.a ( "class " ).a ( s->getName() ).e();
    }
}

String CppGenerator::getFilePath ( Symbol * symbol, const CppGenerator::ExtType & extType, Symbol * fromSymbol )
{
    assert ( symbol != 0 );

    StringList dirs;
    String fileName;
    bool isGenerated = symbol->isGenerated();

    const String baseProtoName
        = ( _useProtoFileAsPrefix ) ? ( symbol->getProtoFileBaseName() ) : ( String::EmptyString );

    // We are doing this instead of just using getPath, because we only
    // include namespace components, and a single, first, message component.
    // All messages that are declared within another message share the same file
    // as that outside message.
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

    String path;

    if ( symbol == fromSymbol )
    {
        dirs.clear();
    }
    else
    {
        // And we want to add extra namespaces in the front
        StringList tmpList = _namespacePrefix.split ( "." );

        for ( size_t i = tmpList.size(); i > 0; --i )
        {
            dirs.prepend ( tmpList[ i - 1 ] );
        }

        path = String::join ( dirs, "/" );
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

        if ( !baseProtoName.isEmpty() )
        {
            path = String ( "%1/%2" ).arg ( baseProtoName, path );
        }

        if ( !_dirPrefix.isEmpty() )
        {
            path = String ( "%1/%2" ).arg ( _dirPrefix, path );
        }

        if ( fromSymbol != 0 && isGenerated )
        {
            StringList otherPath = getFilePath ( fromSymbol, extType ).split ( "/" );
            dirs = path.split ( "/" );

            // otherPath includes file name too!
            while ( dirs.size() > 0 && otherPath.size() > 1 && dirs[ 0 ] == otherPath[ 0 ] )
            {
                dirs.removeFirst();
                otherPath.removeFirst();
            }

            // -1 for the file name
            for ( size_t i = 0; i < otherPath.size() - 1; ++i )
            {
                dirs.prepend ( ".." );
            }

            path = String::join ( dirs, "/" );
            path.append ( "/" );
        }
    }

    path.append ( fileName );

    if ( extType == ExtHeader )
    {
        path.append ( String ( ".%1" ).arg ( _hdrExt ) );
    }
    else if ( extType == ExtImpl )
    {
        path.append ( String ( ".%1" ).arg ( _implExt ) );
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

int CppGenerator::getPresVarSize()
{
    return 32;
}

String CppGenerator::getPresVarType()
{
    return "uint32_t";
}

int CppGenerator::getPresVarShift ( int elemIndex )
{
    assert ( elemIndex >= 0 );

    return ( elemIndex % getPresVarSize() );
}

String CppGenerator::getPresVarNameNum ( int varNum )
{
    assert ( varNum >= 0 );

    String str ( "_pres_bits_" );

    str.append ( String::number ( varNum ) );

    return str;
}

String CppGenerator::getPresVarNameIdx ( int elemIndex )
{
    assert ( elemIndex >= 0 );
    return getPresVarNameNum ( elemIndex / getPresVarSize() );
}

String CppGenerator::getVarName ( Element * e )
{
    assert ( e != 0 );
    return String ( "_" ).append ( e->getCamelCaseName ( "val" ) );
}

String CppGenerator::getDefName ( Element * e )
{
    assert ( e != 0 );
    return String ( "DEF_" ).append ( e->name.toUpper() );
}

String CppGenerator::getFieldIdName ( Element * e )
{
    assert ( e != 0 );
    return String ( "FIELD_ID_" ).append ( e->name.toUpper() );
}

String CppGenerator::getVarType ( CppFile & hdr, Element * e, VarUseType useType )
{
    assert ( e != 0 );

    if ( e->aliasTarget != 0 )
    {
        assert ( !e->isRepeated() );

        int bitLength = e->getAliasPayloadBitLength();

        assert ( bitLength > 0 );

        if ( bitLength == 1 )
        {
            return "bool";
        }

        assert ( _proto.getRoot() != 0 );

        String type;

        if ( bitLength < 1 )
        {
            return type;
        }

        int minFound = 0x7FFFFFFF;

        for ( HashMap<String, Symbol * >::Iterator iter ( _proto.getRoot()->getInternalSymbols() );
              iter.isValid(); iter.next() )
        {
            Symbol * s = iter.value();

            assert ( s != 0 );

            int bLen = s->getBitLength();

            if ( s->canBeAliased() && bLen < minFound && bLen >= bitLength )
            {
                minFound = bLen;
                type = getRawVarType ( hdr, s );
            }
        }

        return type;
    }

    assert ( !e->aliasTarget );

    if ( e->isRepeated() )
    {
        return getListVarType ( hdr, e->typeSymbol, useType );
    }

    return getRawVarType ( hdr, e->typeSymbol, useType );
}

String CppGenerator::getRawVarType ( CppFile & /*hdr*/, Symbol * symbol, VarUseType /*useType*/ )
{
    assert ( symbol != 0 );

    if ( !symbol->isBasic() )
    {
        return getClassPath ( symbol );
    }

    const String & name = symbol->getName();

    if ( name == "double" || name == "float" )
    {
        return name;
    }

    bool ok = false;
    int bitLen = 0;

    if ( name.startsWith ( "int", false ) )
    {
        bitLen = name.substr ( 3 ).toInt32 ( &ok );
    }
    else if ( name.startsWith ( "uint", false ) )
    {
        bitLen = name.substr ( 4 ).toInt32 ( &ok );
    }

    if ( ok )
    {
        switch ( bitLen )
        {
            case 8:
            case 16:
            case 32:
            case 64:
                return String ( name ).append ( "_t" );
                break;
        }
    }

    return name;
}

String CppGenerator::getBitmask ( int numBits )
{
    assert ( numBits > 0 );

    int val = 0;

    while ( --numBits >= 0 )
    {
        val = ( val << 1 ) | 1;
    }

    // We add '0' in front of the one-digit values, because they look better (0x1 vs 0x01)
    // For larger ones it doesn't make much of a difference.
    return String ( "0x" ).append ( String::number ( val, String::Int_HEX, 2, true ) );
}
