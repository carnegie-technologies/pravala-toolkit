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

#include "HtmlGen.hpp"

using namespace Pravala;

String HtmlGenerator::getHelpText()
{
    String text
        = "    A generator that generates an html file with the documentation for the protocol files.\n"
          "    Options:\n"
          "      -f output_file, --output-file=output_file\n"
          "          The output file.\n\n"
          "      --skip-fields-inherited-from=symbols\n"
          "          The documentation will not include fields generated from listed symbols.\n\n";

    text.append ( getBasicHelpText() );

    return text;
}

LanguageGenerator::SetOptResult HtmlGenerator::setOption (
        char shortName, const String & longName, const String & value )
{
    if ( shortName == 'f' || longName == "output-file" )
    {
        if ( value.isEmpty() )
            return OptErrMissingValue;

        _outputFile = value;
    }
    else if ( longName == "skip-fields-inherited-from" )
    {
        if ( value.isEmpty() )
            return OptErrMissingValue;

        _skipInheritedFields.insertAll ( value.split ( " \t," ) );
    }
    else
    {
        return setBasicOption ( shortName, longName, value );
    }

    return OptOkValueConsumed;
}

void HtmlGenerator::run()
{
    if ( _outputFile.isEmpty() )
    {
        throw Error ( "HTML generator requires output file to be specified" );
    }

    if ( !_singleImplFile )
    {
        _singleImplFile = new HtmlFile ( _outputFile );

        addFile ( _singleImplFile );
    }

    assert ( _singleImplFile != 0 );

    // Don't use regular run()

    procSymbol ( _proto.getRoot() );

    writeFile ( _outputFile, _singleImplFile );

    generateFlagFiles();
}

HtmlGenerator::HtmlGenerator ( ProtocolSpec & proto ):
    LanguageGenerator ( proto ), _singleImplFile ( 0 ), _symString ( 0 ), _symIpAddr ( 0 )
{
    _symString = _proto.getRoot()->createBasicRootType ( "string", Symbol::SpecTypeString );
    _symIpAddr = _proto.getRoot()->createBasicRootType ( "ip_addr" );
}

void HtmlGenerator::procRegularSymbol ( Symbol * symbol )
{
    assert ( symbol != 0 );
    assert ( !symbol->isNamespace() );

    // Do we want to generate output?
    if ( !symbol->isGenerated() )
        return;

    if ( symbol->isMessageOrStruct() )
    {
        renderMessageOrStruct ( symbol, *_singleImplFile );
    }
    else if ( symbol->isEnum() )
    {
        renderEnum ( symbol, *_singleImplFile );
    }
    else
    {
        throw Error ( "Unexpected symbol type; Symbol's path: ", symbol->getPath() );
    }

    StringList intSymbols = symbol->getOrdInternalSymbols();

    // This list is in the order those symbols were declared.
    // We want to sort it alphabetically:

    intSymbols.sortAscending();

    for ( size_t i = 0; i < intSymbols.size(); ++i )
    {
        Symbol * const s = symbol->getInternalSymbols().value ( intSymbols[ i ] );

        assert ( s != 0 );

        procSymbol ( s );
    }
}

void HtmlGenerator::procNamespaceSymbol ( Symbol * symbol )
{
    assert ( symbol != 0 );
    assert ( symbol->isNamespace() );

    if ( !symbol->isGenerated() )
        return;

    if ( symbol != _proto.getRoot() )
    {
        // We don't want to render the root namespace.

        renderNamespace ( symbol, *_singleImplFile );
    }

    StringList intSymbols = symbol->getOrdInternalSymbols();

    // This list is in the order those symbols were declared.
    // We want to sort it alphabetically:

    intSymbols.sortAscending();

    // We want to process internal namespaces after all other symbols.
    // So we run the whole thing twice, first (m=0) will generate other symbols,
    // second (m=1) will generate internal namespaces.

    for ( int m = 0; m < 2; ++m )
    {
        for ( size_t i = 0; i < intSymbols.size(); ++i )
        {
            Symbol * const s = symbol->getInternalSymbols().value ( intSymbols[ i ] );

            assert ( s != 0 );

            if ( m == ( s->isNamespace() ? 1 : 0 ) )
            {
                procSymbol ( s );
            }
        }
    }
}

String HtmlGenerator::getSymbolAnchor ( Symbol * s )
{
    String anchor = String ( "" );

    if ( s->isNamespace() )
    {
        return anchor.append ( "namespace-%1" ).arg ( s->getPath() );
    }
    else if ( s->isMessageOrStruct() )
    {
        return anchor.append ( "struct-%1" ).arg ( s->getPath() );
    }
    else if ( s->isEnum() )
    {
        return anchor.append ( "enum-%1" ).arg ( s->getPath() );
    }

    return anchor;
}

String HtmlGenerator::getSymbolName ( Symbol * s )
{
    String name = String ( "" );
    Symbol * parent = s->getParent();

    // Append the containing item, if it's not a namespace
    // This makes it clearer which classes are internal to another class
    if ( parent != 0 && !parent->isNamespace() )
    {
        name.append ( getSymbolName ( s->getParent() ) ).append ( "." );
    }

    name.append ( s->getName() );
    return name;
}

String HtmlGenerator::getSymbolAnchorLink ( Symbol * s )
{
    if ( !s->isGenerated() )
        return String::EmptyString;

    return String ( "#" ).append ( getSymbolAnchor ( s ) );
}

void HtmlGenerator::renderNamespace ( Symbol * s, HtmlFile & impl )
{
    assert ( s != 0 );
    assert ( s->isGenerated() );

    // Add TOC entry
    impl.setTocSection();
    impl.ae ( String ( "<li><a href=\"%1\">%2</a></li>" ).arg ( getSymbolAnchorLink ( s ), s->getPath() ) );

    // Render namespace into body
    impl.setContentSection();
    impl.ae ( String ( "<h2 id=\"%1\">%2</h2>" ).arg ( getSymbolAnchor ( s ), s->getPath() ) );
}

void HtmlGenerator::renderMessageOrStruct ( Symbol * s, HtmlFile & impl )
{
    impl.setContentSection();

    impl.ae ( "<div class='section-bottom'>" );

    renderTypeHeader ( s, impl );

    renderStructFields ( s, impl );

    // Render inheritance chain
    Symbol * inheritance = s->getInheritance();

    while ( inheritance != 0 )
    {
        if ( _skipInheritedFields.contains ( inheritance->getPath() ) )
        {
            // We found a symbol whose fields we do NOT want to include as 'also inherited'.
            // We just mention the fact that it and all symbols past it in inheritance list are inherited.

            StringList paths;
            StringList links;

            do
            {
                paths.append ( inheritance->getPath() );
                links.append ( getSymbolAnchorLink ( inheritance ) );

                inheritance = inheritance->getInheritance();
            }
            while ( inheritance != 0 );

            assert ( paths.size() > 0 );
            assert ( links.size() > 0 );
            assert ( paths.size() == links.size() );

            impl.a ( "<p><em>also inherits from " );

            for ( size_t i = 0; i < paths.size(); ++i )
            {
                if ( i > 0 )
                {
                    if ( i + 1 >= paths.size() )
                    {
                        impl.a ( " and " );
                    }
                    else
                    {
                        impl.a ( ", " );
                    }
                }

                impl.a ( "<strong>" );

                if ( links.at ( i ).isEmpty() )
                {
                    // We don't generate this symbol - no link.
                    impl.a ( paths.at ( i ) );
                }
                else
                {
                    impl.a ( String ( "<a href=\"%1\">%2</a>" ).arg ( links.at ( i ), paths.at ( i ) ) );
                }

                impl.a ( "</strong>" );
            }

            impl.a ( ".</em></p>" );
            break;
        }

        const String link = getSymbolAnchorLink ( inheritance );

        impl.a ( "<p><em>inherited from <strong>" );

        if ( link.isEmpty() )
        {
            // We don't generate this symbol - no link.
            impl.a ( inheritance->getPath() );
        }
        else
        {
            impl.a ( String ( "<a href=\"%1\">%2</a>" ).arg ( link, inheritance->getPath() ) );
        }

        impl.ae ( "</strong>:</em></p>" );

        renderStructFields ( inheritance, impl );

        inheritance = inheritance->getInheritance();
    }

    // End section-bottom
    impl.ae ( "</div>" );
}

void HtmlGenerator::renderEnum ( Symbol * s, HtmlFile & impl )
{
    impl.setContentSection();

    impl.ae ( "<div class='section-bottom'>" );
    renderTypeHeader ( s, impl );

    const StringList & elems = s->getOrdElements();

    if ( elems.size() == 0 )
    {
        impl.ae ( "<p class=\"muted\"><em>No elements.</em></p>" );
    }
    else
    {
        impl.ae ( "<table class=\"table table-condensed table-bordered table-striped\">" );
        impl.ae ( "<thead>" );
        impl.ae ( "<tr>" );
        impl.ae ( "<th style=\"width:150px\" class=\"text-right\">Name</th>" );
        impl.ae ( "<th style=\"width:75px\" class=\"text-center\">Value</th>" );
        impl.ae ( "<th>Description</th>" );
        impl.ae ( "</tr>" );
        impl.ae ( "</thead>" );

        for ( size_t i = 0; i < elems.size(); ++i )
        {
            Element * e = s->getElements().value ( elems[ i ] );

            assert ( e != 0 );

            impl.ae ( "<tr>" );

            // Name
            impl.ae ( String ( "<td class=\"text-right\"><em>%1</em></td>" ).arg ( e->name ) );

            // Value
            impl.ae ( String ( "<td class=\"text-center\">%1</td>" ).arg ( e->value ) );

            // Description

            impl.ae ( String ( "<td>%1</td>" ).arg ( e->comment ) );

            impl.ae ( "</tr>" );
        }

        impl.ae ( "</table>" );
    }

    // End section-bottom
    impl.ae ( "</div>" );
}

void HtmlGenerator::renderUpToContents ( HtmlFile & impl )
{
    impl.a ( "<a class='pull-right' href='#toc'>" );
    impl.a ( "<small><span class='icon-arrow-up toc-arrow' title='up to contents'></span>" );
    impl.a ( "</small></a>" );
}

void HtmlGenerator::renderTypeHeader ( Symbol * s, HtmlFile & impl )
{
    // Render header
    impl.ae ( String ( "<h3 id=\"%1\">%2 <small>%3</small>" )
              .arg ( getSymbolAnchor ( s ), getSymbolName ( s ), s->getPath() ) );
    renderUpToContents ( impl );
    impl.ae ( "</h3>" );

    // Render comment
    if ( !s->getComment().isEmpty() )
    {
        impl.a ( "<p>" ).a ( s->getComment() ).a ( "</p>" ).e();
    }
}

void HtmlGenerator::renderStructFields ( Symbol * s, HtmlFile & impl )
{
    const StringList & elems = s->getOrdElements();

    if ( elems.size() == 0 )
    {
        impl.ae ( "<p class=\"muted\"><em>No fields.</em></p>" );
    }
    else
    {
        impl.ae ( "<table class=\"table table-condensed table-bordered table-striped\">" );
        impl.ae ( "<thead>" );
        impl.ae ( "<tr>" );
        impl.ae ( "<th style=\"width:150px\" class=\"text-right\">Name</th>" );
        impl.ae ( "<th style=\"width:125px\" class=\"text-center\">Type</th>" );
        impl.ae ( "<th style=\"width:75px\" class=\"text-center\">Required</th>" );
        impl.ae ( "<th>Description</th>" );
        impl.ae ( "</tr>" );
        impl.ae ( "</thead>" );

        for ( size_t i = 0; i < elems.size(); ++i )
        {
            Element * e = s->getElements().value ( elems[ i ] );

            assert ( e != 0 );

            // Mute non-public elements
            if ( e->isPrivate() || e->isProtected() )
            {
                impl.ae ( "<tr class=\"muted\">" );
            }
            else
            {
                impl.ae ( "<tr>" );
            }

            // Name
            String code = !e->isAlias() ? String ( "Code: %1" ).arg ( e->code ) : "";
            impl.ae ( String ( "<td class=\"text-right\" title=\"%1\"><em>%2</em></td>" ).arg ( code, e->name ) );

            // Type

            impl.a ( "<td class=\"text-center\">" );
            String repeated = e->isRepeated() ? "[]" : "";

            if ( e->isSalias() )
            {
                /// @todo TODO Link to concept documentation explaining aliases

                if ( e->getAliasPayloadBitLength() == 1 )
                {
                    impl.a ( "settable alias (1 bit)" );
                }
                else
                {
                    impl.a ( String ( "settable alias (%1 bits)" ).arg ( e->getAliasPayloadBitLength() ) );
                }
            }
            else if ( e->isAlias() )
            {
                /// @todo TODO Link to concept documentation explaining aliases

                if ( e->getAliasPayloadBitLength() == 1 )
                {
                    impl.a ( "alias (1 bit)" );
                }
                else
                {
                    impl.a ( String ( "alias (%1 bits)" ).arg ( e->getAliasPayloadBitLength() ) );
                }
            }
            else if ( !e->typeSymbol->isBasic() )
            {
                const String link = getSymbolAnchorLink ( e->typeSymbol );

                if ( link.isEmpty() )
                {
                    impl.a ( getSymbolName ( e->typeSymbol ) );
                    impl.a ( repeated );
                }
                else
                {
                    impl.a ( String ( "<a href=\"%1\">%2%3</a>" )
                             .arg ( link, getSymbolName ( e->typeSymbol ), repeated ) );
                }
            }
            else
            {
                impl.a ( getSymbolName ( e->typeSymbol ) ).a ( repeated );
            }

            impl.ae ( "</td>" );

            // Required/Optional
            impl.ae ( String ( "<td class=\"text-center\">%1</td>" )
                      .arg ( e->isRequired() ? "required" : "optional" ) );

            // Description
            impl.a ( "<td>" );

            if ( !e->comment.isEmpty() )
            {
                impl.a ( String ( "%1<br/>" ).arg ( e->comment ) );
            }

            for ( HashMap<String, String>::Iterator it ( e->options ); it.isValid(); it.next() )
            {
                impl.a ( String ( "<span class=\"bound-pad\"><strong>%1:</strong> %2</span>" )
                         .arg ( it.key(), it.value() ) );
            }

            impl.ae ( "</td>" );

            impl.ae ( "</tr>" );
        }

        impl.ae ( "</table>" );
    }
}
