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
#include "PravJavaGen.hpp"

#define PROTO_PACKAGE      "com.pravala.protocol"
#define ERRCODE_CLASS      "com.pravala.protocol.auto.ErrorCode"
#define LENGTH_VAR_CODE    "0"

using namespace Pravala;

PravalaJavaGenerator::PravalaJavaGenerator ( ProtocolSpec & proto ):
    JavaGenerator ( proto ),
    _symString ( 0 ),
    _symIpAddr ( 0 )
{
    _symString = _proto.getRoot()->createBasicRootType ( "string", Symbol::SpecTypeString );
    _symIpAddr = _proto.getRoot()->createBasicRootType ( "ip_addr" );
}

String PravalaJavaGenerator::getErrorCode ( const JavaGenerator::ErrorCode & errCode )
{
    switch ( errCode )
    {
        case ErrOK:
            return ERRCODE_CLASS ".Success";
            break;

        case ErrInvalidParam:
            return ERRCODE_CLASS ".InvalidParameter";
            break;

        case ErrInvalidData:
            return ERRCODE_CLASS ".InvalidData";
            break;

        case ErrRequiredFieldNotSet:
            return ERRCODE_CLASS ".RequiredFieldNotSet";
            break;

        case ErrFieldValueOutOfRange:
            return ERRCODE_CLASS ".FieldValueOutOfRange";
            break;

        case ErrStringLengthOutOfRange:
            return ERRCODE_CLASS ".StringLengthOutOfRange";
            break;

        case ErrListSizeOutOfRange:
            return ERRCODE_CLASS ".ListSizeOutOfRange";
            break;

        case ErrDefinedValueMismatch:
            return ERRCODE_CLASS ".DefinedValueMismatch";
            break;

        case ErrProtocolError:
            return ERRCODE_CLASS ".ProtocolError";
            break;
    }

    throw Error ( "Pravala Java generator does not support error code '",
                  String::number ( errCode ), "'" );
}

String PravalaJavaGenerator::getStdType ( const JavaGenerator::StdType & stdType )
{
    switch ( stdType )
    {
        case TypeErrorCode:
            return ERRCODE_CLASS;
            break;

        case TypeReadBuffer:
            return PROTO_PACKAGE ".ReadBuffer";
            break;

        case TypeProtoException:
            return PROTO_PACKAGE ".CodecException";
            break;

        case TypeWriteBuffer:
            return "java.io.OutputStream";
            break;

        case TypeFieldId:
            return "int";
            break;
    }

    throw Error ( "Pravala Java generator does not support '",
                  String::number ( stdType ), "' data type" );
}

String PravalaJavaGenerator::getListVarType ( Symbol * intSymbol )
{
    return String ( "java.util.ArrayList<" ).append ( getRawVarType ( intSymbol ) ).append ( ">" );
}

String PravalaJavaGenerator::getListIfaceType ( Symbol * intSymbol )
{
    return String ( "java.util.List<" ).append ( getRawVarType ( intSymbol ) ).append ( ">" );
}

String PravalaJavaGenerator::getRawVarType ( Symbol * symbol )
{
    assert ( symbol != 0 );

    if ( !symbol->isBasic() )
        return getClassPath ( symbol );

    if ( symbol == _symString )
        return "java.lang.String";

    if ( symbol == _symIpAddr )
        return "java.net.InetAddress";

    const String & name = symbol->getName();

    if ( name == "bool" )
    {
        return "Boolean";
    }
    else if ( name == "double" )
    {
        return "Double";
    }
    else if ( name == "float" )
    {
        return "Float";
    }

    bool ok = false;
    int bitLen = 0;
    String typeName ( PROTO_PACKAGE ".types." );

    if ( name.startsWith ( "int", false ) )
    {
        bitLen = name.substr ( 3 ).toInt32 ( &ok );
    }
    else if ( name.startsWith ( "uint", false ) )
    {
        bitLen = name.substr ( 4 ).toInt32 ( &ok );
        typeName.append ( "U" );
    }

    if ( ok )
    {
        switch ( bitLen )
        {
            case 8:
                return "Byte";
                break;

            case 16:
                return "Short";
                break;

            case 32:
                return "Integer";
                break;

            case 64:
                return "Long";
                break;
        }
    }

    return name;
}

String PravalaJavaGenerator::getRawPrimitiveVarType ( Symbol * symbol )
{
    assert ( symbol != 0 );
    const String & name = symbol->getName();

    if ( symbol->isBasic()
         && symbol != _symString
         && symbol != _symIpAddr )
    {
        if ( name == "double" || name == "float" )
            return name;

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
                    return "byte";
                    break;

                case 16:
                    return "short";
                    break;

                case 32:
                    return "int";
                    break;

                case 64:
                    return "long";
                    break;
            }
        }
    }

    throw Error ( "Java language generator does not support raw primitive type for '", name,
                  "' (current symbol: ", getFilePath ( symbol ), ")" );

    return "unsupported";
}

String PravalaJavaGenerator::getExtends ( Symbol * s )
{
    if ( s != 0 )
    {
        if ( s->isBaseMessage() )
        {
            assert ( !s->getInheritance() );

            return PROTO_PACKAGE ".SerializableBase";
        }
        else if ( s->isMessageOrStruct() && !s->getInheritance() )
        {
            return PROTO_PACKAGE ".Serializable";
        }
    }

    return JavaGenerator::getExtends ( s );
}

String PravalaJavaGenerator::exprThrowException ( const JavaGenerator::ErrorCode & errCode )
{
    return String ( "throw new " PROTO_PACKAGE ".CodecException ( " )
           .append ( getErrorCode ( errCode ) )
           .append ( " )" );
}

String PravalaJavaGenerator::exprThrowException ( const String & errCodeVarName )
{
    return String ( "throw new " PROTO_PACKAGE ".CodecException ( " )
           .append ( errCodeVarName )
           .append ( " )" );
}

String PravalaJavaGenerator::exprStringVarLength ( const String & listVarName )
{
    return String ( listVarName ).append ( ".length()" );
}

String PravalaJavaGenerator::exprListVarSize (
        Symbol * /* intSymbol */,
        const String & varName )
{
    return String ( varName ).append ( ".size()" );
}

String PravalaJavaGenerator::exprListAppend (
        Symbol * /* intSymbol */,
        const String & listVarName,
        const String & appendVarName )
{
    return String ( listVarName ).append ( ".add ( " ).append ( listVarName ).append ( ".size(), " )
           .append ( appendVarName ).append ( " )" );
}

String PravalaJavaGenerator::exprListGetElemIdx (
        Symbol * /* intSymbol */,
        const String & listVarName,
        const String & indexVarName )
{
    return String ( listVarName ).append ( ".get ( " ).append ( indexVarName ).append ( " )" );
}

String PravalaJavaGenerator::exprDeclareAndCreateWriteBuf ( const String & bufVarName )
{
    return String ( "java.io.ByteArrayOutputStream " )
           .append ( bufVarName )
           .append ( " = new java.io.ByteArrayOutputStream()" );
}

String PravalaJavaGenerator::exprDeleteWriteBuf ( const String & bufVarName )
{
    return String ( bufVarName ).append ( " = null" );
}

String PravalaJavaGenerator::exprProtoEncode (
        const String & bufVarName,
        const String & valueVarName,
        const String & valueCode )
{
    return String ( PROTO_PACKAGE ".Codec.appendField (" )
           .append ( bufVarName )
           .append ( ", " )
           .append ( valueVarName )
           .append ( ", " )
           .append ( valueCode )
           .append ( " )" );
}

String PravalaJavaGenerator::exprProtoDecodeFieldValue (
        Symbol * symbol,
        const String & hdrDescVarName,
        const String & bufDescVarName,
        const String & fieldVarName )
{
    assert ( symbol != 0 );

    if ( !symbol->isBasic() )
        throw Error ( "Java language generator does not support direct decoding of complex types (in symbol ",
                      getFilePath ( symbol ), ")" );

    String ret ( fieldVarName );

    ret.append ( " = " );
    ret.append ( PROTO_PACKAGE ".Codec.read" );

    if ( symbol == _symString )
    {
        ret.append ( "String" );
    }
    else if ( symbol == _symIpAddr )
    {
        ret.append ( "InetAddress" );
    }
    else
    {
        const String & name = symbol->getName();

        if ( name == "bool" )
        {
            ret.append ( "Bool" );
        }
        else if ( name == "float" )
        {
            ret.append ( "Float" );
        }
        else if ( name == "double" )
        {
            ret.append ( "Double" );
        }
        else
        {
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
                        ret.append ( "Byte" );
                        break;

                    case 16:
                        ret.append ( "Short" );
                        break;

                    case 32:
                        ret.append ( "Integer" );
                        break;

                    case 64:
                        ret.append ( "Long" );
                        break;

                    default:
                        ok = false;
                        break;
                }
            }

            if ( !ok )
            {
                throw Error ( "Java language generator does not support direct decoding of '", name,
                              "' type (in symbol ", getFilePath ( symbol ), ")" );
            }
        }
    }

    return ret.append ( String ( " ( %1, %2 )" ).arg ( hdrDescVarName, bufDescVarName ) );
}

String PravalaJavaGenerator::exprVarClear ( Element * e )
{
    assert ( e != 0 );
    assert ( e->typeSymbol != 0 );

    // Aliases can't have 'clear()' function
    assert ( !e->aliasTarget );

    if ( e->isRepeated() )
        return getVarName ( e ).append ( ".clear()" );

    return getVarName ( e ).append ( " = null" );
}

void PravalaJavaGenerator::genMsgDeserializeMethod ( Symbol * s, JavaFile & impl )
{
    assert ( s != 0 );

    // ***************************************************************************************************************
    // *********************************************** deserializeField() ********************************************
    // ***************************************************************************************************************

    impl.alines ( String (
                      "@Override\n"
                      "protected boolean deserializeField ( " PROTO_PACKAGE ".Codec.FieldHeader hdr, %1 readBuffer ) "
                      "throws %2\n"
                      "{\n"
                      "  assert readBuffer != null;\n"
                      "  assert readBuffer.getOffset() >= 0;\n"
                      "  \n"
                      "  if ( readBuffer == null || hdr.fieldSize < 0 || readBuffer.getOffset() < 0\n"
                      "    || readBuffer.getOffset() + hdr.fieldSize != readBuffer.getSize() )\n"
                      "  {\n"
                      "    %3;\n"
                      "  }\n\n" )
                  .arg (
                      getStdType ( TypeReadBuffer ),
                      PROTO_PACKAGE ".CodecException",
                      exprThrowException ( ErrInvalidParam ) ) );

    const StringList & elems = s->getOrdElements();

    bool addedLoopBeg = false;

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // We don't deserialize alias fields directly.
        // The actual storage value should be deserialized by the class
        // that actually contains it.
        if ( e->aliasTarget != 0 )
            continue;

        if ( !addedLoopBeg )
        {
            addedLoopBeg = true;

            impl.i ( 1 ).a ( "switch ( hdr.fieldId )" ).e();
            impl.i ( 1 ).a ( "{" ).e();
        }

        if ( i > 0 )
            impl.e();

        impl.i ( 2 ).a ( "case " ).a ( getFieldIdName ( e ) ).a ( ": // " ).a ( e->getCamelCaseName() ).e();
        impl.i ( 2 ).a ( "{" ).e();

        impl.incBaseIndent ( 3 );

        assert ( e->typeSymbol != 0 );

        if ( e->typeSymbol->isMessageOrStruct() )
        {
            impl.ae ( "if ( hdr.isVarLen() )" );
            impl.ae ( "{" );
            impl.ae ( 1, String ( "%1;" ).arg ( exprThrowException ( ErrProtocolError ) ) );
            impl.ae ( "}" ).e();

            assert ( !e->typeSymbol->isEnum() );

            impl.ae ( String ( "%1 tmpVal = new %1();" ).arg ( getRawVarType ( e->typeSymbol ) ) ).e();

            if ( e->typeSymbol->isMessage() )
            {
                impl.ae ( "tmpVal.deserializeBase ( readBuffer );" ).e();
            }
            else
            {
                impl.ae ( "boolean ret = tmpVal.deserializeData ( readBuffer );" ).e();
            }

            if ( e->isRepeated() )
            {
                // It's a list - we just need to append the temporary element to it

                impl.a ( "if ( " ).a ( getVarName ( e ) ).a ( " == null )" ).e();
                impl.a ( "{" ).e();
                impl.i ( 1 ).a ( getVarName ( e ) ).a ( " = new " ).a ( getVarType ( e ) ).a ( "();" ).e();
                impl.a ( "}" ).e().e();

                impl.a ( exprListAppend ( e->typeSymbol, getVarName ( e ), "tmpVal" ) ).a ( ";" ).e();
            }
            else
            {
                impl.a ( getVarName ( e ) ).a ( " = tmpVal;" ).e();
            }

            if ( e->typeSymbol->isMessage() )
            {
                impl.ae ( "// This is a message, let's not propagate 'unknown' code up:" );
                impl.ae ( "return true;" );
            }
            else
            {
                impl.ae ( "return ret;" );
            }
        }
        else
        {
            String varName = getVarName ( e );

            if ( e->isRepeated() )
            {
                varName = "tmpVal";

                impl.a ( getRawVarType ( e->typeSymbol ) ).a ( " " ).a ( varName ).a ( ";" ).e().e();
            }

            if ( e->typeSymbol->isEnum() )
            {
                impl.a ( varName ).a ( " = " ).a ( getClassPath ( e->typeSymbol ) );
                impl.a ( ".deserializeEnum ( hdr, readBuffer );" ).e().e();

                impl.a ( "if ( " ).a ( varName ).a ( " == null )" ).e();
                impl.a ( "{" ).e();
                impl.i ( 1 ).a ( exprThrowException ( ErrInvalidData ) ).a ( ";" ).e();
                impl.a ( "}" ).e().e();
            }
            else
            {
                // This is a regular element - lets use protocol decoder to deserialize it
                impl.a ( exprProtoDecodeFieldValue ( e->typeSymbol, "hdr", "readBuffer", varName ) ).a ( ";" ).e().e();
            }

            if ( e->isRepeated() )
            {
                assert ( varName != getVarName ( e ) );

                // It's a list - we just need to append the temporary element to it
                impl.a ( "if ( " ).a ( getVarName ( e ) ).a ( " == null )" ).e();
                impl.a ( "{" ).e();
                impl.i ( 1 ).a ( getVarName ( e ) ).a ( " = new " ).a ( getVarType ( e ) ).a ( "();" ).e();
                impl.a ( "}" ).e().e();

                impl.a ( exprListAppend ( e->typeSymbol, getVarName ( e ), varName ) ).a ( ";" ).e();
            }

            impl.ae ( "return true;" );
        }

        impl.decBaseIndent ( 3 );
        impl.i ( 2 ).a ( "}" ).e();
    }

    if ( addedLoopBeg )
    {
        impl.e().ae ( 1, "}" ).e();
    }

    if ( s->getInheritance() != 0 )
    {
        impl.ae ( 1, "return super.deserializeField ( hdr, readBuffer );" );
    }
    else
    {
        impl.ae ( 1, "return false;" );
    }

    impl.ae ( "}" ).e();
}

void PravalaJavaGenerator::genMsgSerializeMethod ( Symbol * s, JavaFile & impl )
{
    assert ( s != 0 );

    impl.alines ( String (
                      "@Override\n"
                      "protected void serializeAllFields ( %1 writeTo ) throws java.io.IOException, %2\n"
                      "{" )
                  .arg ( getStdType ( TypeWriteBuffer ), PROTO_PACKAGE ".CodecException" ) );

    const StringList & elems = s->getOrdElements();

    if ( s->getInheritance() != 0 )
    {
        impl.ae ( 1, "super.serializeAllFields ( writeTo );" );
    }

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        impl.e();

        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // We don't serialize alias fields directly.
        // The actual storage value should be serialized by the class
        // that actually contains it.
        if ( e->aliasTarget != 0 )
            continue;

        assert ( e->typeSymbol != 0 );

        String varName = getVarName ( e );

        if ( !e->isRepeated() )
        {
            impl.i ( 1 ).a ( "if ( " ).a ( e->getCamelCaseName ( "has" ) ).a ( "() )" ).e();
            impl.i ( 1 ).a ( "{" ).e();
        }
        else
        {
            impl.i ( 1 ).a ( "for ( int i = 0, lSize = " );
            impl.a ( e->getCamelCaseName ( "count" ) );
            impl.a ( "(); i < lSize; ++i )" ).e();
            impl.i ( 1 ).a ( "{" );

            varName = "listElem";

            impl.i ( 2 ).a ( getRawVarType ( e->typeSymbol ) ).a ( " " ).a ( varName );
            impl.a ( " = " ).a ( exprListGetElemIdx ( e->typeSymbol, getVarName ( e ), "i" ) );
            impl.a ( ";" ).e().e();
        }

        if ( e->typeSymbol->isMessageOrStruct() )
        {
            impl.i ( 2 ).a ( exprDeclareAndCreateWriteBuf ( "tmpBuf" ) ).a ( ";" ).e();

            impl.i ( 2 ).a ( varName ).a ( ".serializeData ( tmpBuf );" ).e().e();

            impl.i ( 2 ).a ( exprProtoEncode ( "writeTo", "tmpBuf", getFieldIdName ( e ) ) ).a ( ";" ).e().e();

            impl.i ( 2 ).a ( exprDeleteWriteBuf ( "tmpBuf" ) ).a ( ";" ).e().e();
        }
        else if ( e->typeSymbol->isEnum() )
        {
            impl.i ( 2 ).a ( varName ).a ( ".serializeEnum( writeTo, " ).a ( getFieldIdName ( e ) ).a ( " );" ).e();
        }
        else
        {
            impl.i ( 2 ).a ( exprProtoEncode ( "writeTo", varName, getFieldIdName ( e ) ) ).a ( ";" ).e();
        }

        impl.i ( 1 ).a ( "}" ).e();
    }

    impl.a ( "}" ).e().e();
}

void PravalaJavaGenerator::genEnumDeserializeMethod ( Symbol * s, JavaFile & impl )
{
    Symbol * enumSym = getEnumStorageSymbol();

    impl.co();
    impl.ce ( "Creates an enum based on a buffer content" );
    impl.ce();
    impl.ce ( "This will check if the deserialized value is legal for this enum" );
    impl.ce ( "and return null if not." );
    impl.ce ( "@param hdr The header of the field" );
    impl.ce ( "@param readBuffer The buffer to deserialize the data from" );
    impl.ce ( "@return The newly created enum, or null in case the value was incorrect for this enum" );
    impl.ce ( "@throws " PROTO_PACKAGE ".CodecException" );
    impl.cc();
    impl.ae ( String ( "public static %1 deserializeEnum ( %2 hdr, %3 readBuffer ) throws %4" )
              .arg ( s->getName(), PROTO_PACKAGE ".Codec.FieldHeader",
                     getStdType ( TypeReadBuffer ), PROTO_PACKAGE ".CodecException" ) );
    impl.a ( "{" ).e();
    impl.i ( 1 ).a ( getRawVarType ( enumSym ) ).a ( " tmpVal;" );
    impl.i ( 1 ).a ( exprProtoDecodeFieldValue ( enumSym, "hdr", "readBuffer", "tmpVal" ) ).a ( ";" ).e().e();
    impl.i ( 1 ).a ( "return createFromRaw ( tmpVal );" ).e();
    impl.a ( "}" ).e().e();
}

void PravalaJavaGenerator::genEnumSerializeMethod ( Symbol * /* s */, JavaFile & impl )
{
    impl.co();
    impl.ce ( "Serializes the enum" );
    impl.ce();
    impl.ce ( "@param writeTo Where to serialize the data to" );
    impl.ce ( "@param fieldId The ID of the field to use" );
    impl.ce ( "@throws java.io.IOException, " PROTO_PACKAGE ".CodecException" );
    impl.cc();
    impl.a ( "public void serializeEnum ( " );
    impl.a ( getStdType ( TypeWriteBuffer ) ).a ( " writeTo, " );
    impl.a ( getStdType ( TypeFieldId ) );
    impl.a ( " fieldId ) throws java.io.IOException, " PROTO_PACKAGE ".CodecException" ).e();
    impl.a ( "{" ).e();
    impl.i ( 1 ).a ( exprProtoEncode ( "writeTo", "this.value", "fieldId" ) ).a ( ";" ).e();
    impl.a ( "}" ).e().e();
}

void PravalaJavaGenerator::hookPosition (
        Symbol * s, JavaFile & impl,
        const PositionType & position )
{
    JavaGenerator::hookPosition ( s, impl, position );

    assert ( s != 0 );

    // End of 'public' block - we add declaration of 'serialize' and 'deserialize' methods
    if ( position == PosBeforeClassClose )
    {
        genDeserFromBaseFunc ( s, impl );
        return;
    }
}

void PravalaJavaGenerator::genDeserFromBaseFunc ( Symbol * s, JavaFile & impl )
{
    assert ( s != 0 );

    // We don't want to generate this if something is not a message
    if ( !s->isMessageOrStruct() )
        return;

    Symbol * baseMsg = s;

    while ( baseMsg->getInheritance() != 0 )
    {
        baseMsg = baseMsg->getInheritance();
    }

    assert ( baseMsg != 0 );

    // If the oldest ancestor of this message is NOT a base message, we can't generate this...
    if ( !baseMsg->isBaseMessage() )
        return;

    assert ( baseMsg != 0 );

    // Otherwise we are a message, and baseMsg is our oldest ancestor (and it is a base message!)
    // (WE could be a base message, but it's ok!)
    assert ( s->isMessageOrStruct() );
    assert ( baseMsg->isMessageOrStruct() );
    assert ( baseMsg->isBaseMessage() );

    impl.co();
    impl.ce ( "Deserializes the entire message using its base message" );
    impl.ce();
    impl.ce ( "It checks whether all values 'defined' by this class (that are part" );
    impl.ce ( "of the base message) have correct (for this message) values" );
    impl.ce ( "If the base message has some values defined by this class it is faster" );
    impl.ce ( "than using the buffer-based deserialization method" );
    impl.ce ( "@param baseMsg The base message to deserialize the data from" );
    impl.ce ( "@return True if the message has been successfully deserialized and it used the entire available data." );
    impl.ce ( "        False means that although the message has been deserialized properly," );
    impl.ce ( "           there are some additional, unknown fields that have not been deserialized." );
    impl.ce ( "        If there is a deserialization error it throws one of the exceptions." );
    impl.ce ( "@throws " PROTO_PACKAGE ".CodecException" );
    impl.cc();

    impl.a ( "public boolean deserializeFromBase ( " ).a ( getClassPath ( baseMsg ) );
    impl.a ( " baseMsg ) throws " PROTO_PACKAGE ".CodecException" ).e();
    impl.a ( "{" ).e();

    bool addedNullCheck = false;

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

        if ( !addedNullCheck )
        {
            addedNullCheck = true;
            impl.i ( 1 ).a ( "if ( baseMsg == null )" ).e();
            impl.i ( 1 ).a ( "{" ).e();
            impl.i ( 2 ).a ( exprThrowException ( ErrInvalidParam ) ).a ( ";" ).e();
            impl.i ( 1 ).a ( "}" ).e().e();
        }

        impl.i ( 1 ).a ( "if ( !baseMsg." ).a ( e->getCamelCaseName ( "has" ) );
        impl.a ( "() || !" ).a ( getDefName ( e ) ).a ( ".equals ( baseMsg." );

        impl.a ( e->getCamelCaseName ( "get" ) ).a ( "() ) )" ).e();
        impl.i ( 1 ).a ( "{" ).e();
        impl.i ( 2 ).a ( exprThrowException ( ErrDefinedValueMismatch ) ).a ( ";" ).e();
        impl.i ( 1 ).a ( "}" ).e();
    }

    if ( addedNullCheck )
    {
        impl.e();
    }
    else
    {
        impl.i ( 1 ).a ( "// This message doesn't have any 'defined' values that could be checked" ).e();
        impl.i ( 1 ).a ( "// in the base message - just go through..." ).e();
    }

    if ( s->getInheritance() != 0 )
    {
        impl.i ( 1 ).a ( "return super.deserializeFromBase ( baseMsg );" ).e();
    }
    else
    {
        impl.i ( 1 ).a ( "return super.deserializeFromBaseSerializable ( baseMsg );" ).e();
    }

    impl.a ( "}" ).e().e();

    impl.co();
    impl.ce ( "Generates a new instance of the message based on the base message" );
    impl.ce();
    impl.ce ( "This function is NOT static, but the new message will have nothing in common" );
    impl.ce ( "with the object this function was called in; The entire state comes from the base message." );
    impl.ce ( "We need it to be 'virtual' so it cannot be static." );
    impl.ce();
    impl.ce ( "It may return a valid message even if the data contains some additional, unknown" );
    impl.ce ( "fields, as long as this message has all required values set and all defined values" );
    impl.ce ( "are correct. To get more information (whether there were some additional fields)" );
    impl.ce ( "other deserialization methods should be used instead." );
    impl.ce ( "@param baseMsg The base message to deserialize the data from" );
    impl.ce ( "@return The new instance of this message if it can be deserialized." );
    impl.ce ( "        Otherwise it throws an error" );
    impl.ce ( "@throws " PROTO_PACKAGE ".CodecException" );
    impl.cc();

    impl.a ( "public " ).a ( s->getName() ).a ( " generate ( " );
    impl.a ( getClassPath ( baseMsg ) ).a ( " baseMsg ) throws " PROTO_PACKAGE ".CodecException" ).e();
    impl.a ( "{" ).e();
    impl.i ( 1 ).a ( s->getName() ).a ( " msg = new " ).a ( s->getName() ).a ( "();" ).e().e();
    impl.i ( 1 ).a ( "// We don't care here if it has extra fields or not, so we ignore the return value." ).e();
    impl.i ( 1 ).a ( "// If anything is wrong it will throw an exception." ).e();
    impl.i ( 1 ).a ( "msg.deserializeFromBase ( baseMsg );" ).e().e();
    impl.i ( 1 ).a ( "return msg;" ).e();
    impl.a ( "}" ).e().e();
}
