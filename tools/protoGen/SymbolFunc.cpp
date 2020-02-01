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

bool Symbol::procTypedefEnd ( Symbol & self, const String & token )
{
    if ( token == ";" )
    {
        self.checkCompleteSymbol();
        self._isFinished = true;

        // There will be an error if we call this again!
        self._procFunc = 0;
        return true;
    }

    throw Error ( "Unexpected token: '", token, "'. Expected: ';'" );
}

bool Symbol::procModifierOrTypeOrSymbolEnd ( Symbol & self, const String & token )
{
    if ( token == "}" )
    {
        self.checkCompleteSymbol();
        self._isFinished = true;

        // There will be an error if we call this again!
        self._procFunc = 0;
        return true;
    }

    // Enums don't have types or modifiers - just names and values:
    if ( self.isEnum() )
    {
        self._curElement = new Element ( &self );
        return procEnumCodeName ( self, token );
    }

    SymType sType = getSymType ( token );

    switch ( sType )
    {
        case SymStruct:
            if ( self.isEnum() )
                throw Error ( "Enums cannot contain other symbols" );

            self._procFunc = procIntStructName;
            return false;
            break;

        case SymMessage:
            if ( self.isEnum() )
                throw Error ( "Enums cannot contain other symbols" );

            self._procFunc = procIntMessageName;
            return false;
            break;

        case SymNamespace:
            if ( !self.isNamespace() )
                throw Error ( "Namespaces cannot be declared inside other symbols (except for other namespaces)" );

            self._procFunc = procIntNamespaceName;
            return false;
            break;

        case SymEnum:
            if ( self.isEnum() )
                throw Error ( "Enums cannot contain other symbols" );

            self._procFunc = procIntEnumName;
            return false;
            break;

        case SymTypedef:
            if ( !self.isNamespace() )
                throw Error ( "Typedefs can be declared only inside namespaces" );

            self._procFunc = procIntTypedefName;
            return false;
            break;

        default:
            if ( self.isNamespace() )
                throw Error ( "Expected symbol type, received: '", token, "'" );

            break;
    }

    return procModifierOrElementType ( self, token );
}

void Symbol::createNewIntSymbol ( const String & name, Symbol::SymType symType )
{
    checkName ( _proto, name, true );

    String nPath = _path;

    if ( nPath.length() > 0 )
    {
        nPath.append ( "." );
    }

    nPath.append ( name );

    if ( _internalSymbols.contains ( unifiedName ( name ) ) )
    {
        throw Error ( "Symbol '", nPath, "' already exists" );
    }

    if ( _typedefSymbols.contains ( unifiedName ( name ) ) )
    {
        throw Error ( "Symbol '", nPath, "' already exists (and is a typedef to '",
                      _typedefSymbols[ unifiedName ( name ) ]->_name, "')" );
    }

    if ( unifiedName ( name ) == unifiedName ( _name )
         && ( symType == SymMessage || symType == SymStruct || symType == SymEnum )
         && ( _symType == SymMessage || _symType == SymStruct || _symType == SymEnum ) )
    {
        throw Error ( "Symbol '", nPath, "' has the same name as the symbol in which it is declared" );
    }

    Symbol * nSymbol = new Symbol ( *this, symType, name, nPath, _curComment );

    if ( symType != SymTypedef )
    {
        _internalSymbols[ unifiedName ( name ) ] = nSymbol;
        _ordIntSymbols.append ( unifiedName ( name ) );
    }

    _curIntSymbol = nSymbol;
    _procFunc = procInternalSymbol;
}

bool Symbol::procIntStructName ( Symbol & self, const String & token )
{
    self.createNewIntSymbol ( token, SymStruct );
    return false;
}

bool Symbol::procIntMessageName ( Symbol & self, const String & token )
{
    self.createNewIntSymbol ( token, SymMessage );
    return false;
}

bool Symbol::procIntTypedefName ( Symbol & self, const String & token )
{
    self.createNewIntSymbol ( token, SymTypedef );
    return false;
}

bool Symbol::procIntEnumName ( Symbol & self, const String & token )
{
    self.createNewIntSymbol ( token, SymEnum );
    return false;
}

bool Symbol::procIntNamespaceName ( Symbol & self, const String & token )
{
    if ( self._internalSymbols.contains ( unifiedName ( token ) ) )
    {
        Symbol * nSym = self._internalSymbols[ unifiedName ( token ) ];

        if ( nSym->isNamespace() )
        {
            self._curIntSymbol = nSym;
            self._procFunc = procInternalSymbol;

            // This namespace already exists - we need
            // to "reopen" it - set _isFinished to false,
            // and set the correct _procFunc
            nSym->reopenNamespace();
            return false;
        }
    }

    self.createNewIntSymbol ( token, SymNamespace );
    return false;
}

bool Symbol::procSymInheritBeg ( Symbol & self, const String & token )
{
    if ( token == "(" )
    {
        self._procFunc = procInheritName;
    }
    else
    {
        throw Error ( "Unexpected token '", token, "'. '(' expected." );
    }

    return false;
}

bool Symbol::procSymOpenBracketOrInheritBeg ( Symbol & self, const String & token )
{
    if ( token == "{" )
    {
        self._procFunc = procModifierOrTypeOrSymbolEnd;
    }
    else if ( token == "(" )
    {
        self._procFunc = procInheritName;
    }
    else
    {
        throw Error ( "Unexpected token '", token, "'. '{' or '(' expected." );
    }

    return false;
}

bool Symbol::procInheritName ( Symbol & self, const String & token )
{
    if ( !self.isMessageOrStruct() && !self.isTypedef() )
        throw Error ( "Error defining '", self._name, "' - only messages, structs and typedefs can inherit" );

    assert ( self._parent != 0 );

    Symbol * inhSym = self._parent->findUsableSymbol ( token );

    if ( self.isTypedef() )
    {
        if ( !inhSym )
            throw Error ( "Error type-defining '", self._name,
                          "' - could not find defined symbol '", token, "'" );

        if ( !inhSym->isBasic() )
            throw Error ( "Error type-defining '", self._name,
                          "' (to '", token, "') - only basic types can be the target of typedefs" );
    }
    else
    {
        if ( !inhSym )
            throw Error ( "Error defining '", self._name,
                          "' - could not find inherited parent '", token, "'" );

        if ( self._symType != inhSym->_symType )
            throw Error ( "Error defining '", self._name, "' - inherited parent '", self._path,
                          "' is has a different type (message vs struct mismatch)." );
    }

    self._inheritance = inhSym;
    self._procFunc = procInheritEnd;
    return false;
}

bool Symbol::procInheritEnd ( Symbol & self, const String & token )
{
    if ( self.isTypedef() )
    {
        return self.procExpSymbol ( token, ")", procTypedefEnd );
    }

    return self.procExpSymbol ( token, ")", procSymOpenBracket );
}

bool Symbol::procSymOpenBracket ( Symbol & self, const String & token )
{
    return self.procExpSymbol ( token, "{", procModifierOrTypeOrSymbolEnd );
}

bool Symbol::procModifierOrElementType ( Symbol & self, const String & token )
{
    assert ( !self.isEnum() );
    assert ( !self.isNamespace() );
    assert ( self.isMessageOrStruct() );

    // We want to call this function in the future as well
    // (instead of the currently set) - once we have the first modifier,
    // we don't accept symbol types or symbol end anymore.
    self._procFunc = procModifierOrElementType;

    if ( !self._curElement )
        self._curElement = new Element ( &self );

    Element::ElemRule eRule = getElemRule ( token );

    if ( eRule != Element::RuleUnknown )
    {
        if ( self._curElement->rule == Element::RuleUnknown )
        {
            self._curElement->rule = eRule;
            return false;
        }

        throw Error ( "Multiple rule modifiers specified" );
    }

    Element::ElemAccess eAccess = getElemAccess ( token );

    if ( eAccess != Element::AccessUnknown )
    {
        if ( self._curElement->access == Element::AccessUnknown )
        {
            self._curElement->access = eAccess;
            return false;
        }

        throw Error ( "Multiple access mode modifiers specified" );
    }

    if ( token == ProtocolSpec::KW_UNIQ )
    {
        if ( self._curElement->isUnique )
            throw Error ( "Multiple unique modifiers specified" );

        self._curElement->isUnique = true;
        return false;
    }

    if ( token == ProtocolSpec::KW_DEFINED )
    {
        self._curElement->typeName = token;
        self._procFunc = procDefinedName;
        return false;
    }

    // Below it will either be a type or an error
    self._procFunc = procElementName;

    if ( token == ProtocolSpec::KW_ALIAS || token == ProtocolSpec::KW_SALIAS )
    {
        self._curElement->typeName = token;
        return false;
    }

    Symbol * dataType = self.findUsableSymbol ( token );

    if ( !dataType )
        throw Error ( "Unknown data type '", token, "'" );

    if ( dataType->isMessage() && !dataType->isBaseMessage() )
    {
        assert ( dataType != dataType->getBaseInheritance() );

        throw Error ( "Messages can only be stored using their base type; In this case '",
                      dataType->getBaseInheritance()->getPath(),
                      "' should be used instead of '", dataType->getPath(), "'" );
    }

    self._curElement->typeSymbol = dataType;
    self._curElement->typeName = dataType->_path;

    return false;
}

bool Symbol::procEnumCodeName ( Symbol & self, const String & token )
{
    String name;
    String extName;

    parseEnumCode ( token, name, extName );

    // We treat enum elements as 'type names':
    checkName ( self._proto, name, true );

    if ( self._elements.contains ( unifiedName ( name ) ) )
        throw Error ( "Symbol '", self._path, "' already contains '", name, "' code" );

    self._curElement->name = name;
    self._curElement->extName = extName;
    self._curElement->comment = self._curComment;
    self._procFunc = procEnumEq;

    return false;
}

bool Symbol::procElementName ( Symbol & self, const String & token )
{
    checkName ( self._proto, token, false );

    Symbol * sym = &self;

    // TODO: should we check the entire naming branch for collisions?

    while ( sym != 0 )
    {
        if ( sym->_elements.contains ( unifiedName ( token ) ) )
            throw Error ( "Symbol '", self._path, "' contains element '", token,
                          "' that already is used by the same or inherited symbol '",
                          sym->_path, "'" );

        sym = sym->_inheritance;
    }

    self._curElement->name = token;
    self._curElement->comment = self._curComment;
    self._procFunc = procElementCol;

    return false;
}

bool Symbol::procDefinedName ( Symbol & self, const String & token )
{
    checkName ( self._proto, token, false );

    Symbol * sym = &self;
    Element * elem = 0;
    String nameFoundIn;

    while ( sym != 0 )
    {
        if ( sym->_defines.contains ( unifiedName ( token ) ) )
            throw Error ( "Could not define '", token, "' in '", self._path,
                          "' - it was already defined in symbol '", sym->_path, "'" );

        if ( sym->_elements.contains ( unifiedName ( token ) ) )
        {
            if ( sym == &self )
                throw Error ( "Defining a local value ( ", token,
                              " ) does not make sense" );

            assert ( !elem );
            elem = sym->_elements[ unifiedName ( token ) ];
            nameFoundIn = sym->_path;
        }

        sym = sym->_inheritance;
    }

    if ( !elem )
        throw Error ( "Could not find element to define: '", token, "'" );

    if ( elem->isPrivate() )
        throw Error ( "Could not define element '", token, "' from symbol '",
                      nameFoundIn, "' - it was declared 'private'" );

    if ( !elem->isProtected() )
        throw Error ( "Could not define element '", token, "' from symbol '",
                      nameFoundIn, "' - only 'protected' elements can be defined" );

    if ( elem->isRepeated() )
        throw Error ( "Could not define element '", token, "' from symbol '",
                      nameFoundIn, "' - it is declared as 'repeated'" );

    if ( elem->lastAliasedIn.length() > 0 )
        throw Error ( "Could not define element '", token, "' from symbol '",
                      nameFoundIn, "' - it is aliased (in symbol '", elem->lastAliasedIn,
                      "'); aliased elements cannot be defined" );

    self._curElement->name = token;
    self._curElement->comment = self._curComment;
    self._curElement->definedTarget = elem;
    self._procFunc = procDefinedEqOrAsIn;

    return false;
}

bool Symbol::procEnumEq ( Symbol & self, const String & token )
{
    return self.procExpSymbol ( token, "=", procEnumValue );
}

bool Symbol::procElementCol ( Symbol & self, const String & token )
{
    if ( self._curElement->isAlias() )
    {
        return self.procExpSymbol ( token, ":", procAliasTarget );
    }

    return self.procExpSymbol ( token, ":", procElementCode );
}

bool Symbol::procDefinedEqOrAsIn ( Symbol & self, const String & token )
{
    if ( token == "=" )
    {
        self._procFunc = procDefinedValue;
    }
    else if ( token == ProtocolSpec::KW_DEFINED_AS_IN )
    {
        self._procFunc = procDefinedAsInTarget;
    }
    else
    {
        throw Error ( "Unexpected token '", token,
                      "'. '=' or '", ProtocolSpec::KW_DEFINED_AS_IN,
                      "' expected." );
    }

    return false;
}

bool Symbol::procEnumNegValue ( Symbol & self, const String & token )
{
    return setEnumValue ( self, String ( "-" ).append ( token ) );
}

bool Symbol::procEnumValue ( Symbol & self, const String & token )
{
    // I just thought about it ;)
    assert ( !self._inheritance );

    if ( token == "-" )
    {
        self._procFunc = procEnumNegValue;
        return false;
    }

    return setEnumValue ( self, token );
}

bool Symbol::setEnumValue ( Symbol & self, const String & token )
{
    for ( HashMap<String, Element *>::Iterator it ( self._elements ); it.isValid(); it.next() )
    {
        assert ( it.value() != 0 );

        if ( it.value()->value == token )
        {
            throw Error ( "Element '", self._curElement->name, "' in enum '", self._name,
                          "' has value '", token, "' that already is used by the element '",
                          it.key(), "'" );
        }

        if ( it.value()->value.toInt64() == token.toInt64() )
        {
            throw Error ( "Element '", self._curElement->name, "' in enum '", self._name,
                          "' has value '", token, "' that already is used by the element '",
                          it.key(), "'" );
        }
    }

    self._curElement->value = token;
    self._procFunc = procElementSemiColOrOptBeg;
    return false;
}

bool Symbol::procElementCode ( Symbol & self, const String & token )
{
    if ( token == "-" )
        throw Error ( "Element codes can not have negative values" );

    bool ok = false;
    uint32_t uCode = token.toUInt32 ( &ok );

    if ( !ok || uCode < 1 )
    {
        throw Error ( "Element '", self._curElement->name, "' in symbol '", self._path,
                      "' has incorrect code '", token, "'. Codes should be uint32 values greater than 0." );
    }

    Symbol * sym = &self;

    if ( self._proto.getIdScope() == ProtocolSpec::IdScopeBranch )
    {
        while ( sym != 0 )
        {
            for ( HashMap<String, Element *>::Iterator it ( sym->_elements ); it.isValid(); it.next() )
            {
                if ( it.value()->code == uCode )
                {
                    throw Error ( "Element '", self._curElement->name, "' in symbol '", self._path,
                                  "' has code '", token, "' that is already used by element '",
                                  it.key(), "' in '", sym->_path, "'" );
                }
            }

            sym = sym->_inheritance;
        }
    }
    else if ( self._proto.getIdScope() == ProtocolSpec::IdScopeTree )
    {
        assert ( sym != 0 );

        while ( sym->_inheritance != 0 )
        {
            sym = sym->_inheritance;
        }

        assert ( sym != 0 );
        assert ( !sym->_inheritance );

        if ( sym->_treeRootFieldCodes.contains ( uCode ) )
        {
            throw Error ( "Element '", self._curElement->name, "' in symbol '", self._path,
                          "' has code '", token, "' that already is used by the element '",
                          sym->_treeRootFieldCodes[ uCode ], "' which is a part of the same inheritance tree. "
                          "To change (relax) this restriction, use --id-scope=branch option in the command line" );
        }

        sym->_treeRootFieldCodes[ uCode ] = self._path;
    }
    else if ( self._proto.getIdScope() == ProtocolSpec::IdScopeGlobal )
    {
        assert ( sym != 0 );

        if ( sym->_proto.globalFieldCodes.contains ( uCode ) )
        {
            throw Error ( "Element '", self._curElement->name, "' in symbol '", self._path,
                          "' has code '", token, "' that already is used by the element '",
                          sym->_proto.globalFieldCodes[ uCode ], "'. Field codes have to be globally unique. "
                          "To change (relax) this restriction, use --id-scope=branch, "
                          "or --id-scope=tree option in the command line" );
        }

        sym->_proto.globalFieldCodes[ uCode ] = self._path;
    }
    else
    {
        throw Error ( "Unknown field ID scope: ", String::number ( self._proto.getIdScope() ) );
    }

    self._curElement->code = uCode;
    self._procFunc = procElementSemiColOrOptBeg;
    return false;
}

bool Symbol::procAliasTarget ( Symbol & self, const String & token )
{
    checkName ( self._proto, token, false );

    Symbol * sym = &self;

    while ( sym != 0 )
    {
        if ( sym->_elements.contains ( unifiedName ( token ) ) )
        {
            Element * elem = sym->_elements[ unifiedName ( token ) ];
            Symbol * elType = elem->typeSymbol;

            assert ( elType != 0 );

            if ( !elType->canBeAliased() || elType->getBitLength() < 1 )
                throw Error ( "Error creating an alias '", self._curElement->name,
                              "' - could not declare an alias to data type '",
                              elem->typeName, "'" );

            if ( sym != &self && elem->isPrivate() )
                throw Error ( "Could not create an alias '", self._curElement->name,
                              "' to non-local element declared as private" );

            if ( elem->isRepeated() )
                throw Error ( "Could not create an alias '", self._curElement->name,
                              "' to a data field declared as repeated" );

            if ( self._curElement->isPrivate()
                 && !elem->isPrivate() )
            {
                throw Error ( "Could not create a 'private' alias '", self._curElement->name,
                              "' to a data field not declared as 'private'" );
            }
            else if ( self._curElement->isProtected()
                      && !elem->isProtected()
                      && !elem->isPrivate() )
            {
                throw Error ( "Could not create a 'protected' alias '", self._curElement->name,
                              "' to a data field not declared as 'private' or 'protected'" );
            }

            String optName;

            if ( elem->options.contains ( ProtocolSpec::KW_DEFAULT ) )
            {
                optName = ProtocolSpec::KW_DEFAULT;
            }
            else if ( elem->options.contains ( ProtocolSpec::KW_MIN ) )
            {
                optName = ProtocolSpec::KW_MIN;
            }
            else if ( elem->options.contains ( ProtocolSpec::KW_MAX ) )
            {
                optName = ProtocolSpec::KW_MAX;
            }
            else if ( elem->options.contains ( ProtocolSpec::KW_MIN_LENGTH ) )
            {
                optName = ProtocolSpec::KW_MIN_LENGTH;
            }
            else if ( elem->options.contains ( ProtocolSpec::KW_MAX_LENGTH ) )
            {
                optName = ProtocolSpec::KW_MAX_LENGTH;
            }

            if ( optName.length() > 0 )
            {
                throw Error ( "An element that has option '", optName, "' set cannot be aliased" );
            }

            if ( elem->defValues.size() > 0 )
            {
                String definedIn = "unknown";

                const HashMap<String, String>::Iterator it ( elem->defValues );

                if ( it.isValid() )
                    definedIn = it.value();

                throw Error ( "An element that is defined (in symbol '", definedIn, "') cannot be aliased" );
            }

            elem->lastAliasedIn = self._name;

            self._curElement->aliasTargetBitLength = elType->getBitLength();
            self._curElement->aliasTarget = elem;
            self._procFunc = procAliasRangeBeg;
            return false;
        }

        sym = sym->_inheritance;
    }

    throw Error ( "Could not find target '", token, "' of the alias '",
                  self._curElement->name, "'" );
}

bool Symbol::procAliasRangeBeg ( Symbol & self, const String & token )
{
    return self.procExpSymbol ( token, "[", procAliasRangeFrom );
}

bool Symbol::procAliasRangeFrom ( Symbol & self, const String & token )
{
    if ( token == "-" )
        throw Error ( "Alias range values can not be negative numbers" );

    for ( int i = 0; i < token.length(); ++i )
    {
        if ( !isdigit ( token[ i ] ) )
            throw Error ( "Beginning of alias target's range is not a number: '",
                          token, "'" );
    }

    self._curElement->strAliasRangeFrom = token;
    self._procFunc = procAliasRangeEndOrDash;
    return false;
}

bool Symbol::procAliasRangeEndOrDash ( Symbol & self, const String & token )
{
    if ( token == "-" )
    {
        self._procFunc = procAliasRangeTo;
    }
    else if ( token == "]" )
    {
        self._curElement->strAliasRangeTo = self._curElement->strAliasRangeFrom;

        return procAliasRangeEnd ( self, token );
    }
    else
    {
        throw Error ( "Unexpected token '", token, "'. '-' or ']' expected." );
    }

    return false;
}

bool Symbol::procAliasRangeTo ( Symbol & self, const String & token )
{
    if ( token == "-" )
        throw Error ( "Alias range values can not be negative numbers" );

    for ( int i = 0; i < token.length(); ++i )
    {
        if ( !isdigit ( token[ i ] ) )
            throw Error ( "End of alias target's range is not a number: '",
                          token, "'" );
    }

    self._curElement->strAliasRangeTo = token;
    self._procFunc = procAliasRangeEnd;
    return false;
}

bool Symbol::procAliasRangeEnd ( Symbol & self, const String & token )
{
    return self.procExpSymbol ( token, "]", procElementSemiColOrOptBeg );
}

bool Symbol::procDefinedNegValue ( Symbol & self, const String & token )
{
    return setDefinedValue ( self, String ( "-" ).append ( token ) );
}

bool Symbol::procDefinedValue ( Symbol & self, const String & token )
{
    if ( token == "-" )
    {
        self._procFunc = procDefinedNegValue;
        return false;
    }

    return setDefinedValue ( self, token );
}

void Symbol::checkSetDefinedValue (
        const String & symbolName,
        Element * element,
        const String & defValue )
{
    assert ( element != 0 );

    // checkAssValue will throw an error if the value is, for some reason, incorrect.
    String assValue = element->checkAssValue ( defValue );

    if ( element->isUnique )
    {
        if ( element->defValues.contains ( assValue ) )
        {
            throw Error ( "Element '", element->name, "' declared as '", ProtocolSpec::KW_UNIQ,
                          "' already has a value '", defValue, "' defined in symbol '",
                          element->defValues[ assValue ], "'" );
        }

        // If the 'default' option is not used, the 'default' is 0
        String elemOwnValue = "0";
        bool hasDefaultOpt = false;

        if ( element->options.contains ( ProtocolSpec::KW_DEFAULT ) )
        {
            hasDefaultOpt = true;
            elemOwnValue = element->options.value ( ProtocolSpec::KW_DEFAULT );
        }
        else if ( element->typeSymbol != 0 && element->typeSymbol->isEnum() )
        {
            Element * defEnumElem = element->typeSymbol->getEnumDefault();

            assert ( defEnumElem != 0 );

            if ( !defEnumElem )
                throw Error ( "Enum '", element->typeSymbol->getPath(), "' has no default element" );

            elemOwnValue = defEnumElem->name;
        }

        elemOwnValue = element->checkAssValue ( elemOwnValue );

        if ( elemOwnValue == assValue )
        {
            throw Error ( "Element '", element->name, "' declared as '", ProtocolSpec::KW_UNIQ,
                          "' has a default value '", elemOwnValue, "' - it cannot be 'defined' using this value" );
        }

        if ( !hasDefaultOpt && element->typeSymbol != 0 && element->typeSymbol->_specType == SpecTypeString )
        {
            elemOwnValue = element->checkAssValue ( "\"\"" );

            if ( elemOwnValue == assValue )
            {
                throw Error ( "Element '", element->name, "' declared as '", ProtocolSpec::KW_UNIQ,
                              "' has a default value '", elemOwnValue, "' - it cannot be 'defined' using this value" );
            }
        }
    }

    element->defValues[ assValue ] = symbolName;
}

bool Symbol::setDefinedValue ( Symbol & self, const String & token )
{
    self._curElement->value = token;
    self._procFunc = procElementSemiCol;

    checkSetDefinedValue ( self._name, self._curElement->definedTarget, token );

    return false;
}

bool Symbol::procDefinedAsInTarget ( Symbol & self, const String & token )
{
    Symbol * sym = self.findUsableSymbol ( token );

    assert ( sym != 0 );

    if ( !sym->isMessageOrStruct() )
        throw Error ( "Could not use '", self._path, "' as 'as in' target. It is not a message" );

    Element * asInElement = 0;

    if ( !sym->_defines.find ( unifiedName ( self._curElement->name ), asInElement )
         || !asInElement
         || !asInElement->definedTarget )
    {
        throw Error ( "Error defining '", self._curElement->name,
                      "' - the 'as in' target '", sym->_path,
                      "' does not define the same field name" );
    }

    assert ( self._curElement->definedTarget != 0 );

    bool mismatchedTypes = false;

    if ( asInElement->definedTarget->typeSymbol != self._curElement->definedTarget->typeSymbol )
    {
        // Types don't match, which means that:
        // - they use two different symbols
        // - one of them is a symbol and the other is an alias

        mismatchedTypes = true;
    }
    else if ( !asInElement->definedTarget->typeSymbol && !self._curElement->definedTarget->typeSymbol )
    {
        // They don't use symbols (they have to be aliases).

        if ( !asInElement->definedTarget->aliasTarget
             || !self._curElement->definedTarget->aliasTarget
             || asInElement->definedTarget->getAliasPayloadBitLength()
             != self._curElement->definedTarget->getAliasPayloadBitLength() )
        {
            // One of them is an alias, the other - not,
            // or they are both aliases with different bit lengths

            mismatchedTypes = true;
        }
    }

    if ( mismatchedTypes )
    {
        throw Error ( "Error defining '", self._curElement->name,
                      "' - the 'as in' target '", sym->_path,
                      "' defines the same field name, but with a different (and incompatible) type" );
    }

    self._curElement->value = sym->_defines[ unifiedName ( self._curElement->name ) ]->value;
    self._procFunc = procElementSemiCol;
    return false;
}

bool Symbol::procElementSemiColOrOptBeg ( Symbol & self, const String & token )
{
    if ( token == "[" )
    {
        self._procFunc = procElementOptName;
    }
    else if ( token == ";" )
    {
        return procElementSemiCol ( self, token );
    }
    else
    {
        throw Error ( "Unexpected token '", token, "'. '[' or ';' expected." );
    }

    return false;
}

bool Symbol::procElementOptName ( Symbol & self, const String & token )
{
    if ( self._curElement->options.contains ( token ) )
        throw Error ( "Option '", token, "' is already declared (with value '",
                      self._curElement->options[ token ], "')" );

    if ( self.isEnum() )
    {
        if ( token == ProtocolSpec::KW_DEFAULT )
        {
            // Let's make sure no other element of this enum is declared as 'default':
            Element * existingDefault = self.getEnumDefault();

            if ( existingDefault != 0 )
                throw Error ( "Only one enum's element can be declared as '", ProtocolSpec::KW_DEFAULT,
                              "'. '", existingDefault->name, "' is already declared as '",
                              existingDefault->options.value ( ProtocolSpec::KW_DEFAULT ), "'" );

            assert ( self._curElement != 0 );

            // This is a "special" option. In the .proto file we use 2 different option names:
            // default and invalid_default. However, in the internal map of options
            // we use 'default' as the name of the option, and either 'default' or 'invalid_default'
            // as its value. This is to simplify processing - invalid_default is also a 'default' option,
            // even though sometimes we need to check which one it is!
            self._curElement->options[ ProtocolSpec::KW_DEFAULT ] = token;
        }
        else
        {
            throw Error ( "Option '", token, "' is not valid for enum elements (only '",
                          ProtocolSpec::KW_DEFAULT, "' option is allowed)" );
        }

        // So far we only have one possibility that makes sense, but in case we add more options later
        // we will accept a ',' as well:
        self._procFunc = self.procElementOptComOrEnd;
        return false;
    }

    if ( !self._proto.validOptions.contains ( token ) )
        throw Error ( "Unknown option '", token, "'" );

    if ( token == ProtocolSpec::KW_DEFAULT )
    {
        if ( !self._curElement->isOptional() )
            throw Error ( "Option '", token, "' only makes sense for 'optional' elements" );

        if ( self._curElement->isRepeated() )
            throw Error ( "Option '", token, "' does not make sense for 'repeated' elements" );
    }

    if ( token == ProtocolSpec::KW_MIN_LENGTH || token == ProtocolSpec::KW_MAX_LENGTH )
    {
        if ( !self._curElement->typeSymbol || self._curElement->typeSymbol->_specType != SpecTypeString )
            throw Error ( "Option '", token, "' only makes sense for string types" );
    }

    if ( token == ProtocolSpec::KW_MIN || token == ProtocolSpec::KW_MAX )
    {
        if ( !self._curElement->typeSymbol->isInteger() && !self._curElement->typeSymbol->isFloatingPoint() )
        {
            throw Error ( "Option '", token, "' only makes sense for numeric types" );
        }
    }

    self._curElement->curOption = token;
    self._procFunc = self.procElementOptEq;
    return false;
}

bool Symbol::procElementOptEq ( Symbol & self, const String & token )
{
    return self.procExpSymbol ( token, "=", procElementOptValue );
}

bool Symbol::procElementOptNegValue ( Symbol & self, const String & token )
{
    return setElementOptValue ( self, String ( "-" ).append ( token ) );
}

bool Symbol::procElementOptValue ( Symbol & self, const String & token )
{
    if ( token == "-" )
    {
        self._procFunc = procElementOptNegValue;
        return false;
    }

    return setElementOptValue ( self, token );
}

bool Symbol::setElementOptValue ( Symbol & self, const String & token )
{
    Element * elem = self._curElement;

    assert ( elem != 0 );

    const String & optName = elem->curOption;

    if ( optName == ProtocolSpec::KW_DEFAULT )
    {
        elem->options[ elem->curOption ] = elem->checkAssValue ( token );
    }
    else if ( optName == ProtocolSpec::KW_MIN || optName == ProtocolSpec::KW_MAX )
    {
        if ( ( elem->typeSymbol != 0 && ( elem->typeSymbol->isInteger() || elem->typeSymbol->isFloatingPoint() ) )
             || ( elem->aliasTarget != 0 && elem->getAliasPayloadBitLength() > 0 ) )
        {
            /// @todo We could check whether min/max values are legal ( min <= max),
            /// but this would depend on the actual type - signed/unsigned.
            /// This is not critical, so is left out for now.
            elem->options[ elem->curOption ] = elem->checkAssValue ( token );
        }
        else
        {
            throw Error ( "Option '", optName, "' can only be used for numeric elements" );
        }
    }
    else if ( optName == ProtocolSpec::KW_MIN_LENGTH || optName == ProtocolSpec::KW_MAX_LENGTH
              || optName == ProtocolSpec::KW_MIN_LIST_SIZE || optName == ProtocolSpec::KW_MAX_LIST_SIZE )
    {
        if ( optName == ProtocolSpec::KW_MIN_LENGTH || optName == ProtocolSpec::KW_MAX_LENGTH )
        {
            if ( !elem->typeSymbol || elem->typeSymbol->_specType != SpecTypeString )
                throw Error ( "Option '", optName, "' can only be used for string elements" );
        }
        else if ( optName == ProtocolSpec::KW_MIN_LIST_SIZE || optName == ProtocolSpec::KW_MAX_LIST_SIZE )
        {
            if ( !elem->isRepeated() )
                throw Error ( "Option '", optName, "' can only be used for 'repeated' elements" );
        }

        bool ok = false;
        int val = token.toInt32 ( &ok );

        if ( !ok || val < 1 )
            throw Error ( "Option '", optName, "' can only have positive integral values" );

        String oValName;
        bool exstSmaller = false;

        // In two following 'if' conditions we actually want to SET exstSmaller to true (to save space!)
        if ( optName == ProtocolSpec::KW_MAX_LENGTH && ( exstSmaller = true ) )
        {
            oValName = ProtocolSpec::KW_MIN_LENGTH;
        }
        else if ( optName == ProtocolSpec::KW_MAX_LIST_SIZE && ( exstSmaller = true ) )
        {
            oValName = ProtocolSpec::KW_MIN_LIST_SIZE;
        }
        else if ( optName == ProtocolSpec::KW_MIN_LENGTH )
        {
            oValName = ProtocolSpec::KW_MAX_LENGTH;
        }
        else if ( optName == ProtocolSpec::KW_MIN_LIST_SIZE )
        {
            oValName = ProtocolSpec::KW_MAX_LIST_SIZE;
        }

        if ( exstSmaller && elem->options.contains ( oValName ) )
        {
            int oVal = elem->options[ oValName ].toInt32 ( &ok );

            assert ( ok );

            if ( oVal > val )
                throw Error ( "The value of '", optName, "' option cannot be lesser than the value of '",
                              oValName, "' option" );
        }
        else if ( !exstSmaller && elem->options.contains ( oValName ) )
        {
            int oVal = elem->options[ oValName ].toInt32 ( &ok );

            assert ( ok );

            if ( oVal < val )
                throw Error ( "The value of '", optName, "' option cannot be greater than the value of '",
                              oValName, "' option" );
        }

        elem->options[ elem->curOption ] = token;
    }
    else
    {
        elem->options[ elem->curOption ] = token;
    }

    elem->curOption.clear();
    self._procFunc = procElementOptComOrEnd;

    return false;
}

bool Symbol::procElementOptComOrEnd ( Symbol & self, const String & token )
{
    if ( token == "," )
    {
        self._procFunc = procElementOptName;
    }
    else if ( token == "]" )
    {
        self._procFunc = procElementSemiCol;
    }
    else
    {
        throw Error ( "Unexpected token '", token, "'. ',' or ']' expected." );
    }

    return false;
}

bool Symbol::procElementSemiCol ( Symbol & self, const String & token )
{
    if ( token != ";" )
        throw Error ( "Unexpected token: '", token, "'. Expected ';'" );

    if ( self._curElement->typeName == ProtocolSpec::KW_DEFINED )
    {
        self.checkCurDefined();

        assert ( !self._defines.contains ( unifiedName ( self._curElement->name ) ) );

        self._defines[ unifiedName ( self._curElement->name ) ] = self._curElement;
    }
    else
    {
        if ( self._curElement->isAlias() )
        {
            self.checkCurAlias();
        }
        else
        {
            self.checkCurBasic();
        }

        assert ( !self._elements.contains ( unifiedName ( self._curElement->name ) ) );

        self._elements[ unifiedName ( self._curElement->name ) ] = self._curElement;
        self._ordElements.append ( unifiedName ( self._curElement->name ) );

        assert ( self._elements.size() == self._ordElements.size() );
    }

    self._curElement = 0;
    self._procFunc = procModifierOrTypeOrSymbolEnd;

    return false;
}
