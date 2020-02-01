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

#include <cstdio>
#include <cassert>
#include <cctype>

#include "Symbol.hpp"
#include "Error.hpp"

using namespace Pravala;

// Root node
Symbol::Symbol ( ProtocolSpec & protoSpec ):
    _proto ( protoSpec ),
    _procFunc ( procModifierOrTypeOrSymbolEnd ),
    _parent ( 0 ),
    _inheritance ( 0 ),
    _canBeNegative ( false ),
    _bitLength ( 0 ),
    _isFinished ( true ),
    _symType ( SymNamespace ),
    _specType ( SpecTypeDefault ),
    _generated ( protoSpec.isGenerateMode() ),
    _curIntSymbol ( 0 ),
    _curElement ( 0 )
{
}

Symbol::Symbol (
        Symbol & parent, SymType symType, const String & name,
        const String & path, const String & comment,
        SpecBasicType specType ):
    _proto ( parent._proto ),
    _procFunc ( 0 ),
    _parent ( &parent ),
    _inheritance ( 0 ),
    _canBeNegative ( false ),
    _bitLength ( 0 ),
    _isFinished ( false ),
    _symType ( symType ),
    _specType ( specType ),
    _generated ( _proto.isGenerateMode() ),
    _protoFilePath ( _proto.getProtoFilePath() ),
    _name ( name ),
    _path ( path ),
    _comment ( comment ),
    _curIntSymbol ( 0 ),
    _curElement ( 0 )
{
    if ( _generated )
    {
        Symbol * p = _parent;

        while ( p != 0 && p->isNamespace() )
        {
            p->_generated = true;
            p = p->_parent;
        }
    }

    if ( isMessageOrStruct() )
    {
        _procFunc = procSymOpenBracketOrInheritBeg;
    }
    else if ( isTypedef() )
    {
        _procFunc = procSymInheritBeg;
    }
    else if ( isBasic() )
    {
        _procFunc = 0;
        _isFinished = true;
    }
    else
    {
        _procFunc = procSymOpenBracket;
    }
}

Symbol::~Symbol()
{
    for ( HashMap<String, Element *>::Iterator it ( _elements ); it.isValid(); it.next() )
    {
        assert ( it.value() != 0 );

        delete it.value();
    }

    _elements.clear();

    for ( HashMap<String, Symbol *>::Iterator it ( _internalSymbols ); it.isValid(); it.next() )
    {
        assert ( it.value() != 0 );

        delete it.value();
    }

    _internalSymbols.clear();
}

const String Symbol::getProtoFileBaseName() const
{
    if ( _protoFilePath.isEmpty() )
        return String::EmptyString;

    int idx = _protoFilePath.findLastOf ( "/" );

    String name = _protoFilePath.substr ( ( idx < 0 ) ? 0 : ( idx + 1 ) );

    return name.substr ( 0, name.findLastOf ( "." ) );
}

void Symbol::reopenNamespace()
{
    assert ( isNamespace() );
    assert ( _isFinished );
    assert ( !_procFunc );

    _isFinished = false;
    _procFunc = procSymOpenBracket;
}

bool Symbol::appendToken ( const String & token, const String & comment )
{
    _curComment = comment.trimmed();

    if ( !_curComment.isEmpty() )
    {
        const char c = _curComment[ _curComment.length() - 1 ];

        if ( c != '.' && c != ';' && c != '!' && c != '?' )
        {
            _curComment.append ( "." );
        }
    }

    assert ( _procFunc != 0 );

    if ( !_procFunc )
        throw Error ( "Internal error. Symbol::appendToken called with no processor set" );

    return _procFunc ( *this, token );
}

bool Symbol::procExpSymbol (
        const String & token, const char * expToken,
        bool ( * nextFunc )( Symbol &, const String & ) )
{
    assert ( nextFunc != 0 );

    if ( token != expToken )
    {
        throw Error ( "Unexpected token: '", token,
                      "'. Expected: '", expToken, "'" );
    }

    _procFunc = nextFunc;
    return false;
}

bool Symbol::procInternalSymbol ( Symbol & self, const String & token )
{
    assert ( self._curIntSymbol != 0 );

    if ( self._curIntSymbol->appendToken ( token, self._curComment ) )
    {
#ifndef NDEBUG
        if ( self._curIntSymbol->isTypedef() )
        {
            assert ( self._typedefSymbols.contains ( unifiedName ( self._curIntSymbol->_name ) ) );

            Symbol * intSym = self._typedefSymbols[ unifiedName ( self._curIntSymbol->_name ) ];

            assert ( intSym == self._curIntSymbol );
            assert ( intSym->isTypedef() != 0 );
            assert ( intSym->_inheritance != 0 );
            assert ( intSym->_inheritance->isBasic() );
        }
#endif

        self._curIntSymbol = 0;
        self._procFunc = procModifierOrTypeOrSymbolEnd;
    }

    return false;
}

Symbol * Symbol::createBasicRootType ( const String & name, SpecBasicType specType )
{
    return createBasicRootType ( name, 0, false, specType );
}

Symbol * Symbol::createBasicRootType (
        const String & name,
        int bitLength,
        bool setCanBeNegative,
        SpecBasicType specType )
{
    assert ( isRoot() );
    assert ( bitLength >= 0 );

    if ( !isRoot() )
        return 0;

    if ( _internalSymbols.contains ( unifiedName ( name ) ) )
        return _internalSymbols.value ( unifiedName ( name ) );

    Symbol * s = new Symbol ( *this, SymBasic, name, name, String::EmptyString, specType );

    s->_bitLength = bitLength;
    s->_canBeNegative = setCanBeNegative;

    _internalSymbols[ unifiedName ( name ) ] = s;
    _ordIntSymbols.append ( unifiedName ( name ) );

    return s;
}

void Symbol::checkName ( const ProtocolSpec & pSpec, const String & name, bool isTypeName )
{
    if ( isTypeName && pSpec.reservedNames.contains ( name ) )
    {
        throw Error ( "Could not use '", name, "' as a name - it is a reserved keyword" );
    }

    if ( name.find ( "." ) >= 0 )
    {
        throw Error ( "The name '", name, "' is incorrect. Names cannot contain '.' characters" );
    }

    String tmpName = name.replace ( "_", "a" );

    for ( int i = 0; i < tmpName.length(); ++i )
    {
        if ( !isalnum ( tmpName[ i ] ) )
        {
            throw Error ( "Could not use '", name, "' as a name - it contains illegal characters" );
        }
    }

    for ( int i = 0; i < name.length(); ++i )
    {
        if ( name[ i ] == '_' || isalpha ( name[ i ] ) )
            return;
    }

    throw Error ( "Could not use '", name, "' as a name - it needs at least one non-digit character" );
}

Symbol::SymType Symbol::getSymType ( const String & sType )
{
    if ( sType == ProtocolSpec::KW_NAMESPACE )
        return SymNamespace;

    if ( sType == ProtocolSpec::KW_MESSAGE )
        return SymMessage;

    if ( sType == ProtocolSpec::KW_STRUCT )
        return SymStruct;

    if ( sType == ProtocolSpec::KW_ENUM )
        return SymEnum;

    if ( sType == ProtocolSpec::KW_TYPEDEF )
        return SymTypedef;

    return SymUnknown;
}

Element::ElemAccess Symbol::getElemAccess ( const String & sAccess )
{
    if ( sAccess == ProtocolSpec::KW_PROTECTED )
        return Element::AccessProtected;

    if ( sAccess == ProtocolSpec::KW_PRIVATE )
        return Element::AccessPrivate;

    if ( sAccess == ProtocolSpec::KW_PUBLIC )
        return Element::AccessPublic;

    return Element::AccessUnknown;
}

Element::ElemRule Symbol::getElemRule ( const String & sRule )
{
    if ( sRule == ProtocolSpec::KW_OPTIONAL )
        return Element::RuleOptional;

    if ( sRule == ProtocolSpec::KW_REPEATED )
        return Element::RuleRepeated;

    if ( sRule == ProtocolSpec::KW_REQUIRED )
        return Element::RuleRequired;

    return Element::RuleUnknown;
}

void Symbol::checkCurDefined()
{
    if ( _curElement->rule != Element::RuleUnknown )
        throw Error ( "Rules cannot be used for defines" );

    if ( _curElement->access != Element::AccessUnknown )
        throw Error ( "Access modes can not be used for defines" );

    if ( _curElement->isUnique )
        throw Error ( "'Unique' modifier can not be used for defines" );

    // TODO: Anything else?
    return;
}

void Symbol::checkCurAlias()
{
    if ( _curElement->rule != Element::RuleUnknown && !_curElement->isSalias() )
        throw Error ( "Rules cannot be used for basic aliases" );

    bool okA = false;
    bool okB = false;

    _curElement->iAliasRangeFrom = _curElement->strAliasRangeFrom.toInt32 ( &okA );
    _curElement->iAliasRangeTo = _curElement->strAliasRangeTo.toInt32 ( &okB );

    if ( !okA || !okB )
    {
        throw Error ( "Incorrect alias range specified" );
    }

    _curElement->iAliasPayloadRangeFrom = _curElement->iAliasRangeFrom;

    if ( _curElement->iAliasRangeFrom < 0 || _curElement->iAliasRangeTo < 0 )
        throw Error ( "Range boundaries of an alias can not be negative" );

    if ( _curElement->iAliasRangeFrom > _curElement->iAliasRangeTo )
        throw Error ( "First value in the alias' range can not be greater than the second one" );

    assert ( _curElement->aliasTargetBitLength > 0 );

    if ( _curElement->iAliasRangeTo >= _curElement->aliasTargetBitLength )
        throw Error ( "Alias' range is too big. The target's type can store up to ",
                      String::number ( _curElement->aliasTargetBitLength ), " bytes" );

    if ( _curElement->isSalias() )
    {
        if ( _curElement->iAliasRangeFrom < 1 )
            throw Error ( "S-Aliases use one extra bit to mark field's presence. "
                          "The lowest allowed start bit range for s-aliases is 1, not 0." );

        --_curElement->iAliasRangeFrom;
    }

    return;
}

void Symbol::checkCurBasic()
{
    if ( isMessageOrStruct() && _curElement->rule == Element::RuleUnknown )
    {
        throw Error ( "Missing rule for the element (possible rules: required repeated optional)" );
    }
}

void Symbol::checkCompleteSymbol()
{
    if ( isNamespace() )
        return;

    if ( isTypedef() )
    {
        if ( !_inheritance )
            throw Error ( "Typedef '", _name, "' does not define any other type" );

        if ( !_inheritance->isBasic() )
            throw Error ( "Typedef '", _name, "' should define a basic symbol" );

        if ( !_parent )
            throw Error ( "Typedef '", _name, "' does not have a parent" );

        if ( _parent->_internalSymbols.contains ( unifiedName ( _name ) )
             || _parent->_typedefSymbols.contains ( unifiedName ( _name ) ) )
            throw Error ( "The parent of typedef '", _name, "' already has the symbol with the same name" );

        _parent->_typedefSymbols[ unifiedName ( _name ) ] = this;
        return;
    }

    if ( isEnum() )
    {
        if ( _elements.size() < 1 )
            throw Error ( "Enum '", _name, "' does not have any elements" );

        if ( !getEnumDefault() )
            throw Error ( "Enum '", _name, "' does not have the default element (add '",
                          ProtocolSpec::KW_DEFAULT, "' option with no value to one of the elements)" );

        return;
    }

    // Check if aliases in each inheritance list use different bits:
    if ( isMessageOrStruct() )
    {
        HashSet<Element *> clearedElems;

        Symbol * sym = this;

        while ( sym != 0 )
        {
            for ( HashMap<String, Element *>::Iterator it ( sym->_elements ); it.isValid(); it.next() )
            {
                Element * e = it.value();

                if ( e->aliasTarget != 0 || e->isAlias() )
                {
                    assert ( e->aliasTarget != 0 );
                    assert ( e->typeName == ProtocolSpec::KW_ALIAS || e->typeName == ProtocolSpec::KW_SALIAS );

                    if ( !clearedElems.contains ( e->aliasTarget ) )
                    {
                        clearedElems.insert ( e->aliasTarget );

                        e->aliasTarget->tmpAliasedBits.clear();
                    }

                    for ( int i = e->iAliasRangeFrom; i <= e->iAliasRangeTo; ++i )
                    {
                        assert ( i >= 0 );

                        if ( e->aliasTarget->tmpAliasedBits.contains ( i ) )
                            throw Error ( "Alias bit range collision (in bit ", String::number ( i ),
                                          "); Alias '", e->name, "' in '", sym->_path,
                                          "' collides with ", e->aliasTarget->tmpAliasedBits[ i ],
                                          "; Note that 's-aliases' use one additional bit before the actual payload" );

                        String desc ( "'" );
                        desc.append ( e->name );
                        desc.append ( "' in '" );
                        desc.append ( sym->_path );
                        desc.append ( "'" );

                        e->aliasTarget->tmpAliasedBits[ i ] = desc;
                    }
                }
            }

            sym = sym->_inheritance;
        }
    }
}

Symbol * Symbol::tryPath ( Symbol * inSymbol, const StringList & path )
{
    assert ( path.size() > 0 );

    StringList nextPath = path;

    assert ( nextPath.size() > 0 );

    nextPath.removeIndex ( 0 );

    for ( HashMap<String, Symbol *>::Iterator it ( inSymbol->_internalSymbols ); it.isValid(); it.next() )
    {
        if ( it.value()->_name == path.at ( 0 ) )
        {
            if ( nextPath.isEmpty() )
                return it.value();

            Symbol * found = tryPath ( it.value(), nextPath );

            if ( found != 0 )
                return found;
        }
    }

    if ( nextPath.isEmpty() )
    {
        for ( HashMap<String, Symbol *>::Iterator it ( inSymbol->_typedefSymbols ); it.isValid(); it.next() )
        {
            if ( it.value()->_name == path.at ( 0 ) )
            {
                assert ( it.value() != 0 );
                assert ( it.value()->_inheritance != 0 );

                return it.value()->_inheritance;
            }
        }
    }

    return 0;
}

Symbol * Symbol::findUsableSymbol ( const String & path )
{
    Symbol * sym = findSymbol ( path );

    if ( !sym )
    {
        throw Error ( "Symbol '", path, "' not found (from symbol '", _path, "')" );
    }

    if ( !sym->isFinished() )
        throw Error ( "Could not use symbol '", sym->_path,
                      "' - it has not yet been fully constructed" );

    return sym;
}

Symbol * Symbol::findSymbol ( const String & path )
{
    StringList namePath = path.split ( ".", true );

    for ( size_t i = 0; i < namePath.size(); ++i )
    {
        if ( namePath[ i ].length() == 0 )
            throw Error ( "Name '", path, "' is invalid" );
    }

    Symbol * symFound = 0;
    Symbol * trySymbol = this;

    // Lets try the symbols inside this symbol,
    // and inside the symbols it inherits
    while ( trySymbol != 0 )
    {
        symFound = tryPath ( trySymbol, namePath );

        if ( symFound != 0 )
            return symFound;

        trySymbol = trySymbol->_inheritance;
    }

    trySymbol = _parent;

    // We go up the naming tree and try to match the name path
    while ( trySymbol != 0 )
    {
        symFound = tryPath ( trySymbol, namePath );

        if ( symFound != 0 )
            return symFound;

        trySymbol = trySymbol->_parent;
    }

    return 0;
}

Element * Symbol::getEnumDefault()
{
    if ( !isEnum() )
    {
        assert ( false );

        throw Error ( "Called 'getEnumDefault()' in non-enum symbol '", _path, "'" );
    }

    for ( HashMap<String, Element * >::Iterator elemsIt ( _elements ); elemsIt.isValid(); elemsIt.next() )
    {
        Element * elem = elemsIt.value();

        assert ( elem != 0 );

        if ( elem->isEnumDefault() )
        {
            return elem;
        }
    }

    return 0;
}

Symbol * Symbol::getOldestMessageOrStructParent()
{
    if ( !_parent || !_parent->isMessageOrStruct() )
        return 0;

    Symbol * ret = _parent;

    while ( ret->getParent() != 0 && ret->getParent()->isMessageOrStruct() )
    {
        ret = ret->getParent();
    }

    return ret;
}

Symbol * Symbol::getBaseInheritance()
{
    Symbol * s = this;

    while ( s->getInheritance() != 0 )
    {
        s = s->getInheritance();
    }

    return s;
}

String Symbol::unifiedName ( const String & name )
{
    return name.removeChars ( "_" ).toLower();
}

void Symbol::parseEnumCode ( const String & code, String & name, String & extName )
{
    if ( !code.startsWith ( "\"" ) || !code.endsWith ( "\"" ) )
    {
        name = code;
        extName.clear();
    }
    else
    {
        if ( code.length() < 3 )
        {
            throw Error ( "Invalid enum code name: '", code, "'" );
        }

        name.clear();

        // Code name w/o " "
        extName = code.substr ( 1, code.length() - 2 );

        for ( int i = 0; i < extName.length(); ++i )
        {
            if ( isalnum ( extName[ i ] ) )
            {
                name.append ( extName[ i ] );
            }
            else if ( !name.endsWith ( "_" ) )
            {
                // A sequence of multiple special characters is replaced by exactly one '_'.
                name.append ( "_" );
            }
        }

        if ( name.isEmpty() || name == "_" )
        {
            throw Error ( "Invalid enum code name: '", code, "'" );
        }
    }

    if ( name[ 0 ] != '_' && !isalpha ( name[ 0 ] ) )
    {
        // We don't want names that start with a digit.
        name = String ( "_%1" ).arg ( name );
    }
    else if ( name.find ( '_' ) < 0 )
    {
        // We want enum codes that don't include '_' to always start with a capital letter.
        // It looks better and avoids conflicts with reserved language keywords.
        name = name.capitalize();
    }
}
