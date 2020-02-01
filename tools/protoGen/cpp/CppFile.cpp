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

#include "CppFile.hpp"

using namespace Pravala;

CppFile::CppFile ( const String & path, const String & singleIndent ):
    FileObject ( path, singleIndent )
{
    _secHeader = appendSection();
    _secNamesp = appendSection();
    _secBody = appendSection();
    _secFooter = appendSection();

    setHeaderSection();
    genAutogenComment();

    setBodySection();
}

FileObject & CppFile::c ( const String & value )
{
    if ( value.isEmpty() )
        return a ( "///" );

    return a ( "/// " ).a ( value );
}

void CppFile::finalizeFile()
{
    setHeaderSection();
    genIncludes();
    setFooterSection();

    FileObject::finalizeFile();
}

void CppFile::genAutogenComment()
{
    a ( "//" ).e();
    a ( "// This file has been automatically generated" ).e();
    a ( "//" ).e();
    a ( "// DO NOT EDIT" ).e();
    a ( "//" ).e();
    e();
}

void CppFile::genIncludes()
{
    bool added = false;

    for ( int idx = 0; idx < IncludeSize; ++idx )
    {
        for ( HashMap<String, IncludeType>::Iterator it ( _cIncludes ); it.isValid(); it.next() )
        {
            if ( it.value() == ( IncludeType ) idx )
            {
                if ( !added )
                {
                    a ( "extern \"C\"" ).e();
                    a ( "{" ).e();
                    added = true;
                }

                if ( ( IncludeType ) idx == IncludeGlobal )
                {
                    a ( "#include <" ).a ( it.key() ).a ( ">" ).e();
                }
                else
                {
                    a ( "#include \"" ).a ( it.key() ).a ( "\"" ).e();
                }
            }
        }
    }

    if ( added )
    {
        // We append, no need to use separate subsection
        a ( "}" ).e();
        e();
    }

    for ( int idx = 0; idx < IncludeSize; ++idx )
    {
        added = false;

        for ( HashMap<String, IncludeType>::Iterator it ( _cppIncludes ); it.isValid(); it.next() )
        {
            if ( it.value() == ( IncludeType ) idx )
            {
                added = true;

                if ( ( IncludeType ) idx == IncludeGlobal )
                {
                    a ( "#include <" ).a ( it.key() ).a ( ">" ).e();
                }
                else
                {
                    a ( "#include \"" ).a ( it.key() ).a ( "\"" ).e();
                }
            }
        }

        if ( added )
            e();
    }
}

void CppFile::addCInclude ( const String & path, IncludeType includeType )
{
    if ( path == _path )
        return;

    _cIncludes.insert ( path, includeType );
}

void CppFile::addCppInclude ( const String & path, IncludeType includeType )
{
    if ( path == _path )
        return;

    if ( includeType == CppFile::IncludeOwn
         && !path.startsWith ( "/" )
         && !path.startsWith ( "./" ) )
    {
        String nPath ( "./" );
        nPath.append ( path );

        _cppIncludes.insert ( nPath, includeType );
    }
    else
    {
        _cppIncludes.insert ( path, includeType );
    }
}
