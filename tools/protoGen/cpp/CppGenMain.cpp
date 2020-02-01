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

#include "../Symbol.hpp"
#include "../Error.hpp"
#include "CppGen.hpp"

using namespace Pravala;

#define INT_ENUM_TYPENAME    "_EnumType"

void CppGenerator::genEnumClass ( Symbol * s, CppFile & hdr, CppFile & impl )
{
    assert ( s != 0 );
    assert ( s->isEnum() );

    // For snprintf in toString()
    impl.addCppInclude ( "cstdio", CppFile::IncludeGlobal );

    hdr.ce ( "@brief Internal structure for storing the 'unsafe' enum type" );
    hdr.a ( "struct _" ).a ( s->getName() ).e();
    hdr.a ( "{" ).e();
    hdr.i ( 1 ).c ( "@brief Internal 'unsafe' enum type for values of '" );
    hdr.a ( s->getName() ).a ( "' enumerator" ).e();
    hdr.i ( 1 ).a ( "enum " ).a ( INT_ENUM_TYPENAME ).e();
    hdr.i ( 1 ).a ( "{" ).e();

    hdr.incBaseIndent ( 2 );

    const StringList & elems = s->getOrdElements();

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // This is an enum!
        assert ( !e->isRequired() );
        assert ( !e->isRepeated() );
        assert ( !e->aliasTarget );

        hdr.a ( e->name ).a ( " = " ).a ( e->value );

        if ( i + 1 < elems.size() )
        {
            hdr.a ( "," );
        }

        if ( e->isEnumDefault() || e->comment.length() > 0 || e->extName.length() > 0 )
        {
            hdr.a ( " //" );

            if ( e->comment.length() > 0 || e->extName.length() > 0 )
            {
                hdr.a ( "/<" );

                if ( e->comment.length() > 0 )
                {
                    hdr.a ( String ( " %1" ).arg ( e->comment ) );
                }

                if ( e->extName.length() > 0 )
                {
                    hdr.a ( String ( " String value: '%1'" ).arg ( e->extName ) );
                }
            }

            if ( e->isEnumDefault() )
            {
                hdr.a ( " [default]" );
            }
        }

        hdr.e();
    }

    hdr.decBaseIndent ( 2 );

    hdr.i ( 1 ).a ( "};" ).e();
    hdr.a ( "};" ).e().e();

    hookPosition ( s, hdr, impl, PosBeforeClass );

    hdr.c ( "@brief Class representing '" ).a ( s->getName() ).a ( "' enumerator" ).e();
    hdr.ce();
    hdr.ce ( "It is not a regular enum type, but a 'safe enum wrapper' for improved type safety" );
    hdr.ce ( "It also provides some additional functions." );

    if ( s->getComment().length() > 0 )
    {
        hdr.ce();
        hdr.c ( s->getName() ).a ( ": " ).a ( s->getComment() ).e();
    }

    hdr.a ( "class " ).a ( s->getName() ).a ( ": public " ).a ( String ( "_" ).append ( s->getName() ) ).e();
    hdr.a ( "{" ).e();

    hdr.incBaseIndent();

    hookPosition ( s, hdr, impl, PosClassOpened );

    hdr.a ( "public:" ).e();

    hdr.incBaseIndent();

    hookPosition ( s, hdr, impl, PosPublicBeg );

    hdr.e();
    hdr.c ( "@brief Default constructor of " ).a ( s->getName() ).a ( " 'enum' class" ).e();
    hdr.ce ( "Calls 'clear()'" );
    hdr.a ( "inline " ).a ( s->getName() ).a ( "()" ).e();
    hdr.a ( "{" ).e();
    hdr.i ( 1 ).a ( "clear();" ).e();
    hdr.a ( "}" ).e().e();

    hdr.e();
    hdr.c ( "@brief A constructor of " ).a ( s->getName() ).a ( " 'enum' class" ).e();
    hdr.ce ( "@param [in] useValue The enum's code to set the internal value to" );
    hdr.a ( "inline " ).a ( s->getName() ).a ( " ( " ).a ( INT_ENUM_TYPENAME );
    hdr.a ( " useValue ) : _enumValue ( useValue )" ).e();
    hdr.a ( "{}" ).e().e();

    hdr.ce ( "@brief Returns the internal representation of this enum's value" );
    hdr.ce ( "@return The internal representation of this enum's value" );
    hdr.a ( "inline " ).a ( INT_ENUM_TYPENAME ).a ( " value() const" ).e();
    hdr.a ( "{" ).e();
    hdr.i ( 1 ).a ( "return _enumValue;" ).e();
    hdr.a ( "}" ).e().e();

    hdr.a ( "// We don't provide a cast (to internal enum type) operator." ).e();
    hdr.a ( "// This would mean automatic casting to integers, which is one of the things we want to avoid." ).e();
    hdr.a ( "// If the internal value is needed, the 'value()' method should be used instead." ).e();
    hdr.e();

    hdr.ce ( "@brief 'equal' operator" );
    hdr.ce ( "@param [in] other The other object to compare this one to" );
    hdr.ce ( "@return True if the two enum objects are equal, false otherwise" );
    hdr.a ( "inline bool operator == ( const " ).a ( s->getName() ).a ( " & other ) const" ).e();
    hdr.a ( "{" ).e();
    hdr.i ( 1 ).a ( "return ( _enumValue == other._enumValue );" ).e();
    hdr.a ( "}" ).e().e();

    hdr.ce ( "@brief 'equal' operator" );
    hdr.ce ( "@param [in] enumValue The enum value to compare this object to" );
    hdr.ce ( "@return True if this object contains matching enum value, false otherwise" );
    hdr.a ( "inline bool operator == ( " ).a ( INT_ENUM_TYPENAME ).a ( " enumValue ) const" ).e();
    hdr.a ( "{" ).e();
    hdr.i ( 1 ).a ( "return ( _enumValue == enumValue );" ).e();
    hdr.a ( "}" ).e().e();

    hdr.ce ( "@brief 'not equal' operator" );
    hdr.ce ( "@param [in] other The other object to compare this one to" );
    hdr.ce ( "@return True if the two enum objects are NOT equal, false otherwise" );
    hdr.a ( "inline bool operator != ( const " ).a ( s->getName() ).a ( " & other ) const" ).e();
    hdr.a ( "{" ).e();
    hdr.i ( 1 ).a ( "return ( _enumValue != other._enumValue );" ).e();
    hdr.a ( "}" ).e().e();

    hdr.ce ( "@brief 'not equal' operator" );
    hdr.ce ( "@param [in] enumValue The enum value to compare this object to" );
    hdr.ce ( "@return True if the two enum objects are NOT equal, false otherwise" );
    hdr.a ( "inline bool operator != ( " ).a ( INT_ENUM_TYPENAME ).a ( " enumValue ) const" ).e();
    hdr.a ( "{" ).e();
    hdr.i ( 1 ).a ( "return ( _enumValue != enumValue );" ).e();
    hdr.a ( "}" ).e().e();

    Element * defElem = s->getEnumDefault();

    assert ( defElem != 0 );

    if ( !defElem )
    {
        throw Error ( "Enumerators have to have a default value!" );
    }

    hdr.ce ( "@brief 'Clears' the enum" );
    hdr.c ( "It sets the enum to its default value (" ).a ( defElem->name ).a ( ")" ).e();
    hdr.a ( "inline void clear()" ).e();
    hdr.a ( "{" ).e();
    hdr.i ( 1 ).a ( "_enumValue = " ).a ( defElem->name ).a ( ";" ).e();
    hdr.a ( "}" ).e().e();

    hdr.ce ( "@brief Deserializes the enum" );
    hdr.ce();
    hdr.ce ( "This will check if the deserialized value is legal for this enum" );
    hdr.ce ( "and return error without modifying it if not." );
    hdr.ce ( "@param [in] buf The buffer to deserialize the data from" );
    hdr.ce ( "@param [in] offset Offset in the buffer" );
    hdr.ce ( "@param [in] dataSize Size of data to read (from the offset)" );
    hdr.ce ( "@param [in] wireType Encoding type." );
    hdr.ce ( "@return The error code" );

    hdr.ae ( String ( "%1 deserializeEnum ( %2 buf, size_t offset, size_t dataSize, %3 wireType );" )
             .arg ( getStdType ( TypeErrorCode ), getStdType ( TypeReadBuffer ), getStdType ( TypeWireType ) ) ).e();

    impl.ae ( String ( "%1 %2::deserializeEnum ( %3 buf, size_t offset, size_t dataSize, %4 wireType )" )
              .arg ( getStdType ( TypeErrorCode ), getClassPath ( s ),
                     getStdType ( TypeReadBuffer ), getStdType ( TypeWireType ) ) );
    impl.a ( "{" ).e();

    impl.i ( 1 ).a ( getStdType ( TypeEnum ) ).a ( " tmpVal = " ).a ( defElem->name ).a ( ";" ).e().e();
    impl.i ( 1 ).a ( getStdType ( TypeErrorCode ) ).a ( " ret = " );
    impl.a ( exprProtoDecodeFieldValue ( "buf", "offset", "dataSize", "wireType", "tmpVal" ) ).a ( ";" ).e().e();

    impl.ae ( 1, String ( "if ( ret != %1 )" ).arg ( getErrorCode ( ErrOK ) ) );
    impl.ae ( 2, "return ret;" ).e();

    impl.ae ( 1, "if ( convertFromRaw ( tmpVal, *this ) )" );
    impl.ae ( 2, String ( "return %1;" ).arg ( getErrorCode ( ErrOK ) ) ).e();

    impl.ae ( 1, String ( "_enumValue = %1;" ).arg ( defElem->name ) );
    impl.ae ( 1, String ( "return %1;" ).arg ( getErrorCode ( ErrProtocolWarning ) ) );
    impl.ae ( "}" ).e();

    hdr.ce ( "@brief Returns the description of the enum's value" );
    hdr.ce ( "@return The description of the enum's value" );
    hdr.a ( "inline const char * toString() const" ).e();
    hdr.a ( "{" ).e();
    hdr.i ( 1 ).a ( "return getRawCodeDescription ( _enumValue );" ).e();
    hdr.a ( "}" ).e().e();

    hdr.ce ( "@brief Returns the description of the raw enum's code" );
    hdr.ce ( "@return The description of the raw enum's code" );
    hdr.a ( "static const char * getRawCodeDescription( " );
    hdr.a ( getStdType ( TypeEnum ) ).a ( " rawValue );" ).e().e();

    impl.a ( "const char * " ).a ( getClassPath ( s ) ).a ( "::getRawCodeDescription( " );
    impl.a ( getStdType ( TypeEnum ) ).a ( " rawValue )" ).e();
    impl.a ( "{" ).e();
    impl.i ( 1 ).a ( "switch ( rawValue )" ).e();
    impl.i ( 1 ).a ( "{" ).e();

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // This is an enum!
        assert ( !e->isRepeated() );
        assert ( !e->aliasTarget );

        impl.ae ( 2, String ( "case %1: return \"%2\";" )
                  .arg ( e->name, e->extName.isEmpty() ? e->name : e->extName ) );
    }

    impl.ae ( 2, "default: return \"Unknown\";" );
    impl.ae ( 1, "}" );
    impl.ae ( "}" ).e();

    hdr.ce ( "@brief Converts the raw code value to enum" );
    hdr.ce();
    hdr.ce ( "This will check if the deserialized value is legal for this enum" );
    hdr.ce ( "and return error without modifying it if not." );
    hdr.ce ( "@param [in] rawValue The raw value of the enum code to be converted" );
    hdr.ce ( "@param [out] enumValue The converted enum value. If the raw value is incorrect," );
    hdr.ce ( "                        enumValue will NOT be modified" );
    hdr.ce ( "@return True if the rawValue was one of the correct values (and the enumValue was set);" );
    hdr.ce ( "        False otherwise" );
    hdr.a ( "static bool convertFromRaw ( " ).a ( getStdType ( TypeEnum ) ).a ( " rawValue, " );
    hdr.a ( s->getName() ).a ( " & enumValue );" ).e().e();

    impl.a ( "bool " ).a ( getClassPath ( s ) ).a ( "::convertFromRaw ( " );
    impl.a ( getStdType ( TypeEnum ) ).a ( " rawValue, " );
    impl.a ( s->getName() ).a ( " & enumValue )" ).e();
    impl.a ( "{" ).e();

    impl.i ( 1 ).a ( "switch ( rawValue )" ).e();
    impl.i ( 1 ).a ( "{" ).e();

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // This is an enum!
        assert ( !e->isRepeated() );
        assert ( !e->aliasTarget );

        impl.i ( 2 ).a ( "case " ).a ( e->name ).a ( ":" ).e();
        impl.i ( 3 ).a ( "enumValue = " ).a ( e->name ).a ( ";" ).e();
        impl.i ( 3 ).a ( "return true;" ).e();
        impl.i ( 2 ).a ( "break;" ).e();
    }

    impl.i ( 1 ).a ( "}" ).e().e();
    impl.i ( 1 ).a ( "return false;" ).e();
    impl.a ( "}" ).e().e();

    hookPosition ( s, hdr, impl, PosPublicEnd );

    hdr.i ( -1 ).a ( "private:" ).e();

    hookPosition ( s, hdr, impl, PosPrivateBeg );

    hdr.a ( INT_ENUM_TYPENAME ).a ( " _enumValue; ///< Internal value of this 'enum'" ).e();

    hookPosition ( s, hdr, impl, PosPrivateEnd );

    hdr.decBaseIndent();
    hdr.decBaseIndent();

    hdr.ae ( "};" ).e();

    hookPosition ( s, hdr, impl, PosClassClosed );

    _intEnumTypes.append ( s );
}

void CppGenerator::genMessageClass ( Symbol * s, CppFile & hdr, CppFile & impl )
{
    hookPosition ( s, hdr, impl, PosBeforeClass );

    // ***************************************************************************************************************
    // ****************************************************** Header *************************************************
    // ***************************************************************************************************************

    hdr.ce ( String ( "@brief Class representing '%1' %2" )
             .arg ( s->getName(), s->isStruct() ? "structure" : "message" ) );

    if ( s->getComment().length() > 0 )
    {
        hdr.ce();
        hdr.c ( s->getName() ).a ( ": " ).a ( s->getComment() ).e();
    }

    genClassHeader ( s, hdr, impl );

    hdr.a ( "{" ).e();

    hdr.incBaseIndent();

    hookPosition ( s, hdr, impl, PosClassOpened );

    // ***************************************************************************************************************
    // ********************************************** "public:" section **********************************************
    // ***************************************************************************************************************

    hdr.a ( "public:" ).e();

    hdr.incBaseIndent();

    hookPosition ( s, hdr, impl, PosPublicBeg );

    // ***************************************************************************************************************
    // ********************************************** Field Id values ************************************************
    // ***************************************************************************************************************

    assert ( s != 0 );

    const StringList & elems = s->getOrdElements();
    String classPath = getClassPath ( s );

    bool addedMsvcDef = false;

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // We don't have field codes for aliases!
        if ( e->aliasTarget != 0 )
        {
            continue;
        }

        hdr.ae ( String ( "static const %1 %2 = %3; ///< The Field ID of %4" )
                 .arg ( getStdType ( TypeFieldId ), getFieldIdName ( e ), String::number ( e->code ), e->name ) );

        if ( !addedMsvcDef )
        {
            addedMsvcDef = true;

            impl.ae ( "// MSVC doesn't like static const integrals defined in implementation files" );
            impl.ae ( "// It doesn't follow C++ spec (9.4.2/4), but there is not much we can do about it..." );
            impl.ae ( "#ifndef _MSC_VER" );
        }

        impl.ae ( String ( "const %1 %2::%3;" )
                  .arg ( getStdType ( TypeFieldId ), classPath, getFieldIdName ( e ) ) );
    }

    if ( addedMsvcDef )
    {
        impl.ae ( "#endif" );
    }

    hdr.e();
    impl.e();

    // ***************************************************************************************************************
    // **************************************** Defined static const values ******************************************
    // ***************************************************************************************************************

    assert ( s != 0 );

    StringList extraImplLines;
    addedMsvcDef = false;

    for ( HashMap<String, Element * >::Iterator defsIt ( s->getDefines() ); defsIt.isValid(); defsIt.next() )
    {
        Element * e = defsIt.value();

        assert ( e != 0 );

        Element * d = e->definedTarget;

        if ( !d )
        {
            continue;
        }

        String varType = getVarType ( hdr, d );
        String varValue = e->value;

        bool valInHeader = false;

        if ( d->aliasTarget != 0 )
        {
            valInHeader = true;
        }
        else
        {
            assert ( d->typeSymbol != 0 );

            // We don't put floating point values in the header.
            if ( d->typeSymbol->isInteger() )
            {
                valInHeader = true;
            }

            if ( d->typeSymbol->isEnum() )
            {
                // We use the enum's code. We need the full path to that enum's element:
                varValue = getClassPath ( d->typeSymbol );
                varValue.append ( "::" );
                varValue.append ( e->value );
            }
        }

        if ( valInHeader )
        {
            hdr.ae ( String ( "static const %1 %2 = %3;" ).arg ( varType, getDefName ( e ), varValue ) );

            if ( !addedMsvcDef )
            {
                addedMsvcDef = true;

                impl.ae ( "// MSVC doesn't like static const integrals defined in implementation files" );
                impl.ae ( "// It doesn't follow C++ spec (9.4.2/4), but there is not much we can do about it..." );
                impl.ae ( "#ifndef _MSC_VER" );
            }

            impl.ae ( String ( "const %1 %2::%3;" ).arg ( varType, classPath, getDefName ( e ) ) );
        }
        else
        {
            hdr.ae ( String ( "static const %1 %2;" ).arg ( varType, getDefName ( e ) ) );

            extraImplLines.append ( String ( "const %1 %2::%3 ( %4 );" )
                                    .arg ( varType, classPath, getDefName ( e ), varValue ) );
        }
    }

    if ( addedMsvcDef )
    {
        impl.ae ( "#endif" ).e();
    }

    for ( size_t i = 0; i < extraImplLines.size(); ++i )
    {
        impl.ae ( extraImplLines.at ( i ) );
    }

    hdr.e();
    impl.e();

    // ***************************************************************************************************************
    // ********************************************** Internal symbols ***********************************************
    // ***************************************************************************************************************

    const StringList & intSymbols = s->getOrdInternalSymbols();

    for ( size_t i = 0; i < intSymbols.size(); ++i )
    {
        Symbol * intSym = s->getInternalSymbols().value ( intSymbols[ i ] );

        assert ( intSym != 0 );

        genRegularSymbol ( intSym, hdr, impl );
    }

    // We need to set present bit indices for each elements that require that:

    int nextPresenceIdx = 0;

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // We don't need presence bits for lists and for aliases!
        if ( e->isRepeated() || e->aliasTarget != 0 )
        {
            continue;
        }

        e->presenceIndex = nextPresenceIdx++;
    }

    // ***************************************************************************************************************
    // ************************** Public methods for field operations ************************************************
    // ***************************************************************************************************************

    genMsgFieldMethods ( s, hdr, impl, AccessPublic );

    genMsgStdMethods ( s, hdr, impl );

    hookPosition ( s, hdr, impl, PosPublicEnd );

    // ***************************************************************************************************************
    // ********************************************** "protected:" section *******************************************
    // ***************************************************************************************************************

    hdr.i ( -1 ).a ( "protected:" ).e();

    hookPosition ( s, hdr, impl, PosProtectedBeg );

    genMsgFieldMethods ( s, hdr, impl, AccessProtected );

    hookPosition ( s, hdr, impl, PosProtectedEnd );

    // ***************************************************************************************************************
    // *********************************************** "private:" section ********************************************
    // ***************************************************************************************************************

    hdr.i ( -1 ).a ( "private:" ).e();

    hookPosition ( s, hdr, impl, PosPrivateBeg );

    genMsgFields ( s, hdr, impl );

    genMsgFieldMethods ( s, hdr, impl, AccessPrivate );

    genMsgStdPrivMethods ( s, hdr, impl );

    hookPosition ( s, hdr, impl, PosPrivateEnd );

    hdr.decBaseIndent();
    hdr.decBaseIndent();

    hdr.a ( "};" ).e();

    hookPosition ( s, hdr, impl, PosClassClosed );
}

void CppGenerator::genMsgFieldMethods (
        Symbol * s, CppFile & hdr, CppFile & /*impl*/, const CppGenerator::AccessType &
        accType )
{
    assert ( s != 0 );

    const StringList & elems = s->getOrdElements();

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        if ( accType == AccessPublic )
        {
            hdr.c ( "@brief Reads the value of '" ).a ( e->getCamelCaseName() ).a ( "'" ).e();

            if ( e->aliasTarget != 0 )
            {
                hdr.c ( "This is the value of a bitrange inside the underlying storage type '" );
                hdr.a ( e->aliasTarget->getCamelCaseName() ).a ( "'" ).e();

                if ( e->isSalias() )
                {
                    hdr.ce ( "@note This is an 's-alias', which means it uses one additional presence bit" );
                }
            }

            hdr.genVarComments ( e );

            hdr.c ( "@return The value of '" ).a ( e->getCamelCaseName() ).a ( "'" ).e();

            if ( !e->aliasTarget )
            {
                hdr.a ( "inline " );

                if ( e->typeSymbol != 0
                     && ( e->typeSymbol->isInteger()
                          || e->typeSymbol->isFloatingPoint()
                          || e->typeSymbol->isEnum() ) )
                {
                    hdr.a ( getVarType ( hdr, e, VarUseGetter ) ).a ( " " );
                }
                else
                {
                    hdr.a ( "const " ).a ( getVarType ( hdr, e, VarUseGetter ) ).a ( " & " );
                }

                hdr.a ( e->getCamelCaseName ( "get" ) ).a ( "() const" ).e();
            }
            else
            {
                hdr.a ( "inline " ).a ( getVarType ( hdr, e, VarUseGetter ) );
                hdr.a ( " " ).a ( e->getCamelCaseName ( "get" ) );
                hdr.a ( "() const" ).e();
            }

            hdr.a ( "{" ).e();
            hdr.a ( 1, "return ( " );

            if ( !e->aliasTarget )
            {
                // Regular element
                hdr.a ( getVarName ( e ) );
            }
            else
            {
                assert ( e->aliasTarget != 0 );
                assert ( e->getAliasPayloadBitLength() > 0 );

                const int numBits = e->getAliasPayloadBitLength();

                assert ( numBits > 0 );

                hdr.a ( String ( "( ( %1() >> %2 ) & %3 )" )
                        .arg ( e->aliasTarget->getCamelCaseName ( "get" ) )
                        .arg ( e->iAliasPayloadRangeFrom )
                        .arg ( getBitmask ( numBits ) ) );
            }

            hdr.ae ( " );" );
            hdr.ae ( "}" ).e();

            hdr.ce ( String ( "@brief Checks if '%1' is set (present) in the %2" )
                     .arg ( s->getName(), s->isStruct() ? "structure" : "message" ) );

            if ( e->isSalias() )
            {
                hdr.ce ( "@note This is an 's-alias', which means it uses one additional presence bit" );
            }

            hdr.ce ( "@return True if the value is set (present), false otherwise" );
            hdr.a ( "inline bool " ).a ( e->getCamelCaseName ( "has" ) ).a ( "() const" ).e();
            hdr.a ( "{" ).e();

            hdr.a ( 1, "return ( " );

            if ( e->isRepeated() )
            {
                hdr.a ( String ( "%1 > 0" ).arg ( exprListVarSize ( e->typeSymbol, getVarName ( e ) ) ) );
            }
            else if ( e->aliasTarget != 0 )
            {
                hdr.a ( String ( "%1()" ).arg ( e->aliasTarget->getCamelCaseName ( "has" ) ) );

                if ( e->isSalias() )
                {
                    hdr.a ( String ( " && ( ( ( %1() >> %2 ) & 0x01 ) == 0x01 )" )
                            .arg ( e->aliasTarget->getCamelCaseName ( "get" ) )
                            .arg ( e->iAliasRangeFrom ) );
                }
            }
            else
            {
                assert ( e->presenceIndex >= 0 );

                hdr.a ( String ( "( ( ( %1 >> %2 ) & 0x01 ) == 0x01 )" )
                        .arg ( getPresVarNameIdx ( e->presenceIndex ) )
                        .arg ( getPresVarShift ( e->presenceIndex ) ) );
            }

            hdr.ae ( " );" );
            hdr.ae ( "}" ).e();
        }

        if ( ( accType == AccessPublic && e->isPublic() )
             || ( accType == AccessProtected && e->isProtected() )
             || ( accType == AccessPrivate && e->isPrivate() ) )
        {
            bool addUnset = false;
            bool addSet = false;

            if ( e->aliasTarget != 0 )
            {
                // No 'repeated' aliases!
                assert ( !e->isRepeated() );

                hdr.c ( "@brief Sets the value of bit range (alias) '" ).a ( e->getCamelCaseName() ).a ( "'" ).e();

                if ( e->isSalias() )
                {
                    hdr.ce ( "@note This is an 's-alias', which means it uses one additional presence bit" );
                }

                hdr.genVarComments ( e );
                hdr.ce ( "@param [in] newValue The value to set" );

                if ( !e->usesFullType() )
                {
                    hdr.ce ( "@param [out] validValue If used, it will be set to true if the value was set properly," );
                    hdr.ce ( "                         and to false if the new value is not in allowed range" );
                    hdr.ce ( "@return reference to this class (for chaining)" );

                    hdr.ae ( String ( "inline %1 & %2 ( %3 newValue, bool * validValue = 0 )" )
                             .arg ( getClassPath ( s ),
                                    e->getCamelCaseName ( "bset" ),
                                    getVarType ( hdr, e, VarUseSetter ) ) );
                }
                else
                {
                    hdr.ce ( "@return reference to this class (for chaining)" );

                    hdr.ae ( String ( "inline %1 & %2 ( %3 newValue )" )
                             .arg ( getClassPath ( s ),
                                    e->getCamelCaseName ( "set" ),
                                    getVarType ( hdr, e, VarUseSetter ) ) );
                }

                hdr.ae ( "{" );

                const int numBits = e->getAliasPayloadBitLength();

                assert ( numBits > 0 );

                String bMask = getBitmask ( numBits );

                if ( !e->usesFullType() )
                {
                    hdr.ae ( 1, "// It's quite possible, that the type used for the argument" );
                    hdr.ae ( 1, "// of this function is bigger then the bit length of this field." );
                    hdr.ae ( 1, "// We can use the bitmask to see if the actual value can fit in the number" );
                    hdr.ae ( 1, "// of bits that we have! If not - set 'valid' to false and don't do anything" );
                    hdr.ae ( 1, String ( "if ( ( newValue & %1 ) != newValue )" ).arg ( bMask ) );
                    hdr.ae ( 1, "{" );
                    hdr.ae ( 2, "if ( validValue != 0 ) *validValue = false;" );
                    hdr.ae ( 2, "return *this;" );
                    hdr.ae ( 1, "}" ).e();
                }

                // We want to get this:
                // ( ( x & (~ ( bmask << payloadrangefrom ) ) ) | ( ( newval & bmask ) << payloadrangefrom ) )
                // and optional: | ( 1 << rangefrom )

                if ( e->isSalias() )
                    hdr.ae ( 1, "// We also set the presence bit, since this is an s-alias:" );

                hdr.a ( 1, String ( "%1 ( ( %2() & ( ~( %3 << %4 ) ) ) | ( ( newValue & %3 ) << %4 )" )
                        .arg ( e->aliasTarget->getCamelCaseName ( "set" ) )
                        .arg ( e->aliasTarget->getCamelCaseName ( "get" ) )
                        .arg ( bMask )
                        .arg ( e->iAliasPayloadRangeFrom ) );

                if ( e->isSalias() )
                {
                    hdr.a ( String ( " | ( 0x01 << %1 )" ).arg ( e->iAliasRangeFrom ) );

                    addUnset = true;
                }

                hdr.ae ( " );" ).e();

                if ( !e->usesFullType() )
                {
                    hdr.ae ( 1, "if ( validValue != 0 ) *validValue = true;" );
                }

                genObjectModified ( s, hdr, 1 );

                hdr.ae ( 1, "return *this;" );
                hdr.ae ( "}" ).e();
            }
            else
            {
                // For everything that is NOT an alias, we want to have 'set' and 'unset' function.

                assert ( !e->aliasTarget );

                addUnset = true;
                addSet = true;

                if ( e->isRepeated() || ( e->typeSymbol != 0 && e->typeSymbol->isStruct() ) )
                {
                    // For ALL repeated elements, and for non-repeated structures we want to include 'mod' function.
                    // Messages are stored using their base type, and we don't want to expose them like that.

                    hdr.c ( "@brief Returns a writable reference to '" ).a ( e->getCamelCaseName() );
                    hdr.a ( "' field" ).e();
                    hdr.ce ( "It should be used to modify the value of that field" );
                    hdr.ce ( "Calling this function will mark the field as 'set'," );
                    hdr.ce ( "whether the value is actually modified or not" );
                    hdr.c ( "For read-only access use " ).a ( e->getCamelCaseName ( "get" ) ).a ( "() instead" ).e();

                    hdr.genVarComments ( e );

                    hdr.c ( "@return The reference to '" ).a ( e->getCamelCaseName() ).a ( "'" ).e();

                    hdr.a ( "inline " ).a ( getVarType ( hdr, e, VarUseGetter ) ).a ( " & " );
                    hdr.a ( e->getCamelCaseName ( "mod" ) ).a ( "()" ).e();
                    hdr.a ( "{" ).e();

                    if ( !e->isRepeated() )
                    {
                        // 'presence' of repeated fields is determined by the size of the list,
                        // we need to do this only for regular fields!

                        assert ( e->presenceIndex >= 0 );

                        hdr.i ( 1 ).a ( getPresVarNameIdx ( e->presenceIndex ) ).a ( " |= ( 1 << " );
                        hdr.a ( getPresVarShift ( e->presenceIndex ) ).a ( " );" ).e();
                    }

                    genObjectModified ( s, hdr, 1 );

                    hdr.i ( 1 ).a ( "return " ).a ( getVarName ( e ) ).a ( ";" ).e();
                    hdr.a ( "}" ).e().e();
                }
            }

            if ( addSet )
            {
                hdr.c ( "@brief Sets the value of '" ).a ( e->getCamelCaseName() ).a ( "'" ).e();
                hdr.genVarComments ( e );
                hdr.ce ( "@param [in] newValue The value to set" );
                hdr.ce ( "@return reference to this class (for chaining)" );
                hdr.a ( String ( "inline %1 & %2 ( " ).arg ( getClassPath ( s ), e->getCamelCaseName ( "set" ) ) );

                if ( e->typeSymbol != 0
                     && ( e->typeSymbol->isInteger()
                          || e->typeSymbol->isFloatingPoint()
                          || e->typeSymbol->isEnum() ) )
                {
                    hdr.a ( getVarType ( hdr, e, VarUseSetter ) );
                }
                else
                {
                    hdr.a ( "const " ).a ( getVarType ( hdr, e, VarUseSetter ) ).a ( " & " );
                }

                hdr.a ( " newValue" ).ae ( " )" );
                hdr.ae ( "{" );
                hdr.ae ( 1, String ( "%1 = newValue;" ).arg ( getVarName ( e ) ) );

                if ( !e->isRepeated() )
                {
                    // 'presence' of repeated fields is determined by the size of the list,
                    // we need to do this only for regular fields!

                    assert ( e->presenceIndex >= 0 );

                    hdr.i ( 1 ).a ( getPresVarNameIdx ( e->presenceIndex ) ).a ( " |= ( 1 << " );
                    hdr.a ( getPresVarShift ( e->presenceIndex ) ).a ( " );" ).e().e();
                }

                genObjectModified ( s, hdr, 1 );

                hdr.i ( 1 ).a ( "return *this;" ).e();
                hdr.a ( "}" ).e().e();
            }

            if ( addUnset )
            {
                hdr.c ( "@brief Clears the value of '" ).a ( e->getCamelCaseName() ).a ( "''" ).e();

                if ( e->isRepeated() )
                {
                    hdr.ce ( "This clears the list of elements stored in this field" );
                }
                else if ( !e->isAlias() )
                {
                    hdr.ce ( "It restores the default value and clears the presence bit." );
                }

                hdr.c ( "From now on, the " ).a ( e->getCamelCaseName ( "has" ) );
                hdr.a ( "() function will return false" ).e();
                hdr.a ( "inline void " ).a ( e->getCamelCaseName ( "clear" ) ).a ( "()" ).e();
                hdr.a ( "{" ).e();

                if ( e->isSalias() )
                {
                    hdr.ae ( 1, "// Nothing to do if the underlying field is not set (we don't want to set it)" );

                    hdr.ae ( 1, String ( "if ( !%1() ) return;" )
                             .arg ( e->aliasTarget->getCamelCaseName ( "has" ) ) ).e();

                    hdr.ae ( 1, String ( "%1 ( %2() & ( ~( %3 << %4 ) ) );" )
                             .arg ( e->aliasTarget->getCamelCaseName ( "set" ) )
                             .arg ( e->aliasTarget->getCamelCaseName ( "get" ) )
                             .arg ( getBitmask ( e->getAliasPayloadBitLength() + 1 ) )
                             .arg ( e->iAliasRangeFrom ) );
                }
                else if ( !e->isRepeated() )
                {
                    assert ( e->presenceIndex >= 0 );

                    hdr.i ( 1 ).a ( getPresVarNameIdx ( e->presenceIndex ) ).a ( " &= ~( 1 << " );
                    hdr.a ( getPresVarShift ( e->presenceIndex ) ).a ( " );" ).e();
                }

                if ( !e->isSalias() )
                {
                    hdr.i ( 1 ).a ( exprVarClear ( e ) ).a ( ";" ).e();
                }

                genObjectModified ( s, hdr, 1 );

                hdr.a ( "}" ).e().e();
            }
        }
    }
}

void CppGenerator::genMsgFields ( Symbol * s, CppFile & hdr, CppFile & /* impl */ )
{
    assert ( s != 0 );

    const StringList & elems = s->getOrdElements();

    int maxUsedPresenceIdx = -1;

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // Aliases don't have their own fields!
        if ( e->aliasTarget != 0 )
        {
            continue;
        }

        if ( e->presenceIndex > maxUsedPresenceIdx )
        {
            maxUsedPresenceIdx = e->presenceIndex;
        }

        String comment ( ";" );

        if ( e->comment.length() > 0 || e->isRequired() )
        {
            comment.append ( " //" );

            if ( e->comment.length() > 0 )
            {
                comment.append ( "/< " );
                comment.append ( e->comment );
            }

            if ( e->isRequired() )
            {
                comment.append ( " [required field]" );
            }
        }

        hdr.a ( getVarType ( hdr, e ) ).a ( " " ).a ( getVarName ( e ) ).a ( comment ).e();

        if ( e->typeSymbol->isMessageOrStruct() || e->typeSymbol->isEnum() )
        {
            // We need to add an include to add the header file that declares e->typeSymbol.
            // However, if that symbol is our current symbol (or declared inside of it),
            // we don't need to add that include. Let's check if that's the case:

            Symbol * tmpSym = e->typeSymbol;

            while ( tmpSym != 0 && tmpSym != s )
            {
                tmpSym = tmpSym->getParent();
            }

            if ( !tmpSym )
            {
                // We didn't find 's' among our parents, let's add the include:

                hdr.addCppInclude ( getFilePath ( e->typeSymbol, ExtHeader, s ), CppFile::IncludeProto );
            }
        }
    }

    int usedPresenceVars = 0;

    if ( maxUsedPresenceIdx >= 0 )
    {
        usedPresenceVars = 1 + ( maxUsedPresenceIdx / getPresVarSize() );
    }

    for ( int i = 0; i < usedPresenceVars; ++i )
    {
        hdr.a ( getPresVarType() ).a ( " " ).a ( getPresVarNameNum ( i ) );
        hdr.a ( "; ///< Used for storing presence bits" ).e();
    }

    if ( usedPresenceVars > 0 )
    {
        hdr.e();
    }
}

void CppGenerator::genMsgStdMethods ( Symbol * s, CppFile & hdr, CppFile & impl )
{
    assert ( s != 0 );

    const String classPath = getClassPath ( s );
    const StringList elems = s->getOrdElements();

    // ***************************************************************************************************************
    // *********************************************** Default constructor *******************************************
    // ***************************************************************************************************************

    hdr.ce ( "@brief Default constructor" );
    hdr.ce();
    hdr.ce ( "Calls 'localClear()' method" );
    hdr.a ( s->getName() ).a ( "();" ).e().e();

    impl.a ( classPath ).a ( "::" ).a ( s->getName() ).a ( "()" ).e();
    impl.ae ( "{" );
    impl.ae ( 1, "// Local version, other constructors should call their own localClear() functions too" );
    impl.ae ( 1, "localClear();" );
    impl.ae ( "}" ).e();

    // ***************************************************************************************************************
    // **************************************************** clear() **************************************************
    // ***************************************************************************************************************

    hdr.ae ( "virtual void clear();" ).e();

    impl.a ( "void " ).a ( classPath ).a ( "::clear()" ).e();
    impl.a ( "{" ).e();

    if ( s->getInheritance() != 0 )
    {
        impl.a ( 1, s->getInheritance()->getName() );
        impl.ae ( "::clear();" ).e();
    }

    impl.ae ( 1, "localClear();" );
    impl.ae ( "}" ).e();

    // ***************************************************************************************************************
    // *************************************************** validate() ************************************************
    // ***************************************************************************************************************

    hdr.ae ( String ( "virtual %1 validate ( %2 * extError = 0 ) const;" )
             .arg ( getStdType ( TypeErrorCode ), getStdType ( TypeExtError ) ) ).e();

    impl.ae ( String ( "%1 %2::validate ( %3 * extError ) const" )
              .arg ( getStdType ( TypeErrorCode ), classPath, getStdType ( TypeExtError ) ) );
    impl.ae ( "{" );
    impl.incBaseIndent ( 1 );

    if ( s->getInheritance() != 0 )
    {
        impl.ae ( String ( "%1 ret = %2::validate ( extError );" )
                  .arg ( getStdType ( TypeErrorCode ), s->getInheritance()->getName() ) ).e();

        impl.ae ( String ( "if ( ret != %1 )" ).arg ( getErrorCode ( ErrOK ) ) );
        impl.ae ( "{" );

        genSetupExtError ( impl, 1, "ret",
                           String ( "\"Error validating base class %1 in %2::validate()\"" )
                           .arg ( getClassPath ( s->getInheritance() ), getClassPath ( s ) ) );

        impl.ae ( 1, "return ret;" );
        impl.ae ( "}" );
    }
    else
    {
        impl.ae ( "( void ) extError;" ).e();
    }

    for ( HashMap<String, Element * >::Iterator defsIt ( s->getDefines() ); defsIt.isValid(); defsIt.next() )
    {
        Element * e = defsIt.value();

        assert ( e != 0 );

        if ( !e->definedTarget )
        {
            continue;
        }

        impl.ae ( String ( "if ( !%1() || %2() != %3 )" )
                  .arg ( e->getCamelCaseName ( "has" ),
                         e->getCamelCaseName ( "get" ),
                         getDefName ( e ) ) );
        impl.ae ( "{" );
        impl.incBaseIndent ( 1 );

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
                           .arg ( classPath, e->name, defNameExpr, "%1" ), false );

        impl.ae ( 1, "else" );

        genSetupExtError ( impl, 1,
                           getErrorCode ( ErrDefinedValueMismatch ),
                           String ( "String ( \"%1.%2 is set to '%5', instead of being set to '%6'\" )"
                                    ".arg ( %3 ).arg ( %4 )" )
                           .arg ( classPath, e->name, defValueExpr, defNameExpr, "%1", "%2" ), false );

        impl.ae ( "}" );
        impl.ae ( String ( "return %1;" ).arg ( getErrorCode ( ErrDefinedValueMismatch ) ) );
        impl.decBaseIndent ( 1 );
        impl.ae ( "}" );
    }

    impl.e();

    bool addedTmpRetVar = false;

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );
        String varName = getVarName ( e );
        String hasVarTest ( String ( e->getCamelCaseName ( "has" ) ).append ( "() && " ) );

        assert ( e != 0 );

        impl.e();

        if ( e->isRequired() )
        {
            impl.ae ( String ( "if ( !%1() )" ).arg ( e->getCamelCaseName ( "has" ) ) );
            impl.ae ( "{" );

            genSetupExtError ( impl, 1,
                               getErrorCode ( ErrRequiredFieldNotSet ),
                               String ( "\"Required field %1.%2 is not set\"" )
                               .arg ( classPath, e->name ) );

            impl.ae ( 1, String ( "return %1;" ).arg ( getErrorCode ( ErrRequiredFieldNotSet ) ) );
            impl.ae ( "}" );
        }

        bool insideLoop = false;

        if ( e->isRepeated() )
        {
            if ( e->options.contains ( ProtocolSpec::KW_MIN_LIST_SIZE ) )
            {
                impl.ae ( String ( "if ( %1 < %2 )" )
                          .arg ( exprListVarSize ( e->typeSymbol, varName ),
                                 e->options[ ProtocolSpec::KW_MIN_LIST_SIZE ] ) );
                impl.ae ( "{" );

                genSetupExtError (
                    impl, 1,
                    getErrorCode ( ErrListSizeOutOfRange ),
                    String ( "String ( \"%1.%2 has %5 elements, but it should have at least %3\" ).arg ( %4 )" )
                    .arg ( classPath, e->name, e->options[ ProtocolSpec::KW_MIN_LIST_SIZE ],
                           exprListVarSize ( e->typeSymbol, varName ), "%1" ) );

                impl.ae ( 1, String ( "return %1;" ).arg ( getErrorCode ( ErrListSizeOutOfRange ) ) );
                impl.ae ( "}" );
            }

            if ( e->options.contains ( ProtocolSpec::KW_MAX_LIST_SIZE ) )
            {
                impl.ae ( String ( "if ( %1 > %2 )" )
                          .arg ( exprListVarSize ( e->typeSymbol, varName ),
                                 e->options[ ProtocolSpec::KW_MAX_LIST_SIZE ] ) );
                impl.ae ( "{" );

                genSetupExtError (
                    impl, 1,
                    getErrorCode ( ErrListSizeOutOfRange ),
                    String ( "String ( \"%1.%2 has %5 elements, but it should have at most %3\" ).arg ( %4 )" )
                    .arg ( classPath, e->name, e->options[ ProtocolSpec::KW_MAX_LIST_SIZE ],
                           exprListVarSize ( e->typeSymbol, varName ), "%1" ) );

                impl.ae ( 1, String ( "return %1;" ).arg ( getErrorCode ( ErrListSizeOutOfRange ) ) );
                impl.ae ( "}" );
            }

            // We set this AFTER the list size stuff. We wanted to use the 'list' variable,
            // but from now on we want to use the 'varRef' instead
            varName = "varRef";

            // No need to test whether we have it or not - we are iterating over a list of those elements
            hasVarTest = "";

            impl.e();

            if ( e->options.contains ( ProtocolSpec::KW_MIN_LENGTH )
                 || e->options.contains ( ProtocolSpec::KW_MAX_LENGTH )
                 || e->options.contains ( ProtocolSpec::KW_MIN )
                 || e->options.contains ( ProtocolSpec::KW_MAX )
                 || ( e->typeSymbol != 0 && e->typeSymbol->isMessageOrStruct() ) )
            {
                insideLoop = true;

                impl.ae ( String ( "for ( size_t i = 0, lSize = %1; i < lSize; ++i )" )
                          .arg ( exprListVarSize ( e->typeSymbol, getVarName ( e ) ) ) );
                impl.ae ( "{" );
                impl.incBaseIndent ( 1 );

                impl.ae ( String ( "const %1 & %2 = %3;" )
                          .arg ( getRawVarType ( hdr, e->typeSymbol ),
                                 varName,
                                 exprListGetElemIdxRef ( e->typeSymbol, getVarName ( e ), "i" ) ) );
            }
        }

        if ( e->options.contains ( ProtocolSpec::KW_MIN_LENGTH ) )
        {
            impl.ae ( String ( "if ( %1%2 < %3 )" )
                      .arg ( hasVarTest, exprStringVarLength ( varName ), e->options[ ProtocolSpec::KW_MIN_LENGTH ] ) );
            impl.ae ( "{" );

            genSetupExtError (
                impl, 1,
                getErrorCode ( ErrStringLengthOutOfRange ),
                String ( "String ( \"%1.%2 has length %5, but it should have at least %3\" ).arg ( %4 )" )
                .arg ( classPath, e->name, e->options[ ProtocolSpec::KW_MIN_LENGTH ],
                       exprStringVarLength ( varName ), "%1" ) );

            impl.ae ( 1, String ( "return %1;" ).arg ( getErrorCode ( ErrStringLengthOutOfRange ) ) );
            impl.ae ( "}" );
        }

        if ( e->options.contains ( ProtocolSpec::KW_MAX_LENGTH ) )
        {
            impl.ae ( String ( "if ( %1%2 > %3 )" )
                      .arg ( hasVarTest, exprStringVarLength ( varName ), e->options[ ProtocolSpec::KW_MAX_LENGTH ] ) );
            impl.ae ( "{" );

            genSetupExtError (
                impl, 1,
                getErrorCode ( ErrStringLengthOutOfRange ),
                String ( "String ( \"%1.%2 has length %5, but it should have at most %3\" ).arg ( %4 )" )
                .arg ( classPath, e->name, e->options[ ProtocolSpec::KW_MAX_LENGTH ],
                       exprStringVarLength ( varName ), "%1" ) );

            impl.ae ( 1, String ( "return %1;" ).arg ( getErrorCode ( ErrStringLengthOutOfRange ) ) );
            impl.ae ( "}" );
        }

        if ( e->options.contains ( ProtocolSpec::KW_MIN ) )
        {
            impl.ae ( String ( "if ( %1%2 < %3 )" )
                      .arg ( hasVarTest, varName, e->options[ ProtocolSpec::KW_MIN ] ) );
            impl.ae ( "{" );

            genSetupExtError (
                impl, 1,
                getErrorCode ( ErrFieldValueOutOfRange ),
                String ( "String ( \"%1.%2 has value %5, but it should be at least %3\" ).arg ( %4 )" )
                .arg ( classPath, e->name, e->options[ ProtocolSpec::KW_MIN ], varName, "%1" ) );

            impl.ae ( 1, String ( "return %1;" ).arg ( getErrorCode ( ErrFieldValueOutOfRange ) ) );
            impl.ae ( "}" );
        }

        if ( e->options.contains ( ProtocolSpec::KW_MAX ) )
        {
            impl.ae ( String ( "if ( %1%2 > %3 )" )
                      .arg ( hasVarTest, varName, e->options[ ProtocolSpec::KW_MAX ] ) );
            impl.ae ( "{" );

            genSetupExtError (
                impl, 1,
                getErrorCode ( ErrFieldValueOutOfRange ),
                String ( "String ( \"%1.%2 has value %5, but it should be at most %3\" ).arg ( %4 )" )
                .arg ( classPath, e->name, e->options[ ProtocolSpec::KW_MAX ], varName, "%1" ) );

            impl.ae ( 1, String ( "return %1;" ).arg ( getErrorCode ( ErrFieldValueOutOfRange ) ) );
            impl.ae ( "}" );
        }

        // We need to call 'validate' in embedded messages/structs
        if ( e->typeSymbol != 0 && e->typeSymbol->isMessageOrStruct() )
        {
            if ( !addedTmpRetVar )
            {
                if ( !insideLoop )
                {
                    // If we are adding this field inside the loop, we don't mark it as 'added',
                    // because it won't be visible outside of this loop...
                    addedTmpRetVar = true;
                }

                impl.ae ( String ( "%1 tmpRetVar;" ).arg ( getStdType ( TypeErrorCode ) ) ).e();
            }

            impl.ae ( String ( "if ( %1( tmpRetVar = %2.validate ( extError ) ) != %3 )" )
                      .arg ( hasVarTest, varName, getErrorCode ( ErrOK ) ) );
            impl.ae ( "{" );

            if ( e->isRepeated() )
            {
                genSetupExtError (
                    impl, 1, "tmpRetVar",
                    String ( "String ( \"Internal repeated object %1.%2 (index: %3) "
                             "did not validate properly\" ).arg ( i )" ).arg ( classPath, e->name ) );
            }
            else
            {
                genSetupExtError (
                    impl, 1, "tmpRetVar",
                    String ( "\"Internal object %1.%2 did not validate properly\"" ).arg ( classPath, e->name ) );
            }

            impl.ae ( 1, "return tmpRetVar;" );
            impl.ae ( "}" );
        }

        if ( insideLoop )
        {
            impl.decBaseIndent ( 1 );
            impl.ae ( "}" );
        }
    }

    impl.e();
    impl.ae ( String ( "return %1;" ).arg ( getErrorCode ( ErrOK ) ) );
    impl.decBaseIndent ( 1 );
    impl.ae ( "}" ).e();

    // ***************************************************************************************************************
    // *********************************************** setupDefines() ************************************************
    // ***************************************************************************************************************

    hdr.ae ( "virtual void setupDefines();" ).e();

    impl.ae ( String ( "void %1::setupDefines()" ).arg ( classPath ) );
    impl.ae ( "{" );

    if ( s->getInheritance() != 0 )
    {
        impl.ae ( 1, String ( "%1::setupDefines();" ).arg ( s->getInheritance()->getName() ) ).e();
    }

    bool addLine = false;

    for ( HashMap<String, Element * >::Iterator defsIt ( s->getDefines() ); defsIt.isValid(); defsIt.next() )
    {
        Element * e = defsIt.value();

        assert ( e != 0 );

        if ( !e->definedTarget )
        {
            continue;
        }

        impl.i ( 1 );

        if ( e->definedTarget->aliasTarget != 0 && !e->definedTarget->usesFullType() )
        {
            impl.a ( "b" );
        }

        impl.a ( e->getCamelCaseName ( "set" ) ).a ( " ( " ).a ( getDefName ( e ) ).a ( " );" ).e();

        addLine = true;
    }

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        if ( !e->typeSymbol || !e->typeSymbol->isMessageOrStruct() )
        {
            continue;
        }

        if ( addLine )
        {
            impl.e();
            addLine = false;
        }

        String varName = getVarName ( e );
        int ind = 1;

        if ( e->isRepeated() )
        {
            impl.i ( 1 ).a ( "for ( size_t i = 0, lSize = " );
            impl.a ( exprListVarSize ( e->typeSymbol, getVarName ( e ) ) );
            impl.a ( "; i < lSize; ++i )" ).e();
            impl.i ( 1 ).a ( "{" );

            ind = 2;
            varName = exprListGetElemIdxRef ( e->typeSymbol, getVarName ( e ), "i" );
        }

        impl.i ( ind ).a ( varName ).a ( ".setupDefines();" ).e();

        if ( e->isRepeated() )
        {
            impl.i ( 1 ).a ( "}" ).e();
        }

        addLine = true;
    }

    genObjectModified ( s, impl, 1 );

    impl.ae ( "}" ).e();

    // ***************************************************************************************************************
    // ********************************************** operator== *****************************************************
    // ***************************************************************************************************************

    hdr.ce ( "@brief Equality operator" );
    hdr.ce ( "@param [in] other The object to compare against" );
    hdr.ce ( "@return True if these two objects are equal, false otherwise" );
    hdr.ae ( String ( "bool operator== ( const %1 & other ) const;" ).arg ( s->getName() ) ).e();

    impl.ae ( String ( "bool %1::operator== ( const %1 & other ) const" ).arg ( classPath ) );
    impl.ae ( "{" );

    impl.incBaseIndent ( 1 );

    if ( s->getInheritance() != 0 )
    {
        impl.ae ( String ( "if ( !%1::operator== ( other )) return false;" )
                  .arg ( s->getInheritance()->getName() ) ).e();
    }
    else
    {
        impl.ae ( "( void ) other;" ).e();
    }

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * const e = s->getElements().value ( elems[ i ] );

        // No need to compare aliases, the storage types will be compared directly!
        if ( e->aliasTarget != 0 )
            continue;

        impl.ae ( String ( "if ( %1() != other.%1() || ( %1() && %2 != other.%2 ) )" )
                  .arg ( e->getCamelCaseName ( "has" ), getVarName ( e ) ) );
        impl.ae ( "{" );
        impl.ae ( 1, "return false;" );
        impl.ae ( "}" ).e();
    }

    impl.ae ( "return true;" );

    impl.decBaseIndent ( 1 );
    impl.ae ( "}" ).e();

    hdr.ce ( "@brief Inequality operator" );
    hdr.ce ( "@param [in] other The object to compare against" );
    hdr.ce ( "@return True if these two objects are NOT equal, false otherwise" );
    hdr.ae ( String ( "inline bool operator!= ( const %1 & other ) const" ).arg ( s->getName() ) );
    hdr.ae ( "{" );
    hdr.ae ( 1, "return !( operator== ( other ) );" );
    hdr.ae ( "}" ).e();

    // ***************************************************************************************************************
    // ********************************** deserialize and serialize methods ******************************************
    // ***************************************************************************************************************

    genMsgDeserializeFieldMethod ( s, hdr, impl );
    genMsgSerializeFieldsMethod ( s, hdr, impl );
}

void CppGenerator::genMsgStdPrivMethods ( Symbol * s, CppFile & hdr, CppFile & impl )
{
    assert ( s != 0 );

    String classPath = getClassPath ( s );
    const StringList & elems = s->getOrdElements();

    // ***************************************************************************************************************
    // ************************************************* localClear() ************************************************
    // ***************************************************************************************************************

    hdr.ce ( "@brief Clears the local content" );
    hdr.ce();
    hdr.ce ( "All fields will either be set to their default values (or 0 if not set) or their clear()" );
    hdr.ce ( "method will be called and they will be set as not present." );
    hdr.a ( "void localClear();" ).e().e();

    impl.a ( "void " ).a ( classPath ).a ( "::localClear()" ).e();
    impl.a ( "{" ).e();

    int maxUsedPresenceIdx = -1;

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // We can't clear individual aliases, all of them will be
        // cleared once the storage field is cleared.
        if ( !e->aliasTarget )
        {
            if ( e->presenceIndex > maxUsedPresenceIdx )
            {
                maxUsedPresenceIdx = e->presenceIndex;
            }

            impl.i ( 1 ).a ( exprVarClear ( e ) ).a ( ";" ).e();
        }
    }

    int usedPresenceVars = 0;

    if ( maxUsedPresenceIdx >= 0 )
    {
        usedPresenceVars = 1 + ( maxUsedPresenceIdx / getPresVarSize() );
    }

    if ( usedPresenceVars > 0 )
    {
        impl.e();
    }

    for ( int i = 0; i < usedPresenceVars; ++i )
    {
        impl.i ( 1 ).a ( getPresVarNameNum ( i ) ).a ( " = 0;" ).e();
    }

    impl.e();
    genObjectModified ( s, impl, 1 );

    impl.a ( "}" ).e().e();
}

void CppGenerator::genMsgDeserializeFieldMethod ( Symbol * s, CppFile & hdr, CppFile & impl )
{
    assert ( s != 0 );

    hdr.ae ( String ( "virtual %1 deserializeField ( %2 fieldId, %3 wireType, %4 buf, "
                      "size_t offset, size_t fieldSize, %5 * extError );" )
             .arg ( getStdType ( TypeErrorCode ),
                    getStdType ( TypeFieldId ),
                    getStdType ( TypeWireType ),
                    getStdType ( TypeReadBuffer ),
                    getStdType ( TypeExtError ) ) ).e();

    impl.ae ( String ( "%1 %2::deserializeField ( %3 fieldId, %4 wireType, %5 buf, "
                       "size_t offset, size_t fieldSize, %6 * extError )" )
              .arg ( getStdType ( TypeErrorCode ),
                     getClassPath ( s ),
                     getStdType ( TypeFieldId ),
                     getStdType ( TypeWireType ),
                     getStdType ( TypeReadBuffer ),
                     getStdType ( TypeExtError ) ) );

    impl.ae ( "{" );
    impl.incBaseIndent ( 1 );

    impl.ae ( "( void ) fieldId;" );
    impl.ae ( "( void ) wireType;" );
    impl.ae ( "( void ) buf;" );
    impl.ae ( "( void ) offset;" );
    impl.ae ( "( void ) fieldSize;" );
    impl.ae ( "( void ) extError;" ).e();

    const StringList & elems = s->getOrdElements();
    bool addedSwitch = false;

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // We don't deserialize alias fields directly.
        // The actual storage value should be deserialized by the class
        // that actually contains it.
        if ( e->aliasTarget != 0 )
        {
            continue;
        }

        if ( !addedSwitch )
        {
            addedSwitch = true;

            impl.ae ( "switch ( fieldId )" );
            impl.ae ( "{" );
            impl.incBaseIndent ( 1 );
        }

        if ( i > 0 )
        {
            impl.e();
        }

        impl.ae ( String ( "case %1: // %2" ).arg ( getFieldIdName ( e ), e->getCamelCaseName() ) );
        impl.ae ( 1, "{" );

        String varName = getVarName ( e );

        if ( e->isRepeated() )
        {
            varName = "tmpVal";
            impl.ae ( 2, String ( "%1 %2;" ).arg ( getRawVarType ( hdr, e->typeSymbol ), varName ) ).e();
        }

        assert ( e->typeSymbol != 0 );

        if ( e->typeSymbol->isMessageOrStruct() )
        {
            impl.ae ( 2, String ( "if ( %1 )" ).arg ( exprVarLenWireTypeCheck ( "wireType" ) ) );
            impl.ae ( 2, "{" );

            genSetupExtError ( impl, 3, getErrorCode ( ErrProtocolError ),
                               String ( "\"Error decoding %1.%2 - "
                                        "variable length encoding cannot be used for structures\"" )
                               .arg ( getClassPath ( s ), e->name ) );

            impl.ae ( 3, String ( "return %1;" ).arg ( getErrorCode ( ErrProtocolError ) ) ).e();
            impl.ae ( 2, "}" ).e();

            impl.ae ( 2, "size_t tmpOffset = offset;" ).e();
            impl.ae ( 2, String ( "%1 ret;" ).arg ( getStdType ( TypeErrorCode ) ) );

            genDeserializeMessage ( impl, 2, e->typeSymbol, varName,
                                    "buf", "fieldSize", "tmpOffset", "ret", "extError" );
            impl.e();

            impl.ae ( 2, "// If the field is read properly, the error code should be 'ok' or 'warning'" );

            if ( e->typeSymbol->isMessage() )
            {
                impl.ae ( 2, String ( "if ( ret == %1 )" ).arg ( getErrorCode ( ErrProtocolWarning ) ) );
                impl.ae ( 2, "{" );
                impl.ae ( 3, "// This is field is another 'message', which most likely means" );
                impl.ae ( 3, "// that it is used for storing data of some unknown (at this level) type." );
                impl.ae ( 3, "// Let's not propagate the warning up:" );
                impl.ae ( 3, String ( "ret = %1;" ).arg ( getErrorCode ( ErrOK ) ) );
                impl.ae ( 2, "}" );
                impl.ae ( 2, String ( "else if ( ret != %1 )" ).arg ( getErrorCode ( ErrOK ) ) );
            }
            else
            {
                impl.ae ( 2, String ( "if ( ret != %1 && ret != %2 )" )
                          .arg ( getErrorCode ( ErrOK ), getErrorCode ( ErrProtocolWarning ) ) );
            }

            impl.ae ( 2, "{" );

            genSetupExtError ( impl, 3, "ret",
                               String ( "\"Error deserializing %1.%2\"" )
                               .arg ( getClassPath ( s ), e->name ) );

            impl.ae ( 3, "return ret;" );
            impl.ae ( 2, "}" ).e();
        }
        else
        {
            impl.i ( 2 ).a ( String ( "%1 ret = " ).arg ( getStdType ( TypeErrorCode ) ) );

            if ( e->typeSymbol->isEnum() )
            {
                // Enums have their own function allowing them to deserialize themselves
                impl.a ( varName ).a ( ".deserializeEnum ( buf, offset, fieldSize, wireType )" );
            }
            else
            {
                // This is a regular element - lets use protocol decoder to deserialize it
                impl.a ( exprProtoDecodeFieldValue ( "buf", "offset", "fieldSize", "wireType", varName ) );
            }

            impl.ae ( ";" ).e();

            impl.ae ( 2, String ( "if ( ret != %1 )" ).arg ( getErrorCode ( ErrOK ) ) );
            impl.ae ( 2, "{" );

            genSetupExtError ( impl, 3, "ret",
                               String ( "\"Error deserializing %1.%2\"" )
                               .arg ( getClassPath ( s ), e->name ) );

            impl.ae ( 3, "return ret;" );
            impl.ae ( 2, "}" ).e();
        }

        if ( e->isRepeated() )
        {
            assert ( varName != getVarName ( e ) );

            // It's a list - we just need to append the temporary element to it
            impl.a ( 2, exprListAppend ( e->typeSymbol, getVarName ( e ), varName ) ).ae ( ";" );
        }
        else
        {
            assert ( e->presenceIndex >= 0 );

            // It's a regular element. We already read it, now we mark the presence bit

            impl.ae ( 2, String ( "%1 |= ( 1 << %2 );" )
                      .arg ( getPresVarNameIdx ( e->presenceIndex ) )
                      .arg ( getPresVarShift ( e->presenceIndex ) ) );
        }

        impl.ae ( 2, "return ret;" );
        impl.ae ( 1, "}" );
        impl.ae ( 1, "break;" );
    }

    if ( addedSwitch )
    {
        impl.decBaseIndent ( 1 );
        impl.ae ( "}" );
        impl.e();
    }

    if ( !s->getInheritance() )
    {
        impl.ae ( String ( "return %1;" ).arg ( getErrorCode ( ErrProtocolWarning ) ) );
    }
    else
    {
        impl.ae ( String ( "return %1::deserializeField ( fieldId, wireType, buf, offset, fieldSize, extError );" )
                  .arg ( s->getInheritance()->getName() ) );
    }

    impl.decBaseIndent ( 1 );
    impl.ae ( "}" ).e();
}

void CppGenerator::genMsgSerializeFieldsMethod ( Symbol * s, CppFile & hdr, CppFile & impl )
{
    assert ( s != 0 );

    hdr.ae ( String ( "virtual %1 serializeFields ( %2 & buf, %3 * extError );" )
             .arg ( getStdType ( TypeErrorCode ),
                    getStdType ( TypeWriteBuffer ),
                    getStdType ( TypeExtError ) ) );
    hdr.e();

    const StringList & elems = s->getOrdElements();

    impl.ae ( String ( "%1 %2::serializeFields ( %3 & buf, %4 * extError )" )
              .arg ( getStdType ( TypeErrorCode ),
                     getClassPath ( s ),
                     getStdType ( TypeWriteBuffer ),
                     getStdType ( TypeExtError ) ) );
    impl.ae ( "{" );
    impl.ae ( 1, "( void ) extError;" );
    impl.ae ( 1, "( void ) buf;" ).e();

    if ( s->getInheritance() != 0 )
    {
        impl.i ( 1 ).a ( String ( "%1 ret = " ).arg ( getStdType ( TypeErrorCode ) ) );
        impl.a ( s->getInheritance()->getName() ).a ( "::serializeFields ( buf, extError );" ).e().e();

        impl.ae ( 1, String ( "if ( ret != %1 )" ).arg ( getErrorCode ( ErrOK ) ) );
        impl.ae ( 1, "{" );

        genSetupExtError ( impl, 2, "ret",
                           String ( "\"Error calling %1::serializeFields from %2\"" )
                           .arg ( getClassPath ( s->getInheritance() ), getClassPath ( s ) ) );

        impl.ae ( 2, "return ret;" );
        impl.ae ( 1, "}" );
    }
    else
    {
        impl.ae ( 1, String ( "%1 ret = %2;" ).arg ( getStdType ( TypeErrorCode ), getErrorCode ( ErrOK ) ) );
    }

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // We don't serialize alias fields directly.
        // The actual storage value should be serialized by the class
        // that actually contains it.
        if ( e->aliasTarget != 0 )
        {
            continue;
        }

        assert ( e->typeSymbol != 0 );

        String varName = getVarName ( e );

        if ( !e->isRepeated() )
        {
            impl.ae ( 1, String ( "if ( %1() )" ).arg ( e->getCamelCaseName ( "has" ) ) );
            impl.ae ( 1, "{" );
        }
        else
        {
            impl.ae ( 1, String ( "for ( size_t i = 0, lSize = %1; i < lSize; ++i )" )
                      .arg ( exprListVarSize ( e->typeSymbol, getVarName ( e ) ) ) );
            impl.ae ( 1, "{" );

            varName = "varRef";

            impl.ae ( 2, String ( "%1 & %2 = %3;" )
                      .arg ( getRawVarType ( hdr, e->typeSymbol ),
                             varName,
                             exprListGetElemIdxRef ( e->typeSymbol, getVarName ( e ), "i" ) ) ).e();
        }

        if ( e->typeSymbol->isMessageOrStruct() )
        {
            impl.i ( 2 ).a ( getStdType ( TypeWriteBuffer ) ).ae ( " tmpBuf;" ).e();

            genSerializeMessage ( impl, 2, e->typeSymbol, varName, "tmpBuf", "ret", "extError" );

            impl.ae ( 2, String ( "if ( ret != %1 )" ).arg ( getErrorCode ( ErrOK ) ) );
            impl.ae ( 2, "{" );

            genSetupExtError ( impl, 3, "ret",
                               String ( "\"Error serializing %1.%2 into temporary buffer\"" )
                               .arg ( getClassPath ( s ), e->name ) );

            impl.ae ( 3, "return ret;" );
            impl.ae ( 2, "}" ).e();

            impl.ae ( 2, String ( "ret = %1;" ).arg ( exprProtoEncode ( "buf", "tmpBuf", getFieldIdName ( e ) ) ) );
        }
        else
        {
            if ( e->typeSymbol->isEnum() )
            {
                varName = String ( "( ( %1 ) %2.value() )" ).arg ( getStdType ( TypeEnum ), varName );
            }

            impl.ae ( 2, String ( "ret = %1;" ).arg ( exprProtoEncode ( "buf", varName, getFieldIdName ( e ) ) ) );
        }

        impl.ae ( 2, String ( "if ( ret != %1 )" ).arg ( getErrorCode ( ErrOK ) ) );
        impl.ae ( 2, "{" );

        genSetupExtError ( impl, 3, "ret",
                           String ( "\"Error encoding %1.%2\"" ).arg ( getClassPath ( s ), e->name ) );

        impl.ae ( 3, "return ret;" );
        impl.ae ( 2, "}" );
        impl.ae ( 1, "}" ).e();
    }

    impl.e();
    impl.ae ( 1, "return ret;" );
    impl.ae ( "}" ).e();
}

String CppGenerator::exprVarClear ( Element * e )
{
    if ( !e->typeSymbol )
        return String::EmptyString;

    if ( e->isRepeated() )
        return String::EmptyString;

    String optVal;

    if ( e->isOptional() && e->options.contains ( ProtocolSpec::KW_DEFAULT ) )
    {
        optVal = e->options[ ProtocolSpec::KW_DEFAULT ];
    }

    String ret;

    if ( e->typeSymbol->isInteger() || e->typeSymbol->isFloatingPoint() )
    {
        ret = getVarName ( e ).append ( " = " );

        if ( optVal.length() > 0 )
        {
            ret.append ( optVal );
        }
        else
        {
            ret.append ( "0" );
        }
    }
    else if ( e->typeSymbol->isEnum() )
    {
        ret = getVarName ( e );

        if ( optVal.length() > 0 )
        {
            ret.append ( " = " );
            ret.append ( getClassPath ( e->typeSymbol ) ).append ( "::" );
            ret.append ( optVal );
        }
        else
        {
            ret.append ( ".clear()" );
        }
    }

    return ret;
}
