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

#include "FileObject.hpp"

using namespace Pravala;

TextFile::TextFile ( const String & path, const String & singleIndent ): FileObject ( path, singleIndent )
{
    appendSection();
}

FileObject & TextFile::c ( const String & )
{
    return *this;
}

FileObject::FileObject ( const String & path, const String & obSingleIndent ):
    _baseIndent ( 0 ),
    _sectionId ( -1 ),
    _path ( path ),
    _singleIndent ( obSingleIndent )
{
}

FileObject::~FileObject()
{
}

void FileObject::finalizeFile()
{
}

String & FileObject::getLineToAppend()
{
    assert ( _sectionId >= 0 );
    assert ( ( size_t ) _sectionId < _data.size() );

    StringList & strList = _data[ _sectionId ];

    if ( strList.size() < 1 )
        strList.append ( String::EmptyString );

    return strList[ strList.size() - 1 ];
}

int FileObject::getLineCount() const
{
    assert ( _sectionId >= 0 );
    assert ( ( size_t ) _sectionId < _data.size() );

    return _data[ _sectionId ].size();
}

FileObject & FileObject::i ( int iLevel )
{
    String & str = getLineToAppend();

    if ( str.length() > 0 )
    {
        return e().i ( iLevel );
    }

    if ( _baseIndent > 0 )
        iLevel += _baseIndent;

    while ( --iLevel >= 0 )
    {
        str.append ( _singleIndent );
    }

    return *this;
}

void FileObject::incBaseIndent ( int byWhat )
{
    if ( byWhat < 0 )
        return;

    _baseIndent += byWhat;
}

void FileObject::decBaseIndent ( int byWhat )
{
    if ( byWhat < 0 )
        return;

    assert ( _baseIndent >= byWhat );

    _baseIndent -= byWhat;

    assert ( _baseIndent >= 0 );
}

FileObject & FileObject::a ( const String & value )
{
    String & str = getLineToAppend();

    if ( str.length() < 1 && _baseIndent > 0 )
        i ( 0 );

    str.append ( value );

    return *this;
}

FileObject & FileObject::a ( int iLevel, const String & value )
{
    return i ( iLevel ).a ( value );
}

FileObject & FileObject::a ( int value )
{
    String & str = getLineToAppend();

    if ( str.length() < 1 && _baseIndent > 0 )
        i ( 0 );

    str.append ( String::number ( value ) );

    return *this;
}

FileObject & FileObject::alines ( const String & value )
{
    return alines ( 0, value );
}

FileObject & FileObject::alines ( int iLevel, const String & value )
{
    if ( getLineToAppend().length() > 0 )
    {
        // Finish current line
        e();
    }

    const StringList lines = value.split ( "\n", true );

    for ( size_t idx = 0; idx < lines.size(); ++idx )
    {
        String line = lines.at ( idx );

        int ind = ( iLevel > 0 ) ? iLevel : 0;

        while ( line.startsWith ( " " ) )
        {
            ++ind;
            line = line.substr ( 2 );
        }

        if ( line.isEmpty() )
        {
            e();
            continue;
        }

        ae ( ind, line );
    }

    return *this;
}

FileObject & FileObject::ae ( const String & value )
{
    return a ( value ).e();
}

FileObject & FileObject::ae ( int iLevel, const String & value )
{
    return i ( iLevel ).ae ( value );
}

FileObject & FileObject::e ( bool force )
{
    assert ( _sectionId >= 0 );
    assert ( ( size_t ) _sectionId < _data.size() );

    StringList & data = _data[ _sectionId ];

    if ( !force
         && data.size() > 1
         && data[ data.size() - 1 ].length() < 1
         && data[ data.size() - 2 ].length() < 1 )
        return *this;

    data.append ( String::EmptyString );

    return *this;
}

FileObject & FileObject::ce ( const String & value )
{
    return c ( value ).e();
}

int FileObject::appendSection()
{
    _sectionId = _data.size();
    _data.append ( StringList() );

    return _sectionId;
}

void FileObject::setSection ( const int sectionId )
{
    assert ( sectionId >= 0 );
    assert ( ( size_t ) sectionId < _data.size() );

    _sectionId = sectionId;
}

void FileObject::writeOutput ( FILE * toFile )
{
    assert ( toFile != 0 );

    bool lastEmpty = false;

    for ( size_t idx = 0; idx < _data.size(); ++idx )
    {
        const StringList & lines = _data[ idx ];

        for ( size_t j = 0; j < lines.size(); ++j )
        {
            const String & line = lines[ j ];

            // We don't want to have more than one empty line in a row
            if ( !lastEmpty || !line.isEmpty() )
            {
                fprintf ( toFile, "%s\n", line.c_str() );
            }

            lastEmpty = line.isEmpty();
        }
    }
}

void FileObject::genVarComments ( Element * elem, int ind )
{
    String ofField = "of the field";

    if ( elem->comment.length() > 0 )
    {
        i ( ind ).ce ( elem->comment );
    }

    if ( elem->isRequired() )
    {
        i ( ind ).ce ( "This field is [REQUIRED]." );
    }
    else if ( elem->isOptional() )
    {
        i ( ind ).ce ( "This field is [OPTIONAL]." );
    }
    else if ( elem->isRepeated() )
    {
        if ( elem->options.contains ( ProtocolSpec::KW_MIN_LIST_SIZE ) )
        {
            i ( ind ).c ( "Minimum list size: " ).a ( elem->options[ ProtocolSpec::KW_MIN_LIST_SIZE ] ).a ( "." ).e();
        }

        if ( elem->options.contains ( ProtocolSpec::KW_MAX_LIST_SIZE ) )
        {
            i ( ind ).c ( "Maximum list size: " ).a ( elem->options[ ProtocolSpec::KW_MAX_LIST_SIZE ] ).a ( "." ).e();
        }

        ofField = "of each element in the list";
    }

    if ( elem->options.contains ( ProtocolSpec::KW_MIN_LENGTH ) )
    {
        i ( ind ).c ( "Minimum length " ).a ( ofField );
        a ( ": " ).a ( elem->options[ ProtocolSpec::KW_MIN_LENGTH ] ).a ( "." ).e();
    }

    if ( elem->options.contains ( ProtocolSpec::KW_MAX_LENGTH ) )
    {
        i ( ind ).c ( "Minimum length " ).a ( ofField );
        a ( ": " ).a ( elem->options[ ProtocolSpec::KW_MAX_LENGTH ] ).a ( "." ).e();
    }

    if ( elem->options.contains ( ProtocolSpec::KW_MIN ) )
    {
        i ( ind ).c ( "Minimum value " ).a ( ofField );
        a ( ": " ).a ( elem->options[ ProtocolSpec::KW_MIN ] ).a ( "." ).e();
    }

    if ( elem->options.contains ( ProtocolSpec::KW_MAX ) )
    {
        i ( ind ).c ( "Maximum value " ).a ( ofField );
        a ( ": " ).a ( elem->options[ ProtocolSpec::KW_MAX ] ).a ( "." ).e();
    }
}
