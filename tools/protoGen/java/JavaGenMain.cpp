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
#include "JavaGen.hpp"

using namespace Pravala;

void JavaGenerator::genEnumClass ( Symbol * s, JavaFile & impl, bool nested )
{
    Symbol * enumSym = getEnumStorageSymbol();

    assert ( enumSym != 0 );
    assert ( enumSym->isBasic() );

    hookPosition ( s, impl, PosBeforeClass );

    impl.co();
    impl.c ( "A '" ).a ( s->getName() ).a ( "' enumerator" ).e();
    impl.ce();

    if ( s->getComment().length() > 0 )
    {
        impl.ce();
        impl.c ( s->getName() ).a ( ": " ).a ( s->getComment() ).e();
    }

    impl.cc();

    impl.a ( "public " );

    if ( nested )
        impl.a ( "static " );

    impl.a ( "enum " ).a ( s->getName() ).e();
    impl.a ( "{" ).e();

    impl.incBaseIndent();

    hookPosition ( s, impl, PosClassOpened );

    const StringList & elems = s->getOrdElements();
    bool addedElem = false;
    String comment;

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // This is an enum!
        assert ( !e->isRequired() );
        assert ( !e->isRepeated() );
        assert ( !e->aliasTarget );

        if ( addedElem )
            impl.a ( "," ).a ( comment ).e();

        comment = "";

        if ( e->isEnumDefault() || e->comment.length() > 0 || e->extName.length() > 0 )
        {
            comment.append ( " //" );

            if ( e->comment.length() > 0 || e->extName.length() > 0 )
            {
                comment.append ( "/<" );

                if ( e->comment.length() > 0 )
                {
                    comment.append ( String ( " %1" ).arg ( e->comment ) );
                }

                if ( e->extName.length() > 0 )
                {
                    comment.append ( String ( " String value: '%1'" ).arg ( e->extName ) );
                }
            }

            if ( e->isEnumDefault() )
            {
                comment.append ( " [default]" );
            }
        }

        addedElem = true;

        impl.a ( e->name ).a ( " ( " ).a ( e->value ).a ( " )" );
    }

    if ( addedElem )
        impl.a ( ";" ).a ( comment ).e();

    Element * defElem = s->getEnumDefault();

    assert ( defElem != 0 );

    if ( !defElem )
        throw Error ( "Enumerators have to have a default value!" );

    impl.co();
    impl.ce ( "Returns the default value of the enum class" );
    impl.c ( "@return The default value of the enum class (" ).a ( defElem->name ).a ( ")" ).e();
    impl.cc();
    impl.a ( "public static " ).a ( s->getName() ).a ( " getDefault()" ).e();
    impl.a ( "{" ).e();
    impl.i ( 1 ).a ( "return " ).a ( defElem->name ).a ( ";" ).e();
    impl.a ( "}" ).e();

    genEnumDeserializeMethod ( s, impl );

    impl.co();
    impl.ce ( "Creates an enum based on a 'raw' enum value" );
    impl.ce();
    impl.ce ( "If the raw value is illegal for this enum then this function returns 'null'." );
    impl.ce ( "@param rawValue The raw value that represents the enum" );
    impl.ce ( "@return The newly created enum, or null in case the value was incorrect for this enum" );
    impl.cc();
    impl.a ( "public static " ).a ( s->getName() ).a ( " createFromRaw ( " );
    impl.a ( getRawVarType ( enumSym ) ).a ( " rawValue )" ).e();
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

        impl.i ( 2 ).a ( "case " ).a ( e->value ).a ( ":" ).e();
        impl.i ( 3 ).a ( "return " ).a ( e->name ).a ( ";" ).e();
    }

    impl.i ( 1 ).a ( "}" ).e().e();
    impl.i ( 1 ).a ( "return null;" ).e();
    impl.a ( "}" ).e().e();

    genEnumSerializeMethod ( s, impl );

    impl.co();
    impl.ce ( "Returns the desciption of the enum's value" );
    impl.ce ( "@return String with enum's description" );
    impl.cc();
    impl.a ( "public String toString()" ).e();
    impl.a ( "{" ).e();
    impl.i ( 1 ).a ( "switch ( this )" ).e();
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
    impl.ae ( "}" );

    impl.co();
    impl.ce ( "Private constructor" );
    impl.cc();
    impl.a ( "private " ).a ( s->getName() ).a ( " ( " ).a ( getRawPrimitiveVarType ( enumSym ) ).a ( " value )" ).e();
    impl.a ( "{" ).e();
    impl.i ( 1 ).a ( "this.value = value;" ).e();
    impl.a ( "}" ).e();

    impl.co();
    impl.ce ( "The actual value of the enum" );
    impl.cc();
    impl.a ( "private final " ).a ( getRawVarType ( enumSym ) ).a ( " value;" ).e();

    hookPosition ( s, impl, PosBeforeClassClose );

    impl.decBaseIndent();
    impl.a ( "}" ).e();

    hookPosition ( s, impl, PosClassClosed );
}

void JavaGenerator::genMessageClass ( Symbol * s, JavaFile & impl, bool nested )
{
    hookPosition ( s, impl, PosBeforeClass );

    impl.co();
    impl.ce ( String ( "Class representing '%1' %2" )
              .arg ( s->getName(), s->isStruct() ? "structure" : "message" ) );

    if ( s->getComment().length() > 0 )
    {
        impl.ce();
        impl.c ( s->getName() ).a ( ": " ).a ( s->getComment() ).e();
    }

    impl.cc();

    impl.a ( "public " );

    if ( nested )
        impl.a ( "static " );

    impl.a ( "class " ).a ( s->getName() );

    String extStr = getExtends ( s );

    if ( extStr.length() > 0 )
    {
        impl.a ( " extends " ).a ( extStr );
    }

    StringList implList = getImplements ( s );

    for ( size_t i = 0; i < implList.size(); ++i )
    {
        impl.a ( " implements " ).a ( implList[ i ] );
    }

    impl.e();
    impl.a ( "{" ).e();

    impl.incBaseIndent();

    hookPosition ( s, impl, PosClassOpened );

    assert ( s != 0 );

    const StringList & elems = s->getOrdElements();

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // We don't have field codes for aliases!
        if ( e->aliasTarget != 0 )
            continue;

        impl.co();
        impl.c ( "The Field ID of " ).a ( e->name ).e();
        impl.cc();
        impl.a ( "public static final " ).a ( getStdType ( TypeFieldId ) );
        impl.a ( " " ).a ( getFieldIdName ( e ) ).a ( " = " ).a ( String::number ( e->code ) ).a ( ";" ).e().e();
    }

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

        impl.co();
        impl.ce ( String ( "The value to which %1 will be defined to by this %2" )
                  .arg ( d->name, s->isStruct() ? "structure" : "message" ) );
        impl.cc();
        impl.a ( "public static final " ).a ( getIfaceType ( d ) );
        impl.a ( " " ).a ( getDefName ( e ) ).a ( " = " ).a ( varValue ).a ( ";" ).e().e();
    }

    impl.e();

    const StringList & intSymbols = s->getOrdInternalSymbols();

    for ( size_t i = 0; i < intSymbols.size(); ++i )
    {
        Symbol * intSym = s->getInternalSymbols().value ( intSymbols[ i ] );

        assert ( intSym != 0 );

        genRegularSymbol ( intSym, impl, true );
    }

    genMsgFields ( s, impl );
    genMsgFieldMethods ( s, impl );
    genMsgStdMethods ( s, impl );

    hookPosition ( s, impl, PosBeforeClassClose );

    impl.decBaseIndent();
    impl.a ( "}" ).e();

    hookPosition ( s, impl, PosClassClosed );
}

void JavaGenerator::genMsgFieldMethods ( Symbol * s, JavaFile & impl )
{
    assert ( s != 0 );

    const StringList & elems = s->getOrdElements();

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        impl.co();
        impl.c ( "Checks if '" ).a ( e->getCamelCaseName() );
        impl.a ( "' is set (present) in the " ).a ( s->isStruct() ? "structure" : "message" ).e();

        if ( e->isSalias() )
        {
            impl.ce ( "NOTE: This is an 's-alias', which means it uses one additional presence bit" );
        }

        impl.ce ( "@return True if the value is set (present), false otherwise" );
        impl.cc();
        impl.a ( "public boolean " ).a ( e->getCamelCaseName ( "has" ) ).a ( "()" ).e();
        impl.a ( "{" ).e();

        impl.i ( 1 ).a ( "return ( " );

        if ( e->isRepeated() )
        {
            impl.a ( e->getCamelCaseName ( "count" ) ).a ( "() > 0" );
        }
        else if ( e->aliasTarget != 0 )
        {
            impl.a ( e->aliasTarget->getCamelCaseName ( "has" ) ).a ( "()" );

            if ( e->isSalias() )
            {
                impl.a ( String ( " && ( ( ( %1() >> %2 ) & 0x01 ) == 0x01 )" )
                         .arg ( e->aliasTarget->getCamelCaseName ( "get" ) )
                         .arg ( e->iAliasRangeFrom ) );
            }
        }
        else
        {
            impl.a ( getVarName ( e ) ).a ( " != null" );
        }

        impl.ae ( " );" );
        impl.a ( "}" ).e().e();

        impl.co();
        impl.c ( "Gets the value of '" ).a ( e->getCamelCaseName() ).a ( "'" ).e();

        if ( e->aliasTarget != 0 )
        {
            impl.ce ( "This is the value of a bitrange inside the underlying" );
            impl.c ( "storage type '" ).a ( e->aliasTarget->getCamelCaseName() ).a ( "'" ).e();

            if ( e->isSalias() )
            {
                impl.ce ( "NOTE: This is an 's-alias', which means it uses one additional presence bit" );
            }
        }

        impl.genVarComments ( e );

        impl.c ( "@return The value of '" ).a ( e->getCamelCaseName() ).a ( "'" ).e();
        impl.cc();

        impl.a ( "public " ).a ( getIfaceType ( e ) ).a ( " " );

        if ( !e->aliasTarget )
        {
            // Regular element

            impl.a ( e->getCamelCaseName ( "get" ) ).a ( "()" ).e();
            impl.a ( "{" ).e();

            if ( e->isRepeated() )
            {
                impl.i ( 1 ).a ( "if ( " ).a ( getVarName ( e ) ).a ( " == null )" ).e();
                impl.i ( 1 ).a ( "{" ).e();
                impl.i ( 2 ).a ( getVarName ( e ) ).a ( " = new " ).a ( getVarType ( e ) ).a ( "();" ).e();
                impl.i ( 1 ).a ( "}" ).e();
                impl.e();
            }
            else if ( e->options.contains ( ProtocolSpec::KW_DEFAULT ) )
            {
                impl.i ( 1 ).a ( "if ( !" ).a ( e->getCamelCaseName ( "has" ) ).a ( "() )" ).e();
                impl.i ( 1 ).a ( "{" ).e();
                impl.i ( 2 ).a ( "return " );

                if ( e->typeSymbol != 0 && e->typeSymbol->isEnum() )
                {
                    impl.a ( getVarType ( e ) ).a ( "." );
                }

                impl.a ( getVarLiteral ( e, e->options[ ProtocolSpec::KW_DEFAULT ] ) ).a ( ";" ).e();
                impl.i ( 1 ).a ( "}" ).e().e();
            }

            impl.i ( 1 ).a ( "return " );
            impl.a ( getVarName ( e ) );
        }
        else
        {
            // Alias
            assert ( e->aliasTarget != 0 );
            assert ( e->getAliasPayloadBitLength() > 0 );

            const int numBits = e->getAliasPayloadBitLength();

            assert ( numBits > 0 );

            impl.a ( e->getCamelCaseName ( "get" ) ).a ( "()" ).e();
            impl.a ( "{" ).e();

            impl.i ( 1 ).a ( "if ( !" ).a ( e->getCamelCaseName ( "has" ) ).a ( "() )" ).e();
            impl.i ( 1 ).a ( "{" ).e();

            String cast = "";

            if ( numBits > 1 )
            {
                cast = String ( "( " ).append ( getRawPrimitiveVarType ( getAliasStorageSymbol ( numBits ) ) );
                cast.append ( " ) " );
            }

            if ( e->options.contains ( ProtocolSpec::KW_DEFAULT ) )
            {
                impl.i ( 2 ).a ( "return " ).a ( cast )
                .a ( getVarLiteral ( e, e->options[ ProtocolSpec::KW_DEFAULT ] ) )
                .a ( ";" ).e();
            }
            else
            {
                if ( numBits > 1 )
                {
                    impl.i ( 2 ).a ( "return " ).a ( cast ).a ( "0;" ).e();
                }
                else
                {
                    impl.i ( 2 ).a ( "return false;" ).e();
                }
            }

            impl.i ( 1 ).a ( "}" ).e();
            impl.e();
            impl.i ( 1 ).a ( "return " );

            impl.a ( cast ).a ( "( ( " ).a ( e->aliasTarget->getCamelCaseName ( "get" ) ).a ( "()" );

            if ( e->iAliasPayloadRangeFrom > 0 )
            {
                impl.a ( " >> " ).a ( String::number ( e->iAliasPayloadRangeFrom ) );
            }

            impl.a ( " ) & " ).a ( getBitmask ( numBits ) ).a ( " )" );

            if ( numBits == 1 )
                impl.a ( " == 1" );
        }

        impl.a ( ";" ).e();
        impl.a ( "}" ).e().e();

        if ( e->isRepeated() )
        {
            impl.co();
            impl.c ( "Returns the number of '" ).a ( e->getCamelCaseName() ).a ( "' elements" ).e();
            impl.ce ( "@return The size of the list of the elements" );
            impl.cc();
            impl.a ( "public int " ).a ( e->getCamelCaseName ( "count" ) ).a ( "()" ).e();
            impl.a ( "{" ).e();

            impl.i ( 1 ).a ( "if ( " ).a ( getVarName ( e ) ).a ( " == null )" ).e();
            impl.i ( 1 ).a ( "{" ).e();
            impl.i ( 2 ).a ( "return 0;" ).e();
            impl.i ( 1 ).a ( "}" ).e().e();

            impl.i ( 1 ).a ( "return " ).a ( exprListVarSize ( e->typeSymbol, getVarName ( e ) ) ).a ( ";" ).e();
            impl.a ( "}" ).e().e();
        }

        String accessModif;

        if ( e->isPublic() )
        {
            accessModif = "public ";
        }
        else if ( e->isProtected() )
        {
            accessModif = "protected ";
        }
        else
        {
            accessModif = "private ";
        }

        bool addUnset = false;

        if ( e->aliasTarget != 0 )
        {
            // No 'repeated' aliases!
            assert ( !e->isRepeated() );

            impl.co();
            impl.c ( "Sets the value of bit range (alias) '" ).a ( e->getCamelCaseName() ).a ( "'" ).e();

            if ( e->isSalias() )
            {
                addUnset = true;
                impl.ce ( "NOTE: This is an 's-alias', which means it uses one additional presence bit" );
            }

            impl.genVarComments ( e );

            impl.ce ( "@param newValue The value to set" );

            if ( !e->usesFullType() )
            {
                impl.ce ( "@return True if the value was set properly," );
                impl.ce ( "        False otherwise (for example if the new value is not in allowed range)" );
            }

            impl.cc();

            if ( !e->usesFullType() )
            {
                impl.a ( accessModif ).a ( "boolean " ).a ( e->getCamelCaseName ( "bset" ) );
            }
            else
            {
                impl.a ( accessModif ).a ( "void " ).a ( e->getCamelCaseName ( "set" ) );
            }

            impl.a ( " ( " ).a ( getIfaceType ( e ) ).a ( " newValue )" ).e();
            impl.a ( "{" ).e();

            const int numBits = e->getAliasPayloadBitLength();

            assert ( numBits > 0 );

            String bMask = getBitmask ( numBits );

            if ( !e->usesFullType() )
            {
                impl.i ( 1 ).a ( "// It's quite possible, that the type used for the argument" ).e();
                impl.i ( 1 ).a ( "// of this function is bigger then the bit length of this field." ).e();
                impl.i ( 1 ).a ( "// We can use the bitmask to see if the actual value can fit in the number" ).e();
                impl.i ( 1 ).a ( "// of bits that we have! If not - return false and don't do anything" ).e();
                impl.i ( 1 ).a ( "if ( ( newValue & " ).a ( bMask ).a ( " ) != newValue )" ).e();
                impl.i ( 1 ).a ( "{" ).e();
                impl.i ( 2 ).a ( "return false;" ).e();
                impl.i ( 1 ).a ( "}" ).e().e();
            }

            impl.i ( 1 ).a ( getRawPrimitiveVarType ( e->aliasTarget->typeSymbol ) ).a ( " curVal;" ).e().e();
            impl.i ( 1 ).a ( "if ( " ).a ( e->aliasTarget->getCamelCaseName ( "has" ) ).a ( "() )" ).e();
            impl.i ( 1 ).a ( "{" ).e();
            impl.i ( 2 ).a ( "curVal = " ).a ( e->aliasTarget->getCamelCaseName ( "get" ) ).a ( "();" ).e();
            impl.i ( 1 ).a ( "}" ).e();
            impl.i ( 1 ).a ( "else" ).e();
            impl.i ( 1 ).a ( "{" ).e();
            impl.i ( 2 ).a ( "curVal = 0;" ).e();
            impl.i ( 1 ).a ( "}" ).e().e();

            if ( numBits == 1 )
            {
                impl.ae ( 1, "if ( newValue )" );
                impl.ae ( 1, "{" );

                if ( e->isSalias() )
                    impl.ae ( 2, "// This also sets the presence bit of this s-alias:" );

                impl.ae ( 2, String ( "%1 ( ( %2 ) ( curVal | ( %3 << %4 ) ) );" )
                          .arg ( e->aliasTarget->getCamelCaseName ( "set" ) )
                          .arg ( getRawPrimitiveVarType ( e->aliasTarget->typeSymbol ) )
                          .arg ( e->isSalias() ? "0x03" : "0x01" )
                          .arg ( e->isSalias() ? ( e->iAliasRangeFrom ) : ( e->iAliasPayloadRangeFrom ) ) );

                impl.ae ( 1, "}" );
                impl.ae ( 1, "else" );
                impl.ae ( 1, "{" );

                if ( e->isSalias() )
                    impl.ae ( 2, "// This also sets the presence bit of this s-alias:" );

                impl.ae ( 2, String ( "%1 ( ( %2 ) ( ( curVal & ( ~( 0x01 << %3 ) ) )%4%5%6 ) );" )
                          .arg ( e->aliasTarget->getCamelCaseName ( "set" ) )
                          .arg ( getRawPrimitiveVarType ( e->aliasTarget->typeSymbol ) )
                          .arg ( e->iAliasPayloadRangeFrom )
                          .arg ( e->isSalias() ? " | ( 0x01 << " : "" )
                          .arg ( e->isSalias() ? ( String::number ( e->iAliasRangeFrom ) ) : "" )
                          .arg ( e->isSalias() ? " )" : "" ) );

                impl.ae ( 1, "}" );
            }
            else
            {
                // We want to get this:
                // ( ( x & (~ ( bmask << payloadrangefrom ) ) ) | ( ( newval & bmask ) << payloadrangefrom ) )
                // and optional: | ( 1 << rangefrom )

                if ( e->isSalias() )
                    impl.ae ( 1, "// This also sets the presence bit of this s-alias:" );

                impl.a ( 1, String ( "%1 ( ( %2 ) ( ( curVal & ( ~( %3 << %4 ) ) ) | ( ( newValue & %3 ) << %4 )" )
                         .arg ( e->aliasTarget->getCamelCaseName ( "set" ) )
                         .arg ( getRawPrimitiveVarType ( e->aliasTarget->typeSymbol ) )
                         .arg ( bMask )
                         .arg ( e->iAliasPayloadRangeFrom ) );

                if ( e->isSalias() )
                {
                    impl.a ( String ( " | ( 0x01 << %1 )" ).arg ( e->iAliasRangeFrom ) );
                }

                impl.ae ( " ) );" ).e();
            }

            impl.e();

            if ( !e->usesFullType() )
                impl.i ( 1 ).a ( "return true;" ).e();

            impl.a ( "}" ).e().e();
        }
        else
        {
            impl.co();
            impl.c ( "Sets the value of '" ).a ( e->getCamelCaseName() ).a ( "'" ).e();

            impl.genVarComments ( e );

            impl.ce ( "@param newValue The value to set" );
            impl.cc();
            impl.a ( accessModif ).a ( "void " ).a ( e->getCamelCaseName ( "set" ) ).a ( " ( " );
            impl.a ( getIfaceType ( e ) ).a ( " newValue )" ).e();
            impl.a ( "{" ).e();

            impl.i ( 1 ).a ( getVarName ( e ) ).a ( " = newValue;" ).e();
            impl.a ( "}" ).e().e();

            addUnset = true;
        }

        if ( addUnset && !e->isPrivate() )
        {
            impl.co();
            impl.c ( "Marks the value of '" ).a ( e->name ).a ( "' as 'not present'" ).e();

            if ( e->isRepeated() )
                impl.ce ( "This clears the list of elements stored in this field" );

            impl.c ( "From now on, the " ).a ( e->getCamelCaseName ( "has" ) );
            impl.a ( "() function will return false" ).e();
            impl.cc();
            impl.a ( accessModif ).a ( "void " ).a ( e->getCamelCaseName ( "unset" ) ).a ( "()" ).e();
            impl.ae ( "{" );

            if ( e->isSalias() )
            {
                impl.ae ( 1, "// Nothing to do if the underlying field is not set (we don't want to set it)" );

                impl.ae ( 1, String ( "if ( !%1() ) return;" )
                          .arg ( e->aliasTarget->getCamelCaseName ( "has" ) ) ).e();

                impl.ae ( 1, String ( "%1 ( ( %2 ) ( %3() & ( ~( %4 << %5 ) ) ) );" )
                          .arg ( e->aliasTarget->getCamelCaseName ( "set" ) )
                          .arg ( getRawPrimitiveVarType ( e->aliasTarget->typeSymbol ) )
                          .arg ( e->aliasTarget->getCamelCaseName ( "get" ) )
                          .arg ( getBitmask ( e->getAliasPayloadBitLength() + 1 ) )
                          .arg ( e->iAliasRangeFrom ) );
            }
            else if ( e->isRepeated() )
            {
                impl.i ( 1 ).a ( "if ( " ).a ( getVarName ( e ) ).a ( " != null )" ).e();
                impl.i ( 1 ).a ( "{" ).e();
                impl.i ( 2 ).a ( exprVarClear ( e ) ).a ( ";" ).e();
                impl.i ( 1 ).a ( "}" ).e();
            }
            else
            {
                impl.i ( 1 ).a ( getVarName ( e ) ).a ( " = null;" ).e();
            }

            impl.ae ( "}" ).e();
        }
    }
}

void JavaGenerator::genMsgFields ( Symbol * s, JavaFile & impl )
{
    assert ( s != 0 );

    const StringList & elems = s->getOrdElements();

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // Aliases don't have their own fields!
        if ( e->aliasTarget != 0 )
            continue;

        String comment;

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

        impl.ae ( String ( "private %1 %2 = null; %3" ).arg ( getIfaceType ( e ), getVarName ( e ), comment ) );
    }

    impl.e();
}

void JavaGenerator::genMsgStdMethods ( Symbol * s, JavaFile & impl )
{
    assert ( s != 0 );

    const StringList & elems = s->getOrdElements();

    // ***************************************************************************************************************
    // **************************************************** clear() **************************************************
    // ***************************************************************************************************************

    impl.ae ( "@Override" );
    impl.a ( "public void clear()" ).e();
    impl.a ( "{" ).e();

    if ( s->getInheritance() != 0 )
    {
        impl.i ( 1 ).a ( "super.clear();" ).e().e();
    }

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        // We can't clear individual aliases, all of them will be
        // cleared once the storage field is cleared.
        if ( !e->aliasTarget )
        {
            assert ( e->typeSymbol != 0 );

            if ( !e->isRepeated() )
            {
                impl.i ( 1 ).a ( getVarName ( e ) ).a ( " = null;" ).e();
            }
            else
            {
                impl.i ( 1 ).a ( "if ( " ).a ( getVarName ( e ) ).a ( " != null )" ).e();
                impl.i ( 1 ).a ( "{" ).e();
                impl.i ( 2 ).a ( exprVarClear ( e ) ).a ( ";" ).e();
                impl.i ( 1 ).a ( "}" ).e().e();
            }
        }
    }

    impl.a ( "}" ).e().e();

    // ***************************************************************************************************************
    // *************************************************** validate() ************************************************
    // ***************************************************************************************************************

    impl.ae ( "@Override" );
    impl.ae ( String ( "public void validate() throws %1" ).arg ( getStdType ( TypeProtoException ) ) );
    impl.a ( "{" ).e();

    if ( s->getInheritance() != 0 )
    {
        impl.ae ( 1, "super.validate();" ).e();
    }

    for ( HashMap<String, Element * >::Iterator defsIt ( s->getDefines() ); defsIt.isValid(); defsIt.next() )
    {
        Element * e = defsIt.value();

        assert ( e != 0 );

        if ( !e->definedTarget )
            continue;

        impl.i ( 1 ).a ( "if ( !" ).a ( e->getCamelCaseName ( "has" ) );
        impl.a ( "() || !" ).a ( getDefName ( e ) ).a ( ".equals ( " );

        impl.a ( e->getCamelCaseName ( "get" ) ).a ( "() ) )" ).e();
        impl.i ( 1 ).a ( "{" ).e();
        impl.ae ( 2, String ( "%1;" ).arg ( exprThrowException ( getErrorCode ( ErrDefinedValueMismatch ) ) ) );
        impl.i ( 1 ).a ( "}" ).e();
    }

    impl.e();

    const String retReqNotSet = String ( "%1;" )
                                .arg ( exprThrowException ( getErrorCode ( ErrRequiredFieldNotSet ) ) );
    const String retValOutRange = String ( "%1;" )
                                  .arg ( exprThrowException ( getErrorCode ( ErrFieldValueOutOfRange ) ) );
    const String retStrLenOutRange = String ( "%1;" )
                                     .arg ( exprThrowException ( getErrorCode ( ErrStringLengthOutOfRange ) ) );
    const String retListSizeOutRange = String ( "%1;" )
                                       .arg ( exprThrowException ( getErrorCode ( ErrListSizeOutOfRange ) ) );

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        Element * e = s->getElements().value ( elems[ i ] );
        String varName = getVarName ( e );

        assert ( e != 0 );

        impl.e();

        if ( e->isRequired() )
        {
            impl.i ( 1 ).a ( "if ( !" ).a ( e->getCamelCaseName ( "has" ) ).a ( "() )" ).e();
            impl.i ( 1 ).a ( "{" ).e();
            impl.i ( 2 ).a ( retReqNotSet ).e();
            impl.i ( 1 ).a ( "}" ).e();
        }

        if ( e->isRepeated() )
        {
            if ( e->options.contains ( ProtocolSpec::KW_MIN_LIST_SIZE ) )
            {
                impl.i ( 1 ).a ( "if ( " ).a ( e->getCamelCaseName ( "count" ) );
                impl.a ( "() < " ).a ( e->options[ ProtocolSpec::KW_MIN_LIST_SIZE ] ).a ( " )" ).e();
                impl.i ( 1 ).a ( "{" ).e();
                impl.i ( 2 ).a ( retListSizeOutRange ).e();
                impl.i ( 1 ).a ( "}" ).e();
            }

            if ( e->options.contains ( ProtocolSpec::KW_MAX_LIST_SIZE ) )
            {
                impl.i ( 1 ).a ( "if ( " ).a ( e->getCamelCaseName ( "count" ) );
                impl.a ( "() > " ).a ( e->options[ ProtocolSpec::KW_MAX_LIST_SIZE ] ).a ( " )" ).e();
                impl.i ( 1 ).a ( "{" ).e();
                impl.i ( 2 ).a ( retListSizeOutRange ).e();
                impl.i ( 1 ).a ( "}" ).e();
            }

            // We set this AFTER the list size stuff. We wanted to use the 'list' variable,
            // but from now on we want to use the 'varRef' instead
            varName = "varRef";
        }

        impl.e();

        StringList conditions;

        if ( e->options.contains ( ProtocolSpec::KW_MIN_LENGTH ) )
        {
            conditions.append ( String ( "if ( " ).append ( exprStringVarLength ( varName ) )
                                .append ( " < " ).append ( e->options[ ProtocolSpec::KW_MIN_LENGTH ] )
                                .append ( " ) " ).append ( retStrLenOutRange ) );
        }

        if ( e->options.contains ( ProtocolSpec::KW_MAX_LENGTH ) )
        {
            conditions.append ( String ( "if ( " ).append ( exprStringVarLength ( varName ) )
                                .append ( " > " ).append ( e->options[ ProtocolSpec::KW_MAX_LENGTH ] )
                                .append ( " ) " ).append ( retStrLenOutRange ) );
        }

        if ( e->options.contains ( ProtocolSpec::KW_MIN ) )
        {
            conditions.append ( String ( "if ( " ).append ( varName )
                                .append ( " < " ).append ( getVarLiteral ( e, e->options[ ProtocolSpec::KW_MIN ] ) )
                                .append ( " ) " ).append ( retValOutRange ) );
        }

        if ( e->options.contains ( ProtocolSpec::KW_MAX ) )
        {
            conditions.append ( String ( "if ( " ).append ( varName )
                                .append ( " > " ).append ( getVarLiteral ( e, e->options[ ProtocolSpec::KW_MAX ] ) )
                                .append ( " ) " ).append ( retValOutRange ) );
        }

        // We need to call 'validate' in embedded messages
        if ( e->typeSymbol != 0 && e->typeSymbol->isMessageOrStruct() )
        {
            conditions.append ( String ( "%1.validate();" ).arg ( varName ) );
        }

        if ( conditions.size() > 0 )
        {
            if ( e->isRepeated() )
            {
                impl.ae ( String ( "for ( int i = 0, lSize = %1(); i < lSize; ++i )" )
                          .arg ( e->getCamelCaseName ( "count" ) ) );

                impl.ae ( 1, "{" );
                impl.ae ( 2, String ( "%1 %2 = %3;" )
                          .arg ( getRawVarType ( e->typeSymbol ), varName,
                                 exprListGetElemIdx ( e->typeSymbol, getVarName ( e ), "i" ) ) ).e();
            }
            else
            {
                impl.ae ( 1, String ( "if ( %1() )" ).arg ( e->getCamelCaseName ( "has" ) ) );
                impl.ae ( 1, "{" );
            }

            for ( size_t idx = 0; idx < conditions.size(); ++idx )
            {
                impl.ae ( 2, conditions[ idx ] );
            }

            impl.ae ( 1, "}" );
        }
    }

    impl.ae ( "}" ).e();

    // ***************************************************************************************************************
    // *********************************************** setupDefines() ************************************************
    // ***************************************************************************************************************

    impl.ae ( "@Override" );
    impl.a ( "public void setupDefines()" ).e();
    impl.a ( "{" ).e();

    if ( s->getInheritance() != 0 )
    {
        impl.i ( 1 ).a ( "super.setupDefines();" ).e().e();
    }

    for ( HashMap<String, Element * >::Iterator defsIt ( s->getDefines() ); defsIt.isValid(); defsIt.next() )
    {
        Element * e = defsIt.value();

        assert ( e != 0 );

        if ( !e->definedTarget )
            continue;

        impl.i ( 1 );

        if ( e->definedTarget->aliasTarget != 0 && !e->definedTarget->usesFullType() )
        {
            impl.a ( "b" );
        }

        impl.a ( e->getCamelCaseName ( "set" ) ).a ( " ( " ).a ( getDefName ( e ) ).a ( " );" ).e();
    }

    for ( size_t i = 0; i < elems.size(); ++i )
    {
        impl.e();

        Element * e = s->getElements().value ( elems[ i ] );

        assert ( e != 0 );

        if ( !e->typeSymbol || !e->typeSymbol->isMessageOrStruct() )
            continue;

        String varName = getVarName ( e );
        int ind = 1;

        if ( e->isRepeated() )
        {
            impl.i ( 1 ).a ( "for ( int i = 0, lSize = " );
            impl.a ( e->getCamelCaseName ( "count" ) ).a ( "(); i < lSize; ++i )" ).e();
            impl.i ( 1 ).a ( "{" );

            ind = 2;
            varName = exprListGetElemIdx ( e->typeSymbol, getVarName ( e ), "i" );
        }

        impl.i ( ind ).a ( "if ( " ).a ( varName ).a ( " != null )" ).e();
        impl.i ( ind ).a ( "{" ).e();
        impl.i ( ind + 1 ).a ( varName ).a ( ".setupDefines();" ).e();
        impl.i ( ind ).a ( "}" ).e();

        if ( e->isRepeated() )
        {
            impl.i ( 1 ).a ( "}" ).e();
        }
    }

    impl.a ( "}" ).e().e();

    // ***************************************************************************************************************
    // ********************************** deserialize and serialize methods ******************************************
    // ***************************************************************************************************************

    genMsgDeserializeMethod ( s, impl );
    genMsgSerializeMethod ( s, impl );
}
