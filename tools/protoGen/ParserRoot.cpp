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

#include "basic/MemHandle.hpp"
#include "basic/Buffer.hpp"
#include "md5/Md5.hpp"
#include "ParserRoot.hpp"
#include "Parser.hpp"

using namespace Pravala;

bool ParserRoot::addImportDir ( const String & dir )
{
    if ( dir.isEmpty() )
    {
        return false;
    }

    if ( !dir.endsWith ( "/" ) )
    {
        _importDirs.append ( String ( "%1/" ).arg ( dir ) );
    }
    else
    {
        _importDirs.append ( dir );
    }

    return true;
}

bool ParserRoot::addInputFile ( const String & file )
{
    if ( file.isEmpty() )
    {
        return false;
    }

    _inputFiles.append ( file );
    return true;
}

bool ParserRoot::run ( ProtocolSpec & protoSpec )
{
    for ( size_t i = 0; i < _inputFiles.size(); ++i )
    {
        bool isOk = false;
        const MemHandle buf ( _inputFiles.at ( i ), &isOk );

        if ( !isOk )
        {
            fprintf ( stderr, "Error reading from file '%s'\n", _inputFiles.at ( i ).c_str() );

            return false;
        }

        _generateOutputForFiles.insert ( Md5::generate ( buf ) );
    }

    for ( size_t i = 0; i < _inputFiles.size(); ++i )
    {
        Parser p ( _inputFiles.at ( i ) );

        if ( !p.run ( *this, protoSpec ) )
            return false;
    }

    return true;
}
