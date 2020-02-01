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

#include "JavaFile.hpp"

using namespace Pravala;

JavaFile::JavaComment::JavaComment ( FileObject & myFile, const String & text ):
    CommentBlock ( myFile, " *" )
{
    myFile.e().e().a ( "/**" ).e();
    a ( text );
}

JavaFile::JavaComment::~JavaComment()
{
    if ( !_text.isEmpty() )
        e();

    _myFile.a ( "**/" ).e();
}

JavaFile::JavaFile ( const String & path, const String & singleIndent ):
    FileObject ( path, singleIndent )
{
    _secHeader = appendSection();
    _secPackage = appendSection();
    _secBody = appendSection();
    _secFooter = appendSection();

    setBodySection();
}

JavaFile::JavaComment JavaFile::comment (
        const String & textA, const String & textB,
        const String & textC, const String & textD )
{
    return JavaComment ( *this, String ( textA ).append ( textB ).append ( textC ).append ( textD ) );
}

void JavaFile::co()
{
    e().e().a ( "/**" ).e();
}

void JavaFile::cc()
{
    a ( " */" ).e();
}

FileObject & JavaFile::c ( const String & value )
{
    if ( value.isEmpty() )
        return a ( " *" );

    return a ( " * " ).a ( value );
}

void JavaFile::genAutogenComment()
{
    a ( "//" ).e();
    a ( "// This file has been automatically generated" ).e();
    a ( "//" ).e();
    a ( "// DO NOT EDIT" ).e();
    a ( "//" ).e();
    e();
}

void JavaFile::genImports()
{
    for ( HashSet<String>::Iterator it ( _imports ); it.isValid(); it.next() )
    {
        a ( "import " ).a ( it.value() ).a ( ";" ).e();
    }

    if ( !_imports.isEmpty() )
        e();
}

void JavaFile::addImport ( const String & path )
{
    if ( path == _path )
        return;

    _imports.insert ( path );
}
