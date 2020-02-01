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

#include <ctime>

#include "HtmlFile.hpp"

using namespace Pravala;

// Template from AceJS/service/apidoc/templates/index.hbs

HtmlFile::HtmlFile ( const String & path ):
    FileObject ( path, "" ),
    _secToc ( -1 ),
    _secContent ( -1 )
{
    const time_t now = time ( 0 );
    const char * nowStr = ctime ( &now );

    appendSection();

    // HEADER

    ae ( "<!DOCTYPE html>" );
    ae ( "<html>" );
    ae ( "<head>" );

    ae ( "<meta charset=\"utf-8\">" );
    ae ( "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">" );
    ae ( "<title>Pravala Protocol Documentation</title>" );
    ae ( "<meta name=\"viewport\" content=\"width=device-width\">" );

    ae ( "<link href=\"http://netdna.bootstrapcdn.com/twitter-bootstrap/2.3.1/css/"
         "bootstrap-combined.no-icons.min.css\" rel=\"stylesheet\">" );
    ae ( "<link href=\"http://netdna.bootstrapcdn.com/font-awesome/3.1.0/css/"
         "font-awesome.min.css\" rel=\"stylesheet\">" );

    ae ( "<style>" );
    ae ( ".section-top { padding-top: 20px; }" );
    ae ( ".section-bottom { padding-bottom: 20px; }" );
    ae ( ".bound-pad { padding-right: 3em; }" );
    ae ( ".toc-arrow { color: #999; opacity: 0.2; }" );
    ae ( ".text-center { text-align: center !important; }" );
    ae ( ".text-right { text-align: right !important; }" );
    ae ( "</style>" );

    ae ( "</head>" );
    ae ( "<body data-spy=\"scroll\" data-target=\".navbar\">" );

    // NAVBAR

    ae ( "<div class=\"navbar navbar-inverse\">" );
    ae ( "<div class=\"navbar-inner\">" );
    ae ( "<div class=\"container\">" );
    ae ( "<a class=\"btn btn-navbar\" data-toggle=\"collapse\" data-target=\".nav-collapse\">" );
    ae ( "<span class=\"icon-bar\"></span>" );
    ae ( "<span class=\"icon-bar\"></span>" );
    ae ( "<span class=\"icon-bar\"></span>" );
    ae ( "</a>" );
    ae ( "<a class=\"brand\" href=\"#\">Protocol</a>" );
    ae ( "<div class=\"nav-collapse collapse\">" );
    ae ( "<ul class=\"nav\">" );
    ae ( "<li><a href=\"#toc\">Contents</a></li>" );
    ae ( "<li><a href=\"#symbols\">Symbols</a></li>" );
    ae ( "</ul>" );
    ae ( "</div>" );
    ae ( "</div>" );
    ae ( "</div>" );
    ae ( "</div>" );

    // INTRO

    // Set up containers (closed by footer)
    ae ( "<div class=\"container\" style=\"margin-top: 40px\">" );
    ae ( "<div class=\"row\">" );

    // Intro material
    ae ( "<div class=\"page-header\">" );

    ae ( String ( "<h1>Protocol Documentation <small>%1</small></h1>" ).arg ( nowStr ) );
    ae ( "</div>" );

    a ( "<p>" );
    a ( "The Pravala Protocol is a binary-encoded network-safe protocol for communicating between components, " );
    a ( "including MasClient and MAS, Android Java UI and AcePlus, AceClient and Liberty UI, and many more." );
    ae ( "</p>" );

    a ( "<p>" );
    a ( "This protocol is defined using <code>.proto</code> files which describe the known messages and types. " );
    a ( "Pravala's generator library then turns those definitions into classes, en/decoders, and this documentation." );
    ae ( "</p>" );

    // TOC header

    ae ( "<h3 id=\"toc\">Contents</h3>" );
    ae ( "<ul>" );

    // TOC

    _secToc = appendSection();

    // TOC footer

    appendSection();

    ae ( "</ul>" );

    // CONTENT header

    ae ( "<div class=\"page-header\"><h2 id=\"symbols\">Symbols</h2></div>" );

    // CONTENT

    _secContent = appendSection();

    // FOOTER

    appendSection();

    ae ( "</div>" );
    ae ( "</div>" );
    ae ( "</body>" );
    ae ( "</html>" );

    setContentSection();
}

FileObject & HtmlFile::c ( const String & value )
{
    if ( value.isEmpty() )
        return a ( "<!-- -->" );

    return a ( "<!-- " ).a ( value ).a ( " -->" );
}
