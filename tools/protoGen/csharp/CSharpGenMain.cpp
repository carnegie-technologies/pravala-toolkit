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
#include "CSharpGen.hpp"

using namespace Pravala;

void CSharpGenerator::genEnumClass ( Symbol * s, CSharpFile & impl )
{
    assert ( s != 0 );
    assert ( s->isEnum() );

    impl.csum ( String ( "'%1' enumerator." ).arg ( s->getName() ) );

    if ( !s->getComment().isEmpty() )
        impl.cdesc ( s->getComment() );

    impl.ae ( String ( "public enum %1 : %2" ).arg ( s->getName(), _type.Enum ) );
    impl.ae ( "{" );

    impl.incBaseIndent();

    const StringList & elems = s->getOrdElements();

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // This is an enum!
        assert ( !e->isRequired() );
        assert ( !e->isRepeated() );
        assert ( !e->aliasTarget );

        if ( i > 0 )
            impl.e();

        if ( !e->comment.isEmpty() )
        {
            impl.ae ( String ( "///<summary>%1</summary>" ).arg ( e->comment ) );
        }

        impl.a ( String ( "%1 = %2" ).arg ( e->name, e->value ) );

        if ( i + 1 < elems.size() )
        {
            impl.a ( "," );
        }

        impl.e();
    }

    impl.decBaseIndent();

    impl.ae ( "}" );
}

void CSharpGenerator::genMessageClass ( Symbol * s, CSharpFile & impl )
{
    hookPosition ( s, impl, PosBeforeClass );

    impl.csum ( String ( "Class representing '%1' %2." )
                .arg ( s->getName(), s->isStruct() ? "structure" : "message" ) );

    if ( !s->getComment().isEmpty() )
        impl.cdesc ( s->getComment() );

    impl.a ( "public partial class " ).a ( s->getName() );

    const String extStr = getExtends ( s );

    if ( !extStr.isEmpty() )
    {
        impl.a ( ": " ).a ( extStr );
    }

    impl.e();
    impl.ae ( "{" );

    impl.incBaseIndent();

    hookPosition ( s, impl, PosClassOpened );

    assert ( s != 0 );

    for ( HashMap<String, Element * >::Iterator defsIt ( s->getDefines() ); defsIt.isValid(); defsIt.next() )
    {
        Element * e = defsIt.value();

        assert ( e != 0 );

        Element * d = e->definedTarget;

        if ( !d )
            continue;

        String varValue = e->value;

        if ( d->typeSymbol != 0 && !d->typeSymbol->isBasic() )
        {
            // We need the full path to that element:
            varValue = getClassPath ( d->typeSymbol );
            varValue.append ( "." );
            varValue.append ( e->value );
        }

        impl.csum ( String ( "The value to which '%1' will be defined to by this %2." )
                    .arg ( d->name, s->isStruct() ? "structure" : "message" ) );

        if ( !e->comment.isEmpty() )
            impl.cdesc ( e->comment );

        impl.a ( "public const " );

        if ( !d->typeSymbol )
        {
            assert ( d->aliasTarget != 0 );

            if ( d->getAliasPayloadBitLength() > 1 )
            {
                impl.a ( getRawVarType ( getAliasStorageSymbol ( d->getAliasPayloadBitLength() ) ) );
            }
            else
            {
                impl.a ( "bool" );
            }
        }
        else
        {
            impl.a ( getRawVarType ( d->typeSymbol ) );
        }

        impl.ae ( String ( " %1 = %2;" ).arg ( getDefName ( e ), varValue ) ).e();
    }

    impl.e();

    const StringList & intSymbols = s->getOrdInternalSymbols();

    for ( size_t i = 0; i < intSymbols.size(); ++i )
    {
        Symbol * intSym = s->getInternalSymbols().value ( intSymbols[ i ] );

        assert ( intSym != 0 );

        genRegularSymbol ( intSym, impl );
    }

    genMsgFields ( s, impl );
    genMsgStdMethods ( s, impl );

    hookPosition ( s, impl, PosBeforeClassClose );

    impl.decBaseIndent();
    impl.ae ( "}" );

    hookPosition ( s, impl, PosClassClosed );
}

void CSharpGenerator::genFieldRemarks ( Element * e, CSharpFile & impl, bool closeBlock )
{
    impl.ce ( "<remarks>" );

    impl.genVarComments ( e );

    if ( closeBlock )
        impl.ce ( "</remarks>" );
}

void CSharpGenerator::genMsgAliasField ( Symbol * /* symbol */, Element * e, CSharpFile & impl )
{
    assert ( e->aliasTarget != 0 );

    // No 'repeated' aliases!
    assert ( !e->isRepeated() );

    const int aliasNumBits ( e->getAliasPayloadBitLength() );
    const String aliastBitMask ( getBitmask ( aliasNumBits ) );
    const String aliasType ( ( aliasNumBits == 1 )
            ? "bool" : getRawVarType ( getAliasStorageSymbol ( aliasNumBits ) ) );
    const String storageType ( getRawVarType ( e->aliasTarget->typeSymbol ) );
    const String storageTypeCast ( String ( "(%1) " ).arg ( storageType ) );

    const bool aliasUsesFullType = e->usesFullType();

    String aliasTypeCast;

    if ( aliasNumBits > 1 )
    {
        aliasTypeCast = String ( "(%1) " ).arg ( aliasType );
    }

    uint64_t aliasMaxValue = 0;

    assert ( aliasNumBits > 0 );
    int n = aliasNumBits;

    while ( --n >= 0 )
    {
        aliasMaxValue = ( aliasMaxValue << 1 ) | 1;
    }

    impl.csum ( String ( "Checks whether '%1' is set or not" ).arg ( getFieldName ( e ) ) );

    genFieldRemarks ( e, impl, false );

    if ( e->isSalias() )
    {
        impl.ce ( String ( "This is an s-alias, so it checks whether the '%1' field is set or not, "
                           "but it also verifies the additional presence bit for this s-alias." )
                  .arg ( getFieldName ( e->aliasTarget ) ) );
    }
    else
    {
        impl.ce ( String ( "This is an alias, so it checks whether the '%1' field is set or not." )
                  .arg ( getFieldName ( e->aliasTarget ) ) );
    }

    impl.ce ( "</remarks>" );

    impl.cret ( String ( "<c>true</c>, if this %1alias is set, <c>false</c> otherwise" )
                .arg ( e->isSalias() ? "s-" : "" ) );
    impl.ae ( String ( "public bool %1" ).arg ( getHasName ( e, false ) ) );
    impl.ae ( "{" );

    impl.a ( 1, String ( "return ( %1" ).arg ( getHasName ( e->aliasTarget ) ) );

    if ( e->isSalias() )
    {
        impl.a ( String ( " && ( ( ( %1 >> %2 ) & 0x01 ) == 0x01 )" )
                 .arg ( getGetName ( e->aliasTarget ) )
                 .arg ( e->iAliasRangeFrom ) );
    }

    impl.ae ( " );" );
    impl.ae ( "}" );

    impl.csum ( String ( "Returns the value of %1." ).arg ( getFieldName ( e ) ) );

    genFieldRemarks ( e, impl );

    impl.cret ( String ( "The value of %1, or %2 if it is not set." )
                .arg ( getFieldName ( e ) )
                .arg ( ( e->options.contains ( ProtocolSpec::KW_DEFAULT ) )
                       ? ( e->options[ ProtocolSpec::KW_DEFAULT ] )
                       : ( ( aliasNumBits == 1 ) ? "false" : "0" ) ) );

    impl.ae ( String ( "public %1 %2" ).arg ( aliasType, getGetName ( e, false ) ) );
    impl.ae ( "{" );
    impl.incBaseIndent ( 1 );

    impl.ae ( String ( "if ( !%1 )" ).arg ( getHasName ( e ) ) );
    impl.a ( 1, "return " );

    if ( e->options.contains ( ProtocolSpec::KW_DEFAULT ) )
    {
        impl.a ( aliasTypeCast ).a ( e->options[ ProtocolSpec::KW_DEFAULT ] );
    }
    else if ( aliasNumBits == 1 )
    {
        impl.a ( "false" );
    }
    else
    {
        impl.a ( aliasTypeCast ).a ( "0" );
    }

    impl.ae ( ";" ).e();

    impl.ae ( String ( "return %1( ( ((%2)%3) >> %4 ) & %5 )%6;" )
              .arg ( aliasTypeCast, getRawVarType ( e->aliasTarget->typeSymbol ), getGetName ( e->aliasTarget ) )
              .arg ( e->iAliasPayloadRangeFrom )
              .arg ( aliastBitMask, ( aliasNumBits == 1 ) ? " == 1" : "" ) );

    impl.decBaseIndent ( 1 );
    impl.ae ( "}" );

    impl.csum ( String ( "Sets the value of %1." ).arg ( getFieldName ( e ) ) );

    genFieldRemarks ( e, impl, false );

    impl.ce ( String ( "This value is stored in the bitrange of the underlying property '%1' "
                       "(that uses storage type '%2')." )
              .arg ( getFieldName ( e->aliasTarget ), getRawVarType ( e->aliasTarget->typeSymbol ) ) );

    if ( !aliasUsesFullType )
    {
        impl.ce ( String ( "This field can hold values between 0 and %1." ).arg ( aliasMaxValue ) );
        impl.ce ( "Assigning a value greater than that will cause an exception" );

        impl.cexc ( "System.ArgumentOutOfRangeException",
                    String ( "When assigned value is greater than %1." ).arg ( aliasMaxValue ) );
    }

    impl.ce ( "</remarks>" );

    impl.cparam ( "value", String ( "The new value of %1." ).arg ( getFieldName ( e ) ) );

    if ( e->isProtected() )
    {
        impl.a ( "protected" );
    }
    else if ( e->isPrivate() )
    {
        impl.a ( "private" );
    }
    else
    {
        impl.a ( "public" );
    }

    impl.ae ( String ( " void %1 ( %2 value )" ).arg ( getSetName ( e, false ), aliasType ) );
    impl.ae ( "{" );
    impl.incBaseIndent ( 1 );

    if ( !aliasUsesFullType )
    {
        impl.ae ( String ( "if ( (%1) value > (%1) %2 )" ).arg ( aliasType ).arg ( aliasMaxValue ) );
        impl.ae ( 1,
                  String (
                      "throw new System.ArgumentOutOfRangeException ( \"Passed value (\" + value + \") is "
                      "greater than the max value '%2' can hold: %1\" );" )
                  .arg ( aliasMaxValue ).arg ( getFieldName ( e ) ) ).e();
    }

    if ( aliasNumBits == 1 )
    {
        String setPresence;

        if ( e->isSalias() )
        {
            setPresence = String ( " | ( 0x01 << %1 )" ).arg ( e->iAliasRangeFrom );

            impl.ae ( "// These also set the presence bit of this s-alias:" );
        }

        impl.ae ( "if ( value )" );
        impl.ae ( "{" );

        impl.ae ( 1, String ( "%1 ( %2( %3 | ( 0x01 << %4 )%5 ) );" )
                  .arg ( getSetName ( e->aliasTarget ), storageTypeCast, getGetName ( e->aliasTarget ) )
                  .arg ( e->iAliasPayloadRangeFrom ).arg ( setPresence ) );

        impl.ae ( "}" );
        impl.ae ( "else" );
        impl.ae ( "{" );

        impl.ae ( 1, String ( "%1 ( %2( ( %3 & ~( 0x01 << %4 ) )%5 ) );" )
                  .arg ( getSetName ( e->aliasTarget ), storageTypeCast, getGetName ( e->aliasTarget ) )
                  .arg ( e->iAliasPayloadRangeFrom ).arg ( setPresence ) );

        impl.ae ( "}" );
    }
    else
    {
        // We want to get this:
        // X = ( ( x & (~ ( bmask << rangefrom ) ) ) | ( ( newval & bmask ) << rangefrom ) )

        impl.ae ( String ( "%1 ( %2( ( %3 & ( ~( %4 << %5 ) ) ) | ( ( value & %4 ) << %5 ) ) );" )
                  .arg ( getSetName ( e->aliasTarget ), storageTypeCast, getGetName ( e->aliasTarget ), aliastBitMask )
                  .arg ( e->iAliasPayloadRangeFrom ) );
    }

    impl.decBaseIndent ( 1 );
    impl.ae ( "}" );
}

void CSharpGenerator::genMsgRegularField ( Symbol * /* symbol */, Element * e, CSharpFile & impl )
{
    impl.csum ( String ( "Checks whether '%1' is set or not" ).arg ( getFieldName ( e ) ) );

    genFieldRemarks ( e, impl );

    impl.cret ( "<c>true</c>, if the value was set, <c>false</c> otherwise" );
    impl.ae ( String ( "public bool %1" ).arg ( getHasName ( e, false ) ) );
    impl.ae ( "{" );

    if ( e->isRepeated() )
    {
        impl.ae ( 1, String ( "return (%1 != null && %1.Count > 0);" ).arg ( getVarName ( e ) ) );
    }
    else
    {
        // getVarName returns something that is always nullable (even if it's a numeric type):

        impl.ae ( 1, String ( "return (%1 != null);" ).arg ( getVarName ( e ) ) );
    }

    impl.ae ( "}" );

    impl.csum ( String ( "Returns the value of %1." ).arg ( getFieldName ( e ) ) );

    genFieldRemarks ( e, impl );

    String elemDefValue;

    if ( e->isRepeated() )
    {
        // Repeated - when it's not set it's just 'null';
        elemDefValue = "null";
    }
    else if ( isNullable ( e->typeSymbol ) )
    {
        if ( !e->options.contains ( ProtocolSpec::KW_DEFAULT ) )
        {
            // No default value - null by default.

            elemDefValue = "null";
        }
        else
        {
            // We have a default value:

            elemDefValue = e->options[ ProtocolSpec::KW_DEFAULT ];
        }
    }
    else
    {
        if ( e->options.contains ( ProtocolSpec::KW_DEFAULT ) )
        {
            if ( e->typeSymbol->isEnum() )
            {
                elemDefValue = String ( "%1.%2" )
                               .arg ( getRawVarType ( e->typeSymbol ), e->options[ ProtocolSpec::KW_DEFAULT ] );
            }
            else
            {
                elemDefValue = e->options[ ProtocolSpec::KW_DEFAULT ];
            }
        }
        else
        {
            // No default, let's return type's default.
            // For numeric types it's 0, for enums it's their own default.

            if ( !e->typeSymbol->isEnum() )
            {
                elemDefValue = "0";
            }
            else
            {
                Element * defEnumElem = e->typeSymbol->getEnumDefault();

                assert ( defEnumElem != 0 );

                if ( !defEnumElem )
                    throw Error ( "Enumerators have to have a default value!" );

                elemDefValue = String ( "%1.%2" )
                               .arg ( getRawVarType ( e->typeSymbol ), defEnumElem->name );
            }
        }
    }

    impl.cret ( String ( "The value of %1, or %2 if it is not set." )
                .arg ( getFieldName ( e ), elemDefValue ) );

    impl.ae ( String ( "public %1 %2" ).arg ( getVarType ( e ), getGetName ( e, false ) ) );
    impl.ae ( "{" );
    impl.incBaseIndent ( 1 );

    assert ( !e->aliasTarget );
    assert ( e->typeSymbol != 0 );

    if ( e->isRepeated() )
    {
        // Repeated - just return whatever there is.

        impl.ae ( String ( "return %1;" ).arg ( getVarName ( e ) ) );
    }
    else if ( isNullable ( e->typeSymbol ) )
    {
        if ( !e->options.contains ( ProtocolSpec::KW_DEFAULT ) )
        {
            // No default value - just return (null or not)

            impl.ae ( String ( "return %1;" ).arg ( getVarName ( e ) ) );
        }
        else
        {
            // Default value - return it if the actual value is null

            impl.ae ( String ( "return (%1 != null)?(%1):(%2);" ).arg ( getVarName ( e ), elemDefValue ) );
        }
    }
    else
    {
        // Numeric type or enum

        // It exists - return it (casted to the primitive type).
        // We compare against 'null' even though isNullable would say false.
        // We store numeric types and enums with '?' at the end of the type. This makes them nullable.

        impl.ae ( String ( "return (%1 != null)?((%2) %1):((%2) %3);" )
                  .arg ( getVarName ( e ), getRawVarType ( e->typeSymbol ), elemDefValue ) );
    }

    impl.decBaseIndent ( 1 );

    impl.ae ( "}" );

    impl.csum ( String ( "Sets the value of %1." ).arg ( getFieldName ( e ) ) );

    genFieldRemarks ( e, impl );

    impl.cparam ( "value", String ( "The new value of %1." ).arg ( getFieldName ( e ) ) );

    if ( e->isProtected() )
    {
        impl.a ( "protected" );
    }
    else if ( e->isPrivate() )
    {
        impl.a ( "private" );
    }
    else
    {
        impl.a ( "public" );
    }

    impl.ae ( String ( " void %1 ( %2 value )" ).arg ( getSetName ( e, false ), getVarType ( e ) ) );
    impl.ae ( "{" );
    impl.ae ( 1, String ( "%1 = value;" ).arg ( getVarName ( e ) ) );
    impl.ae ( "}" );

    // Here we actually want to use nullable version of the type:

    impl.csum ( String ( "Backing store for %1" ).arg ( getFieldName ( e ) ) );
    impl.ae ( String ( "private %1 %2;" ).arg ( getVarType ( e, true ), getVarName ( e, false ) ) );
}

void CSharpGenerator::genMsgFields ( Symbol * s, CSharpFile & impl )
{
    assert ( s != 0 );

    const StringList & elems = s->getOrdElements();

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        if ( e->aliasTarget != 0 )
        {
            genMsgAliasField ( s, e, impl );
        }
        else
        {
            genMsgRegularField ( s, e, impl );
        }
    }
}

void CSharpGenerator::genMsgStdMethods ( Symbol * s, CSharpFile & impl )
{
    assert ( s != 0 );

    const StringList & elems = s->getOrdElements();

    // ***************************************************************************************************************
    // *********************************************** Default constructor *******************************************
    // ***************************************************************************************************************

    impl.csum ( String ( "Initializes a new instance of the <see cref=\"%1\" /> class." ).arg ( getClassPath ( s ) ) );
    impl.ae ( String ( "public %1()" ).arg ( s->getName() ) );
    impl.ae ( "{" );
    impl.ae ( "}" );

    // ***************************************************************************************************************
    // **************************************************** clear() **************************************************
    // ***************************************************************************************************************

    impl.csum ( "Clears all the fields" );
    impl.ae ( String ( "public override void %1()" ).arg ( _method.Clear ) );
    impl.ae ( "{" );
    impl.ae ( 1, String ( "base.%1();" ).arg ( _method.Clear ) );

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // We can't clear individual aliases, all of them will be
        // cleared once the storage field is cleared.
        if ( !e->aliasTarget )
        {
            assert ( e->typeSymbol != 0 );

            // getVarName returns something that is always nullable (even if it's a numeric type):

            impl.ae ( 1, String ( "%1 = null;" ).arg ( getVarName ( e ) ) );
        }
    }

    impl.ae ( "}" );

    // ***************************************************************************************************************
    // *************************************************** validate() ************************************************
    // ***************************************************************************************************************

    impl.csum ( "Validates the content of the object" );
    impl
    .cdesc ( "Checks if all required fields in this and all inherited objects are present and have legal values." ).e()
    .a ( "If this is used by external code on messages that are to be sent" ).e()
    .a ( String ( "it is probably a good idea to call %1() first." ).arg ( _method.SetupDefines ) );
    impl.cexc ( _type.Exception, "When the object doesn't validate properly." );
    impl.ae ( String ( "public override void %1()" ).arg ( _method.Validate ) );
    impl.ae ( "{" );
    impl.ae ( 1, String ( "base.%1();" ).arg ( _method.Validate ) );

    for ( HashMap<String, Element * >::Iterator defsIt ( s->getDefines() ); defsIt.isValid(); defsIt.next() )
    {
        Element * e = defsIt.value();

        assert ( e != 0 );

        if ( !e->definedTarget )
            continue;

        impl.ae ( 1, String ( "if ( !%1 || !%2.Equals ( %3 ) )" )
                  .arg ( getHasName ( e ), getGetName ( e ), getDefName ( e ) ) );
        impl.ae ( 1, "{" );
        impl.ae ( 2, String ( "throw new %1 ( %2, \"Incorrect value of %3: \" + %4 + \"; "
                              "Which should (as defined) be: \" + %5 );" )
                  .arg ( _type.Exception, _error.DefinedValueMismatch,
                         getFieldName ( e ),
                         getGetName ( e ), getDefName ( e ) ) );
        impl.ae ( 1, "}" );
    }

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        const String varPath = String ( getClassPath ( s ) ).append ( "." ).append ( getFieldName ( e ) );

        assert ( e != 0 );

        impl.e();

        if ( e->isRequired() )
        {
            impl.ae ( 1, String ( "if ( !%1 )" ).arg ( getHasName ( e ) ) );
            impl.ae ( 1, "{" );
            impl.ae ( 2, String ( "throw new %1 ( %2, \"Required field %3 is not set\" );" )
                      .arg ( _type.Exception, _error.RequiredFieldNotSet, varPath ) );
            impl.ae ( 1, "}" );
        }

        String varName;

        if ( !e->aliasTarget )
            varName = getVarName ( e );

        if ( e->isRepeated() )
        {
            if ( e->options.contains ( ProtocolSpec::KW_MIN_LIST_SIZE ) )
            {
                impl.ae ( 1, String ( "if ( %1 != null && %1.Count < %2 )" )
                          .arg ( getVarName ( e ), e->options[ ProtocolSpec::KW_MIN_LIST_SIZE ] ) );
                impl.ae ( 1, "{" );
                impl.ae ( 2, String ( "throw new %1 ( %2, \"%3 list has too few elements: \" + %4.Count + \"; "
                                      "At least %5 elements are required\" );" )
                          .arg ( _type.Exception,
                                 _error.ListSizeOutOfRange,
                                 varPath, getVarName ( e ),
                                 e->options[ ProtocolSpec::KW_MIN_LIST_SIZE ] ) );
                impl.ae ( 1, "}" );
            }

            if ( e->options.contains ( ProtocolSpec::KW_MAX_LIST_SIZE ) )
            {
                impl.ae ( 1, String ( "if ( %1 != null && %1.Count > %2 )" )
                          .arg ( getVarName ( e ), e->options[ ProtocolSpec::KW_MAX_LIST_SIZE ] ) );
                impl.ae ( 1, "{" );
                impl.ae ( 2, String ( "throw new %1 ( %2, \"%3 list has too many elements: \" + %4.Count + \"; "
                                      "At most %5 elements are required\" );" )
                          .arg ( _type.Exception,
                                 _error.ListSizeOutOfRange,
                                 varPath, getVarName ( e ),
                                 e->options[ ProtocolSpec::KW_MAX_LIST_SIZE ] ) );
                impl.ae ( 1, "}" );
            }

            // We set this AFTER the list size stuff. We wanted to use the 'list' variable,
            // but from now on we want to use the 'listElem' instead.
            varName = "listElem";
        }

        impl.e();

        StringList conditions;

        if ( e->options.contains ( ProtocolSpec::KW_MIN_LENGTH ) )
        {
            conditions.append ( String ( "if ( %1.Length < %2 ) throw new %3 ( %4, \"Too short string "
                                         "in %5 ( \" + %1 + \" characters; minimum is %2 )\" );" )
                                .arg ( varName, e->options[ ProtocolSpec::KW_MIN_LENGTH ],
                                       _type.Exception, _error.StringLengthOutOfRange, varPath ) );
        }

        if ( e->options.contains ( ProtocolSpec::KW_MAX_LENGTH ) )
        {
            conditions.append ( String ( "if ( %1.Length > %2 ) throw new %3 ( %4, \"Too long string "
                                         "in %5 ( \" + %1 + \" characters; maximum is %2 )\" );" )
                                .arg ( varName, e->options[ ProtocolSpec::KW_MAX_LENGTH ],
                                       _type.Exception, _error.StringLengthOutOfRange, varPath ) );
        }

        if ( e->options.contains ( ProtocolSpec::KW_MIN ) )
        {
            conditions.append ( String ( "if ( %1 < %2 ) throw new %3 ( %4, \"Too small value "
                                         "in %5 ( value: \" + %1 + \"; minimum is %2 )\" );" )
                                .arg ( varName, e->options[ ProtocolSpec::KW_MIN ],
                                       _type.Exception, _error.FieldValueOutOfRange, varPath ) );
        }

        if ( e->options.contains ( ProtocolSpec::KW_MAX ) )
        {
            conditions.append ( String ( "if ( %1 > %2 ) throw new %3 ( %4, \"Too big value "
                                         "in %5 ( value: \" + %1 + \"; maximum is %2 )\" );" )
                                .arg ( varName, e->options[ ProtocolSpec::KW_MAX ],
                                       _type.Exception, _error.FieldValueOutOfRange, varPath ) );
        }

        // We need to call 'validate' in embedded messages
        if ( e->typeSymbol != 0 && e->typeSymbol->isMessageOrStruct() )
        {
            conditions.append ( String ( varName ).append ( String ( ".%1();" ).arg ( _method.Validate ) ) );
        }

        if ( conditions.size() > 0 )
        {
            // getVarName returns something that is always nullable (even if it's a numeric type):

            impl.ae ( 1, String ( "if ( %1 != null )" ).arg ( getVarName ( e ) ) );
            impl.ae ( 1, "{" );

            impl.incBaseIndent ( 2 );

            if ( e->isRepeated() )
            {
                impl.ae ( String ( "foreach ( %1 %2 in %3 )" )
                          .arg ( getRawVarType ( e->typeSymbol ), varName, getVarName ( e ) ) );
                impl.ae ( "{" );
                impl.incBaseIndent ( 1 );

                if ( isNullable ( e->typeSymbol ) )
                {
                    impl.ae ( String ( "if ( %1 == null )" ).arg ( varName ) );
                    impl.ae ( "{" );
                    impl.ae ( 1, "continue;" );
                    impl.ae ( "}" ).e();
                }
            }

            for ( size_t idx = 0; idx < conditions.size(); ++idx )
            {
                impl.ae ( conditions[ idx ] );
            }

            if ( e->isRepeated() )
            {
                impl.decBaseIndent ( 1 );
                impl.ae ( "}" );
            }

            impl.decBaseIndent ( 2 );
            impl.ae ( 1, "}" );
        }
    }

    impl.ae ( "}" );

    // ***************************************************************************************************************
    // *********************************************** setupDefines() ************************************************
    // ***************************************************************************************************************

    impl.csum ( "Configures all \"defined\" fields." );
    impl.a ( String ( "public override void %1()" ).arg ( _method.SetupDefines ) ).e();
    impl.a ( "{" ).e();
    impl.ae ( 1, String ( "base.%1();" ).arg ( _method.SetupDefines ) );

    for ( HashMap<String, Element * >::Iterator defsIt ( s->getDefines() ); defsIt.isValid(); defsIt.next() )
    {
        Element * e = defsIt.value();

        assert ( e != 0 );

        if ( e->definedTarget != 0 )
        {
            impl.ae ( 1, String ( "%1 ( %2 );" ).arg ( getSetName ( e ), getDefName ( e ) ) );
        }
    }

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        impl.e();

        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        if ( !e->typeSymbol || !e->typeSymbol->isMessageOrStruct() )
            continue;

        String varName = getVarName ( e );

        // Message types are always nullable

        impl.ae ( 1, String ( "if ( %1 != null )" ).arg ( varName ) );
        impl.ae ( 1, "{" );
        impl.incBaseIndent ( 2 );

        if ( e->isRepeated() )
        {
            varName = "listElem";

            impl.ae ( String ( "foreach ( %1 %2 in %3 )" )
                      .arg ( getRawVarType ( e->typeSymbol ), varName, getVarName ( e ) ) );
            impl.ae ( "{" );
            impl.incBaseIndent ( 1 );

            // Message types are always nullable:

            assert ( isNullable ( e->typeSymbol ) );

            impl.ae ( String ( "if ( %1 == null )" ).arg ( varName ) );
            impl.ae ( "{" );
            impl.ae ( 1, "continue;" );
            impl.ae ( "}" ).e();
        }

        impl.ae ( String ( "%1.%2();" ).arg ( varName, _method.SetupDefines ) );

        if ( e->isRepeated() )
        {
            impl.decBaseIndent ( 1 );
            impl.ae ( "}" );
        }

        impl.decBaseIndent ( 2 );
        impl.ae ( 1, "}" );
    }

    impl.ae ( "}" ).e();

    // ***************************************************************************************************************
    // ********************************** deserialize and serialize methods ******************************************
    // ***************************************************************************************************************

    genMsgSerializeFieldsMethod ( s, impl );
    genMsgDeserializeFieldMethod ( s, impl );
}

void CSharpGenerator::genMsgSerializeFieldsMethod ( Symbol * s, CSharpFile & impl )
{
    assert ( s != 0 );

    impl.csum ( "Serializes fields of this ", s->isStruct() ? "structure" : "message" );
    impl.cdesc ( "It just appends content to the buffer." ).e()
    .a ( String ( "First the %1() from inherited object is called." ).arg ( _method.SerializeFields ) ).e()
    .a ( "Next, all present, local fields are serialized." );
    impl.cparam ( "buffer", "Buffer to serialize data to" );
    impl.ae ( String ( "protected override void %1 ( %2 buffer )" )
              .arg ( _method.SerializeFields, _type.WriteBuffer ) );
    impl.ae ( "{" );
    impl.ae ( 1, String ( "base.%1 ( buffer );" ).arg ( _method.SerializeFields ) );

    const StringList & elems = s->getOrdElements();

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

        String varName = getGetName ( e );

        impl.ae ( 1, String ( "if ( %1 )" ).arg ( getHasName ( e ) ) );
        impl.ae ( 1, "{" );
        impl.incBaseIndent ( 2 );

        if ( e->isRepeated() )
        {
            varName = "listElem";

            impl.ae ( String ( "foreach ( %1 %2 in %3 )" )
                      .arg ( getRawVarType ( e->typeSymbol ), varName, getVarName ( e ) ) );
            impl.ae ( "{" );
            impl.incBaseIndent ( 1 );

            if ( isNullable ( e->typeSymbol ) )
            {
                impl.ae ( String ( "if ( %1 == null )" ).arg ( varName ) );
                impl.ae ( "{" );
                impl.ae ( 1, "continue;" );
                impl.ae ( "}" ).e();
            }
        }

        if ( e->typeSymbol->isMessageOrStruct() )
        {
            impl.ae ( String ( "%1 tmpBuffer = new %1();" ).arg ( _type.WriteBuffer ) );
            impl.ae ( String ( "%1.%2 ( tmpBuffer );" ).arg ( varName, _method.SerializeMessage ) );
            impl.ae ( String ( "%1 ( buffer, %2, tmpBuffer );" ).arg ( _method.Encode ).arg ( e->code ) );
        }
        else if ( e->typeSymbol->isEnum() )
        {
            impl.ae ( String ( "%1 ( buffer, %2, (%3) %4 );" )
                      .arg ( _method.Encode ).arg ( e->code ).arg ( _type.Enum, varName ) );
        }
        else
        {
            impl.ae ( String ( "%1 ( buffer, %2, %3 );" )
                      .arg ( _method.Encode ).arg ( e->code ).arg ( varName ) );
        }

        if ( e->isRepeated() )
        {
            impl.decBaseIndent ( 1 );
            impl.ae ( "}" );
        }

        impl.decBaseIndent ( 2 );
        impl.ae ( 1, "}" );
    }

    impl.ae ( "}" ).e().e();
}

void CSharpGenerator::genMsgDeserializeFieldMethod ( Symbol * s, CSharpFile & impl )
{
    assert ( s != 0 );

    // ***************************************************************************************************************
    // *********************************************** deserializeField() ********************************************
    // ***************************************************************************************************************

    impl.csum ( "Deserializes a single field." );
    impl.cdesc ( "If the field ID received is not recognized by this class," ).e()
    .a ( String ( "%1 in the inherited class is called and its return value is returned" )
         .arg ( _method.DeserializeField ) );

    impl.cparam ( "buffer", "Buffer to deserialize the payload from." );
    impl.cparam ( "offset", "Offset in the buffer where the payload starts." );
    impl.cparam ( "fieldId", "Field identifier." );
    impl.cparam ( "payloadSize", "The size of the payload of this field." );
    impl.cparam ( "wireType", "The wire type used by the field." );
    impl.cret ( "<c>true</c>, if field was deserialized, <c>false</c> if it was unknown." );
    impl.cexc ( _type.Exception, "When the decoding failed for some reason." );

    impl.ae ( String ( "protected override bool %1 ( %2 buffer, %3 offset, %4 fieldId, %5 payloadSize, %6 wireType )" )
              .arg ( _method.DeserializeField,
                     _type.ReadBuffer, _type.ReadOffset, _type.FieldId, _type.ReadPayloadSize, _type.WireType ) );
    impl.ae ( "{" );

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
            continue;

        if ( !addedSwitch )
        {
            addedSwitch = true;

            impl.ae ( 1, "switch ( fieldId )" );
            impl.ae ( 1, "{" );
        }

        if ( i > 0 )
            impl.e();

        impl.ae ( 2, String ( "case %1: // %2" ).arg ( e->code ).arg ( getFieldName ( e ) ) );
        impl.ae ( 2, "{" );

        impl.incBaseIndent ( 3 );

        assert ( e->typeSymbol != 0 );

        if ( e->typeSymbol->isMessageOrStruct() )
        {
            impl.ae ( String ( "if ( wireType == %1 || wireType == %2 )" )
                      .arg ( _wireType.VarLenA, _wireType.VarLenB ) );
            impl.ae ( "{" );
            impl.ae ( 1, String ( "throw new %1 ( %2, \"Field %3 (which is a message/struct) cannot be encoded "
                                  "using variable length encoding (\" + wireType + \")\" );" )
                      .arg ( _type.Exception, _error.ProtocolError, getFieldName ( e ) ) );
            impl.ae ( "}" ).e();

            impl.ae ( String ( "%1 newMsg = new %1();" ).arg ( getRawVarType ( e->typeSymbol ) ) );
            impl.ae ( String ( "bool ret = newMsg.%1 ( buffer, offset, payloadSize );" )
                      .arg ( _method.DeserializeMessage ) );

            if ( e->isRepeated() )
            {
                // It's a list - we just need to append the temporary element to it

                impl.ae ( String ( "if ( %1 == null ) %1 = new System.Collections.Generic.List<%2>();" )
                          .arg ( getVarName ( e ), getRawVarType ( e->typeSymbol ) ) );
                impl.ae ( String ( "%1.Add ( newMsg );" ).arg ( getVarName ( e ) ) );
            }
            else
            {
                impl.ae ( String ( "%1 = newMsg;" ).arg ( getVarName ( e ) ) );
            }

            impl.ae ( "return ret;" );
        }
        else
        {
            impl.ae ( String ( "%1 newValue;" )
                      .arg ( ( e->typeSymbol->isEnum() ) ? _type.Enum : getRawVarType ( e->typeSymbol ) ) );
            impl.ae ( String ( "%1 ( buffer, offset, payloadSize, wireType, out newValue);" ).arg ( _method.Decode ) );

            if ( !e->isRepeated() )
            {
                impl.ae ( String ( "%1 = (%2) newValue;" )
                          .arg ( getVarName ( e ), getRawVarType ( e->typeSymbol ) ) );
            }
            else
            {
                // It's a list - we just need to append the temporary element to it
                impl.ae ( String ( "if ( %1 == null ) %1 = new System.Collections.Generic.List<%2>();" )
                          .arg ( getVarName ( e ), getRawVarType ( e->typeSymbol ) ) );

                impl.ae ( String ( "%1.Add ( (%2) newValue );" )
                          .arg ( getVarName ( e ), getRawVarType ( e->typeSymbol ) ) );
            }

            impl.ae ( "return true;" );
        }

        impl.decBaseIndent ( 3 );

        impl.ae ( 2, "}" );
    }

    if ( addedSwitch )
    {
        impl.ae ( 1, "}" ).e();
    }

    impl.ae ( 1, String ( "return base.%1 ( buffer, offset, fieldId, payloadSize, wireType );" )
              .arg ( _method.DeserializeField ) );
    impl.ae ( "}" ).e();

    if ( !s->isBaseMessage() )
        return;

    // Base messages also have a 'DeserializeFromBase' method:

    impl.csum ( "Used to deserialize a massage based on its base message." );
    impl.cdesc ( "It will deserialize the message, and if it works it will validate it." );
    impl.cparam ( "baseMessage", "The base message to deserialize this message based on." );
    impl.cret ( "<c>true</c>, if all the fields were recognized, <c>false</c>, "
                "if deserializing succeeded, but some of the fields were not recognized." );
    impl.cexc ( _type.Exception, "When the message cannot be properly deserialized, "
                "or when it doesn't validate properly" ).e()
    .a ( "(for example, if required fields are missing or have incorrect values." );

    impl.ae ( String ( "public bool %1 ( %2 baseMessage )" ).arg ( _method.DeserializeFromBase, s->getName() ) );
    impl.ae ( "{" );
    impl.ae ( 1, String ( "return base.%1 ( baseMessage );" ).arg ( _method.DeserializeFromBase ) );
    impl.ae ( "}" );
}
