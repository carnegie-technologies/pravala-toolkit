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

#include "CommentBlock.hpp"
#include "FileObject.hpp"

using namespace Pravala;

CommentBlock::CommentBlock ( FileObject & myFile, const String & commentPrefix ):
    _myFile ( myFile ), _commentPrefix ( commentPrefix ), _numLines ( 0 )
{
}

CommentBlock::~CommentBlock()
{
    if ( !_text.isEmpty() )
        e();
}

CommentBlock & CommentBlock::a ( const String & value )
{
    if ( value.isEmpty() )
        return *this;

    if ( _text.isEmpty() )
    {
        _text = _commentPrefix;

        if ( !_text.endsWith ( " " ) && !value.startsWith ( " " ) )
            _text.append ( " " );
    }

    _text.append ( value );

    return *this;
}

CommentBlock & CommentBlock::a ( long value )
{
    return a ( String::number ( value ) );
}

CommentBlock & CommentBlock::e()
{
    if ( !_text.isEmpty() )
    {
        _myFile.a ( _text );
        _text.clear();
    }

    _myFile.e();
    ++_numLines;

    return *this;
}
