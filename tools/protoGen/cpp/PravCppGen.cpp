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

#include "PravCppGen.hpp"
#include "../Error.hpp"

using namespace Pravala;

PravalaCppGenerator::PravalaCppGenerator ( ProtocolSpec & proto ):
    CppGenerator ( proto ),
    _symString ( _proto.getRoot()->createBasicRootType ( "string", Symbol::SpecTypeString ) ),
    _symIpAddr ( _proto.getRoot()->createBasicRootType ( "ip_addr" ) ),
    _symTimestamp ( _proto.getRoot()->createBasicRootType ( "timestamp" ) ),
    _enableJson ( false )
{
}

String PravalaCppGenerator::getHelpText()
{
    String text = CppGenerator::getHelpText();

    text.append ( "      --enable-json\n"
                  "          If enabled, JSON serializer will be generated as well.\n\n" );

    return text;
}

LanguageGenerator::SetOptResult PravalaCppGenerator::setOption (
        char shortName, const String & longName, const String & value )
{
    if ( longName == "enable-json" )
    {
        _enableJson = true;
        return OptOkValueIgnored;
    }

    return CppGenerator::setOption ( shortName, longName, value );
}

void PravalaCppGenerator::addDefaultIncludes ( Symbol * s, CppFile & hdrFile, CppFile & implFile )
{
    CppGenerator::addDefaultIncludes ( s, hdrFile, implFile );

    hdrFile.addCppInclude ( "basic/MemHandle.hpp", CppFile::IncludeLocal );
    hdrFile.addCppInclude ( "proto/ProtoError.hpp", CppFile::IncludeLocal );

    implFile.addCppInclude ( "proto/ExtProtoError.hpp", CppFile::IncludeLocal );
    implFile.addCppInclude ( "proto/ProtocolCodec.hpp", CppFile::IncludeLocal );
}

String PravalaCppGenerator::getErrorCode ( const CppGenerator::ErrorCode & errCode )
{
    switch ( errCode )
    {
        case ErrOK:
            return "ProtoError::Success";
            break;

        case ErrRequiredFieldNotSet:
            return "ProtoError::RequiredFieldNotSet";
            break;

        case ErrFieldValueOutOfRange:
            return "ProtoError::FieldValueOutOfRange";
            break;

        case ErrStringLengthOutOfRange:
            return "ProtoError::StringLengthOutOfRange";
            break;

        case ErrListSizeOutOfRange:
            return "ProtoError::ListSizeOutOfRange";
            break;

        case ErrDefinedValueMismatch:
            return "ProtoError::DefinedValueMismatch";
            break;

        case ErrProtocolWarning:
            return "ProtoError::ProtocolWarning";
            break;

        case ErrProtocolError:
            return "ProtoError::ProtocolError";
            break;
    }

    throw Error ( "Pravala C++ generator does not support error code '",
                  String::number ( errCode ), "'" );
}

String PravalaCppGenerator::getStdType ( const CppGenerator::StdType & stdType )
{
    switch ( stdType )
    {
        case TypeErrorCode:
            return "Pravala::ProtoError";
            break;

        case TypeExtError:
            return "Pravala::ExtProtoError";
            break;

        case TypeReadBuffer:
            return "const MemHandle &";
            break;

        case TypeWriteBuffer:
            return "Pravala::Buffer";
            break;

        case TypeFieldId:
            return "uint32_t";
            break;

        case TypeWireType:
            return "uint8_t";
            break;

        case TypeEnum:
            return "int32_t";
            break;
    }

    throw Error ( "Pravala C++ generator does not support '",
                  String::number ( stdType ), "' data type" );
}

String PravalaCppGenerator::getListVarType ( CppFile & hdr, Symbol * intSymbol, VarUseType /*useType*/ )
{
    hdr.addCppInclude ( "basic/List.hpp", CppFile::IncludeLocal );

    // We always pass 'storage' if wrapping things in lists.
    // This is because we can't cast from List<Container<T> > to List<T>.

    return String ( "Pravala::List<%1>" )
           .arg ( getRawVarType ( hdr, intSymbol, VarUseStorage ) )
           .replace ( ">>", "> >" );
}

String PravalaCppGenerator::getRawVarType ( CppFile & hdr, Symbol * symbol, VarUseType useType )
{
    if ( symbol == _symString )
    {
        hdr.addCppInclude ( "basic/String.hpp", CppFile::IncludeLocal );

        return "Pravala::String";
    }

    if ( symbol == _symIpAddr )
    {
        hdr.addCppInclude ( "basic/IpAddress.hpp", CppFile::IncludeLocal );

        return "Pravala::IpAddress";
    }

    if ( symbol == _symTimestamp )
    {
        hdr.addCppInclude ( "basic/Timestamp.hpp", CppFile::IncludeLocal );

        return "Pravala::Timestamp";
    }

    if ( symbol != 0 && symbol->isMessage() )
    {
        if ( !symbol->isBaseMessage() )
        {
            throw Error ( "Pravala C++ generator does not support storing messages using their non-base type: '",
                          symbol->getPath(), "'" );
        }

        if ( useType == VarUseStorage )
        {
            // We "wrap" the messages in the container only for storage.
            // Otherwise we want to use the base class, to generate methods like
            // 'const T & getT()' instead of 'const Container<T> & getT()'
            // and 'setT ( const T & t )' instead of 'setT ( const Container<T> & t )'.
            // They are easier to use, and don't require additional code since the Container
            // provides needed assignment and casting operators.

            hdr.addCppInclude ( "proto/SerializableContainer.hpp", CppFile::IncludeLocal );

            return String ( "Pravala::SerializableContainer<%1>" )
                   .arg ( CppGenerator::getRawVarType ( hdr, symbol, useType ) );
        }
    }

    return CppGenerator::getRawVarType ( hdr, symbol, useType );
}

String PravalaCppGenerator::exprVarLenWireTypeCheck ( const String & wireTypeVarName )
{
    return String ( "((%1) == Pravala::ProtocolCodec::WireTypeVariableLengthA "
                    "|| (%1) == Pravala::ProtocolCodec::WireTypeVariableLengthB)" )
           .arg ( wireTypeVarName );
}

String PravalaCppGenerator::exprStringVarLength ( const String & strVarName )
{
    return String ( "%1.length()" ).arg ( strVarName );
}

String PravalaCppGenerator::exprListVarSize (
        Symbol * /* intSymbol */,
        const String & varName )
{
    return String ( "%1.size()" ).arg ( varName );
}

String PravalaCppGenerator::exprListAppend (
        Symbol * /* intSymbol */,
        const String & listVarName,
        const String & appendVarName )
{
    return String ( "%1.append ( %2 )" ).arg ( listVarName, appendVarName );
}

String PravalaCppGenerator::exprListGetElemIdxRef (
        Symbol * /* intSymbol */,
        const String & listVarName,
        const String & indexVarName )
{
    return String ( "%1[%2]" ).arg ( listVarName, indexVarName );
}

String PravalaCppGenerator::exprProtoEncode (
        const String & bufVarName,
        const String & valueVarName,
        const String & valueCode )
{
    return String ( "Pravala::ProtocolCodec::encode ( %1, %2, %3 )" )
           .arg ( bufVarName, valueVarName, valueCode );
}

String PravalaCppGenerator::exprProtoDecodeFieldValue (
        const String & bufVarName, const String & offset,
        const String & fieldSize, const String & wireType, const String & fieldVarName )
{
    return String ( "Pravala::ProtocolCodec::decode ( %1.get ( %2 ), %3, %4, %5 )" )
           .arg ( bufVarName, offset, fieldSize, wireType, fieldVarName );
}

String PravalaCppGenerator::exprVarClear ( Element * e )
{
    const String ret = CppGenerator::exprVarClear ( e );

    if ( !ret.isEmpty() )
        return ret;

    assert ( e != 0 );
    assert ( e->typeSymbol != 0 );

    // Aliases can't have 'clear()' function
    assert ( !e->aliasTarget );

    return String ( "%1.clear()" ).arg ( getVarName ( e ) );
}

void PravalaCppGenerator::genSetupExtError (
        CppFile & file, int ind,
        const String & errCode, const String & errMessage,
        bool addPtrCheck, const String & extErrorPtrName )
{
    if ( addPtrCheck )
        file.ae ( ind, String ( "if ( %1 != 0 )" ).arg ( extErrorPtrName ) );

    file.ae ( ind, "{" );

    file.ae ( ind + 1, String ( "%1->add ( %2%3%4 );" )
              .arg ( extErrorPtrName,
                     errCode,
                     errMessage.isEmpty() ? "" : ", ",
                     errMessage ) );

    file.ae ( ind, "}" );
}

void PravalaCppGenerator::genDeserializeMessage (
        CppFile & file, int indent, Symbol * /*s*/,
        const String & varName, const String & bufVarName,
        const String & fieldSizeExpr, const String & offsetVarName,
        const String & resultVarName, const String & extErrVarName )
{
    file.ae ( indent, String ( "%1 = %2.deserialize ( %3, %4, %5, %6 );" )
              .arg ( resultVarName,
                     varName,
                     bufVarName,
                     offsetVarName,
                     fieldSizeExpr,
                     extErrVarName ) );
}

void PravalaCppGenerator::genSerializeMessage (
        CppFile & file, int ind, Symbol * /*s*/,
        const String & varName, const String & bufVarName,
        const String & resultVarName, const String & extErrVarName )
{
    file.ae ( ind, String ( "%1 = %2.serialize ( %3, %4 );" )
              .arg ( resultVarName, varName, bufVarName, extErrVarName ) );
}

void PravalaCppGenerator::genObjectModified ( Symbol * s, CppFile & file, int indent )
{
    if ( !s || !s->isMessage() )
        return;

    file.e();
    file.i ( indent ).ae ( "messageModified();" ).e();
}

void PravalaCppGenerator::hookPosition (
        Symbol * s, CppFile & hdr, CppFile & impl,
        const PositionType & position )
{
    CppGenerator::hookPosition ( s, hdr, impl, position );

    assert ( s != 0 );

    if ( position != PosPublicEnd )
        return;

    // End of 'public' block - we add testDefines method

    genTestBaseDefsFunc ( s, hdr, impl );

    if ( _genDebugSymbols )
    {
        genDumpFunc ( s, hdr, impl );
    }

    if ( !s || !s->isMessage() )
        return;

    hdr.e();
    hdr.ce ( "@brief Creates a clone of this object" );
    hdr.ce ( "It returns a pointer to the base message type, but it is a copy of the full object," );
    hdr.ce ( "at the lowest point in the inheritance tree." );
    hdr.ce ( "@return The clone of this object" );
    hdr.ae ( String ( "virtual %1 * clone() const" ).arg ( getClassPath ( s->getBaseInheritance() ) ) );
    hdr.ae ( "{" );
    hdr.ae ( 1, String ( "return new %1 ( *this );" ).arg ( s->getName() ) );
    hdr.ae ( "}" ).e();

    if ( !s->isBaseMessage() )
        return;

    // This version of 'deserialize' is only present in base messages:

    hdr.e();
    hdr.ce ( "@brief Tries to deserialize a message using its base message" );
    hdr.ce();
    hdr.ce ( "For this to work, the base message has to still contain the original buffer." );
    hdr.ce();
    hdr.ce ( "Before deserializing this function performs sanity test using testDefines()." );
    hdr.ce ( "If it succeeds, a reference to the original buffer from the baseMsg" );
    hdr.ce ( "will be stored in this object as well." );
    hdr.ce();
    hdr.ce ( "@param [in] baseMsg The base message to use" );
    hdr.ce ( "@param [out] extError Pointer to extended error code if it should be used (only modified on error)." );
    hdr.ce ( "@return The error code" );
    hdr.ae ( String ( "%1 deserialize ( const %2 & baseMsg, %3 * extError = 0 );" )
             .arg ( getStdType ( TypeErrorCode ), getClassPath ( s ), getStdType ( TypeExtError ) ) ).e();

    // This is to expose other versions of 'deserialize':
    hdr.ae ( "using SerializableMessage::deserialize;" );

    impl.ae ( String ( "%1 %2::deserialize ( const %2 & baseMsg, %3 * extError )" )
              .arg ( getStdType ( TypeErrorCode ), getClassPath ( s ), getStdType ( TypeExtError ) ) );

    impl.ae ( "{" );
    impl.incBaseIndent ( 1 );

    impl.ae ( "if ( !testDefines ( baseMsg, extError ) )" );
    impl.ae ( "{" );

    genSetupExtError ( impl, 1,
                       getErrorCode ( ErrDefinedValueMismatch ),
                       String ( "\"Error while testing defines in %1::deserialize ( %1 )\"" )
                       .arg ( getClassPath ( s ) ) );

    impl.ae ( 1, String ( "return %1;" ).arg ( getErrorCode ( ErrDefinedValueMismatch ) ) );
    impl.ae ( "}" ).e();
    impl.ae ( "return deserializeFromBase ( baseMsg, extError );" );
    impl.decBaseIndent ( 1 );
    impl.ae ( "}" ).e();
}

void PravalaCppGenerator::genTestBaseDefsFunc ( Symbol * s, CppFile & hdr, CppFile & impl )
{
    assert ( s != 0 );

    // We don't want to generate this if something is not a message
    if ( !s->isMessageOrStruct() )
        return;

    // Structures don't have base messages
    if ( s->isStruct() )
        return;

    Symbol * baseMsg = s;

    while ( baseMsg->getInheritance() != 0 )
    {
        baseMsg = baseMsg->getInheritance();
    }

    assert ( baseMsg != 0 );

    // By definition, base message is any message that doesn't inherit anything else
    assert ( baseMsg->isBaseMessage() );

    // If the oldest ancestor of this message is NOT a base message, we can't generate this...
    if ( !baseMsg->isBaseMessage() )
        return;

    // Otherwise we are a message, and baseMsg is our oldest ancestor (and it is a base message!)
    // (WE could be a base message, but it's ok!)
    assert ( s->isMessageOrStruct() );
    assert ( baseMsg->isMessageOrStruct() );
    assert ( baseMsg->isBaseMessage() );

    hdr.ce ( "@brief Verifies whether the base message matches 'defines' set by this message" );
    hdr.ce();
    hdr.ce ( "It checks whether all values 'defined' by this class (that are part" );
    hdr.ce ( "of the base message) have correct (for this message) values" );
    hdr.ce ( "If it returns true it still doesn't mean that deserializing will succeed," );
    hdr.ce ( "but when it's false it will fail for sure" );
    hdr.ce();
    hdr.ce ( "@param [in] baseMsg The base message to check" );
    hdr.ce ( "@param [out] extError Pointer to extended error code if it should be used (only modified on error)." );
    hdr.ce ( "@return True if it makes sense to try to deserialize from the buffer associated with" );
    hdr.ce ( "         this base message. It doesn't mean it will succeed for sure, but when it returns" );
    hdr.ce ( "         false there is an error for sure" );
    hdr.ae ( String ( "virtual bool testDefines ( const %1 & baseMsg, %2 * extError = 0 ) const;" )
             .arg ( getClassPath ( baseMsg ), getStdType ( TypeExtError ) ) ).e();

    impl.ae ( String ( "bool %1::testDefines ( const %2 & baseMsg, %3 * extError ) const" )
              .arg ( getClassPath ( s ), getClassPath ( baseMsg ), getStdType ( TypeExtError ) ) );
    impl.ae ( "{" );
    impl.ae ( 1, "( void ) baseMsg;" );
    impl.ae ( 1, "( void ) extError;" );
    impl.e();

    for ( HashMap<String, Element * >::Iterator defsIt ( s->getDefines() ); defsIt.isValid(); defsIt.next() )
    {
        Element * e = defsIt.value();

        assert ( e != 0 );

        if ( !e->definedTarget )
            continue;

        assert ( e->definedTarget->containerSymbol != 0 );

        // The "target" element is not declared in the base message but
        // somewhere else. We don't have access to those get_* functions,
        // we can only call functions defined in our base message!
        // This define cannot be checked...
        if ( e->definedTarget->containerSymbol != baseMsg )
            continue;

        assert ( e->name == e->definedTarget->name );

        impl.ae ( 1, String ( "if ( !baseMsg.%1() || baseMsg.%2() != %3 )" )
                  .arg ( e->getCamelCaseName ( "has" ),
                         e->getCamelCaseName ( "get" ),
                         getDefName ( e ) ) );
        impl.ae ( 1, "{" );
        impl.incBaseIndent ( 2 );

        impl.ae ( "if ( extError != 0 )" );
        impl.ae ( "{" );

        impl.ae ( 1, String ( "if ( !%1() )" ).arg ( e->getCamelCaseName ( "has" ) ) );

        String defValueExpr;
        String defNameExpr;

        if ( e->definedTarget->typeSymbol != 0 && e->definedTarget->typeSymbol->isEnum() )
        {
            defValueExpr = String ( "%1().toString()" ).arg ( e->getCamelCaseName ( "get" ) );
            defNameExpr = String ( "%1.toString()" ).arg ( getDefName ( e ) );
        }
        else
        {
            defValueExpr = String ( "%1()" ).arg ( e->getCamelCaseName ( "get" ) );
            defNameExpr = getDefName ( e );
        }

        genSetupExtError ( impl, 1,
                           getErrorCode ( ErrDefinedValueMismatch ),
                           String ( "String ( \"%1.%2 is not set, instead of being set to '%4'\" ).arg ( %3 )" )
                           .arg ( getClassPath ( s ), e->name, defNameExpr, "%1" ), false );

        impl.ae ( 1, "else" );

        genSetupExtError ( impl, 1,
                           getErrorCode ( ErrDefinedValueMismatch ),
                           String ( "String ( \"%1.%2 is set to '%5', instead of being set to '%6'\" )"
                                    ".arg ( %3 ).arg ( %4 )" )
                           .arg ( getClassPath ( s ), e->name, defValueExpr, defNameExpr, "%1", "%2" ), false );

        impl.ae ( "}" ).e();
        impl.ae ( "return false;" );
        impl.decBaseIndent ( 2 );
        impl.ae ( 1, "}" );
    }

    impl.e();

    if ( s->getInheritance() != 0 )
    {
        impl.ae ( 1, String ( "return %1::testDefines ( baseMsg, extError );" )
                  .arg ( s->getInheritance()->getName() ) );
    }
    else
    {
        impl.ae ( 1, "return true;" );
    }

    impl.ae ( "}" ).e();
}

void addI ( CppFile & file )
{
    file.a ( "buf.append ( indent );" ).e();
}

void add ( CppFile & file, const char * what )
{
    file.a ( "buf.append ( \"" ).a ( what ).a ( "\" );" ).e();
}

void add ( CppFile & file, const String & what )
{
    add ( file, what.c_str() );
}

void addCode ( CppFile & file, const char * what )
{
    file.a ( "buf.append ( " ).a ( what ).a ( " );" ).e();
}

void addCode ( CppFile & file, const String & what )
{
    addCode ( file, what.c_str() );
}

void eol ( CppFile & file )
{
    file.a ( "buf.append(\"\\n\");" ).e();
}

void PravalaCppGenerator::genEnumHashGets ( CppFile & hdr )
{
    if ( _intEnumTypes.isEmpty() )
        return;

    hdr.addCppInclude ( "basic/HashMap.hpp", CppFile::IncludeLocal );

    while ( !_intEnumTypes.isEmpty() )
    {
        Symbol * eSym = _intEnumTypes.first();
        _intEnumTypes.removeFirst();

        assert ( eSym != 0 );
        assert ( eSym->isEnum() );

        hdr.ce ( "@brief Returns the hashing code for the enum value provided." );
        hdr.ce ( "@param [in] hKey The value used as a key, used for generating the hashing code." );
        hdr.ce ( "@return The hashing code for the enum value provided." );
        hdr.ae ( String ( "inline size_t getHash ( const %1 & hKey )" ).arg ( getClassPath ( eSym ) ) );
        hdr.ae ( "{" );
        hdr.ae ( 1, "return Pravala::getHash ( hKey.value() );" );
        hdr.ae ( "}" );
    }
}

void PravalaCppGenerator::genClassHeader ( Symbol * s, CppFile & hdr, CppFile & impl )
{
    assert ( s != 0 );

    if ( s->isBaseMessage() )
    {
        assert ( !s->getInheritance() );

        hdr.ae ( String ( "class %1: public SerializableMessage" ).arg ( s->getName() ) );

        hdr.addCppInclude ( "proto/SerializableMessage.hpp", CppFile::IncludeLocal );
        return;
    }

    if ( s->isStruct() && !s->getInheritance() )
    {
        hdr.ae ( String ( "class %1: public Serializable" ).arg ( s->getName() ) );

        hdr.addCppInclude ( "proto/Serializable.hpp", CppFile::IncludeLocal );
        return;
    }

    CppGenerator::genClassHeader ( s, hdr, impl );
    return;
}

void PravalaCppGenerator::genMsgSerializeFieldsMethod ( Symbol * s, CppFile & hdr, CppFile & impl )
{
    CppGenerator::genMsgSerializeFieldsMethod ( s, hdr, impl );

    if ( !_enableJson || !s || !s->isMessageOrStruct() )
    {
        return;
    }

    // We add those includes only in the base file.
    bool addedArrInclude = ( s->getOldestMessageOrStructParent() != 0 );
    bool addedObjInclude = addedArrInclude;

    hdr.ae ( String ( "virtual %1 serializeFields ( Json & json, %2 * extError );" )
             .arg ( getStdType ( TypeErrorCode ),
                    getStdType ( TypeExtError ) ) );
    hdr.e();

    const StringList & elems = s->getOrdElements();

    impl.ae ( String ( "%1 %2::serializeFields ( Json & json, %3 * extError )" )
              .arg ( getStdType ( TypeErrorCode ),
                     getClassPath ( s ),
                     getStdType ( TypeExtError ) ) );
    impl.ae ( "{" );

    impl.incBaseIndent();

    if ( s->getInheritance() != 0 )
    {
        impl.a ( String ( "%1 ret = " ).arg ( getStdType ( TypeErrorCode ) ) );
        impl.a ( s->getInheritance()->getName() ).a ( "::serializeFields ( json, extError );" ).e().e();

        impl.ae ( String ( "if ( ret != %1 )" ).arg ( getErrorCode ( ErrOK ) ) );
        impl.ae ( "{" );

        genSetupExtError ( impl, 1, "ret",
                           String ( "\"Error calling %1::serializeFields from %2\"" )
                           .arg ( getClassPath ( s->getInheritance() ), getClassPath ( s ) ) );

        impl.ae ( 1, "return ret;" );
        impl.ae ( "}" );
    }
    else
    {
        impl.ae ( "( void ) extError;" );
        impl.ae ( String ( "%1 ret = %2;" ).arg ( getStdType ( TypeErrorCode ), getErrorCode ( ErrOK ) ) );
        impl.ae ( "json.clear();" );
    }

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // In JSON mode we don't serialize fields used as alias storages.
        // We will store only aliases themselves, as individual entries.
        if ( !e->lastAliasedIn.isEmpty() )
        {
            continue;
        }

        String varName;

        if ( e->aliasTarget != 0 )
        {
            varName = e->getCamelCaseName ( "get" ).append ( "()" );
        }
        else
        {
            assert ( e->typeSymbol != 0 );

            varName = getVarName ( e );
        }

        if ( !e->isRepeated() )
        {
            if ( !addedObjInclude )
            {
                addedObjInclude = true;
                impl.addCppInclude ( "json/Json.hpp", CppFile::IncludeLocal );
            }

            impl.ae ( String ( "if ( %1() )" ).arg ( e->getCamelCaseName ( "has" ) ) );
            impl.ae ( "{" );
        }
        else
        {
            assert ( !e->aliasTarget );
            assert ( e->typeSymbol != 0 );

            if ( !addedArrInclude )
            {
                addedArrInclude = true;
                impl.addCppInclude ( "json/JsonArray.hpp", CppFile::IncludeLocal );
            }

            impl.ae ( String ( "if ( %1 > 0 )" ).arg ( exprListVarSize ( e->typeSymbol, getVarName ( e ) ) ) );
            impl.ae ( "{" );

            impl.incBaseIndent();

            impl.ae ( "JsonArray jsonArr;" ).e();

            impl.ae ( String ( "for ( size_t i = 0, lSize = %1; i < lSize; ++i )" )
                      .arg ( exprListVarSize ( e->typeSymbol, getVarName ( e ) ) ) );
            impl.ae ( "{" );

            varName = "varRef";

            impl.ae ( 1, String ( "%1 & %2 = %3;" )
                      .arg ( getRawVarType ( hdr, e->typeSymbol, VarUseStorage ),
                             varName,
                             exprListGetElemIdxRef ( e->typeSymbol, getVarName ( e ), "i" ) ) ).e();
        }

        impl.incBaseIndent();

        if ( !e->typeSymbol )
        {
            // No need to do anything.
            // varName is either the 'get' function, or a primitive variable itself.
            // We can just pass it to Json.
        }
        else if ( e->typeSymbol->isMessageOrStruct() )
        {
            impl.ae ( "Json tmpJson;" ).e();

            impl.ae ( String ( "ret = %1.serialize ( tmpJson, extError );" ).arg ( varName ) ).e();
            impl.ae ( String ( "if ( ret != %1 )" ).arg ( getErrorCode ( ErrOK ) ) );
            impl.ae ( "{" );

            genSetupExtError ( impl, 1, "ret",
                               String ( "\"Error serializing %1.%2 into temporary JSON object\"" )
                               .arg ( getClassPath ( s ), e->name ) );

            impl.ae ( 1, "return ret;" );
            impl.ae ( "}" ).e();

            // We still need to add it to Json, but we will be storing the tmpJson object:
            varName = "tmpJson";
        }
        else if ( e->typeSymbol->isEnum() )
        {
            // This is en enum, so we will be storing its value as a string:
            varName = String ( "( %1.toString() )" ).arg ( varName );
        }

        if ( e->isRepeated() )
        {
            impl.ae ( String ( "if ( !jsonArr.append(%1) )" ).arg ( varName ) );
            impl.ae ( "{" );
            genSetupExtError ( impl, 1, getErrorCode ( ErrProtocolError ),
                               String ( "\"Error appending %1.%2 to JSON array\"" )
                               .arg ( getClassPath ( s ), e->name ) );
            impl.ae ( 1, String ( "return %1;" ).arg ( getErrorCode ( ErrProtocolError ) ) );
            impl.ae ( "}" );
            impl.decBaseIndent();
            impl.ae ( "}" ).e();

            // And now we need to store the array itself in the JSON object:
            varName = "jsonArr";
        }

        impl.ae ( String ( "if ( !json.put(\"%1\",%2) )" ).arg ( e->getCamelCaseName ( "" ), varName ) );
        impl.ae ( "{" );

        genSetupExtError ( impl, 1, getErrorCode ( ErrProtocolError ),
                           String ( "\"Error inserting %1.%2 into JSON object\"" )
                           .arg ( getClassPath ( s ), e->name ) );

        impl.ae ( 1, String ( "return %1;" ).arg ( getErrorCode ( ErrProtocolError ) ) );
        impl.ae ( "}" );

        impl.decBaseIndent();
        impl.ae ( "}" ).e();
    }

    impl.e();
    impl.ae ( "return ret;" );
    impl.decBaseIndent();
    impl.ae ( "}" ).e();
}

void PravalaCppGenerator::genDumpFunc ( Symbol * s, CppFile & hdr, CppFile & impl )
{
    assert ( s != 0 );

    // We don't do this for enum symbols - they don't really
    // carry any data, and should be carried by other messages
    if ( s->isEnum() )
        return;

    hdr.ce ( "@brief Dumps the data description to the buffer" );
    hdr.ce();
    hdr.ce ( "@param [in] buf The buffer to dump the description to" );
    hdr.ce ( "@param [in,out] indent The indent to be used by the lowest class" );
    hdr.a ( "virtual void dumpDataDesc ( Buffer & buf, String & indent );" );
    hdr.e().e();

    impl.a ( "void " ).a ( getClassPath ( s ) ).a ( "::dumpDataDesc ( Buffer & buf, String & indent )" ).e();
    impl.a ( "{" ).e();

    impl.incBaseIndent();

    String msgStr;

    if ( s->getInheritance() != 0 )
    {
        impl.a ( s->getInheritance()->getName() );
        impl.a ( "::dumpDataDesc ( buf, indent );" ).e().e();
        msgStr = "^ ";
    }
    else
    {
        msgStr = "* ";
    }

    addI ( impl );

    msgStr.append ( getClassPath ( s ) );

    String str ( msgStr );
    str.append ( " [" );

    if ( !s->isBaseMessage() )
    {
        str.append ( "msg; valid: " );
    }
    else
    {
        str.append ( "base-msg; valid: " );
    }

    add ( impl, str );

    addCode ( impl, "validate().toString()" );
    add ( impl, "]\\n" );

    impl.e();
    impl.a ( "String orgIndent = indent;" ).e().e();
    impl.a ( "indent.append ( \"   \" );" ).e().e();

    const StringList & elems = s->getOrdElements();

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        String varName = getVarName ( e );

        str = e->name;
        str.append ( ":" );
        str.append ( getFieldIdName ( e ) );
        str.append ( " [" );

        if ( e->isPrivate() )
            str.append ( " private" );
        if ( e->isProtected() )
            str.append ( " protected" );
        if ( e->isPublic() )
            str.append ( " private" );
        if ( e->isOptional() )
            str.append ( " optional" );
        if ( e->isRequired() )
            str.append ( " required" );
        if ( e->isUnique )
            str.append ( " unique" );

        for ( HashMap<String, String>::Iterator it ( e->options ); it.isValid(); it.next() )
        {
            str.append ( " " );
            str.append ( it.key() );
            str.append ( "='" );
            str.append ( it.value().replace ( "\"", "\\\"" ) );
            str.append ( "'" );
        }

        if ( e->isRepeated() )
            str.append ( " repeated (size: " );

        addI ( impl );
        add ( impl, str );
        str = "";

        if ( e->isRepeated() )
        {
            addCode ( impl, String ( "String::number ( " )
                      .append ( exprListVarSize ( e->typeSymbol, getVarName ( e ) ) )
                      .append ( " ) " ) );
            str = ")";
        }

        str.append ( " ]" );
        bool addEol = true;

        add ( impl, str );

        impl.e();

        if ( !e->isRepeated() )
        {
            impl.a ( "if ( " ).a ( e->getCamelCaseName ( "has" ) ).a ( "() )" ).e();
            impl.a ( "{" ).e();
            impl.incBaseIndent();
        }
        else
        {
            impl.e();

            impl.a ( "for ( size_t i = 0, lSize = " );
            impl.a ( exprListVarSize ( e->typeSymbol, getVarName ( e ) ) );
            impl.a ( "; i < lSize; ++i )" ).e();
            impl.a ( "{" ).e();
            impl.incBaseIndent();

            eol ( impl );
            addI ( impl );

            str = " ";
            str.append ( e->name );
            str.append ( "[" );
            add ( impl, str );
            addCode ( impl, "String::number ( i )" );
            add ( impl, "]" );

            impl.e();

            varName = "varRef";

            impl.a ( getRawVarType ( hdr, e->typeSymbol, VarUseStorage ) ).a ( " & " ).a ( varName );
            impl.a ( " = " ).a ( exprListGetElemIdxRef ( e->typeSymbol, getVarName ( e ), "i" ) );
            impl.a ( ";" ).e().e();
        }

        if ( !e->typeSymbol )
        {
            // This is only possible in enum symbols and for elements that are aliases.
            // This function should have exited if this symbol is an enum!
            assert ( e->aliasTarget != 0 );
            assert ( !e->isRepeated() );

            add ( impl, " = " );
            addCode ( impl, String ( "String::number(" )
                      .append ( e->getCamelCaseName ( "get" ) ).append ( "() )" ) );
        }
        else if ( e->typeSymbol->isEnum() )
        {
            add ( impl, " = " );
            addCode ( impl, String ( varName ).append ( ".toString()" ) );
        }
        else if ( e->typeSymbol->isMessageOrStruct() )
        {
            eol ( impl );
            addEol = false;

            impl.e();
            impl.a ( varName ).a ( ".dumpDataDesc ( buf, String ( indent ).append ( \" \" ) );" ).e();
        }
        else
        {
            if ( e->typeSymbol == _symString )
            {
                add ( impl, " = '" );
                addCode ( impl, varName );
                add ( impl, "'" );
            }
            else if ( e->typeSymbol == _symIpAddr || e->typeSymbol == _symTimestamp )
            {
                add ( impl, " = '" );
                addCode ( impl, String ( varName ).append ( ".toString()" ) );
                add ( impl, "'" );
            }
            else if ( e->typeSymbol->isInteger() || e->typeSymbol->isFloatingPoint() )
            {
                add ( impl, " = " );
                addCode ( impl, String ( "String::number ( " ).append ( varName ).append ( " )" ) );
            }
            else
            {
                add ( impl, " = '" );
                addCode ( impl, varName );
                add ( impl, "'" );
            }
        }

        impl.decBaseIndent();
        impl.a ( "}" ).e();

        if ( !e->isRepeated() )
        {
            impl.a ( "else" ).e();
            impl.a ( "{" ).e();
            impl.incBaseIndent();

            add ( impl, " = <not set>" );

            impl.decBaseIndent();
            impl.a ( "}" ).e();
        }

        if ( addEol )
            eol ( impl );

        impl.e();
    }

    impl.a ( "indent = orgIndent;" ).e().e();

    addI ( impl );
    add ( impl, msgStr );
    eol ( impl );

    impl.e();
    impl.a ( "indent.append ( \" \" );" ).e();
    impl.decBaseIndent();
    impl.a ( "}" ).e().e();
}
