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

#include "Diagnostics.hpp"

#include "auto/log/Log/LogType.hpp"
#include "auto/log/Log/DiagnosticMessage.hpp"

using namespace Pravala;

BinLog Diagnostics::_diagLog ( Log::LogType::DiagnosticMessage, "diag_updates" );
TextLog Diagnostics::_log ( "diagnostics" );

Diagnostics::Diagnostics()
{
}

Diagnostics & Diagnostics::get()
{
    static Diagnostics * instance = 0;

    if ( !instance )
    {
        instance = new Diagnostics();
    }

    return *instance;
}

void Diagnostics::set ( String key, String value )
{
    key = key.trimmed();
    value = value.trimmed();

    Log::Diagnostic diag;

    if ( _diagnostics.find ( key, diag ) && diag.getValue() == value )
    {
        return;
    }

    diag.setKey ( key );
    diag.setValue ( value );
    diag.setTimestamp ( time ( 0 ) );

    _diagnostics.insert ( key, diag );

    LOG ( L_DEBUG2, "Setting diagnostic: '" << key << "':'" << value << "' [" << diag.getTimestamp() << "]" );

    // Send the message if _diagLog is active
    if ( _diagLog.isActive() )
    {
        Log::DiagnosticMessage msg;

        msg.setDiagnostic ( diag );

        LOG ( L_DEBUG3, "Sending diagnostic update: " << diag.getKey()
              << "='" << diag.getValue() << "' [" << diag.getTimestamp() << "]" );

        _diagLog.send ( msg );
    }
}

void Diagnostics::remove ( String key )
{
    key = key.trimmed();

    LOG ( L_DEBUG2, "Removing diagnostic: '" << key << "'" );

    _diagnostics.remove ( key );

    if ( _diagLog.isActive() )
    {
        Log::Diagnostic diag;

        diag.setKey ( key );
        diag.setTimestamp ( time ( 0 ) );

        // No value - means we just removed that key.

        Log::DiagnosticMessage msg;

        msg.setDiagnostic ( diag );

        LOG ( L_DEBUG3, "Sending diagnostic update: " << diag.getKey()
              << "='" << diag.getValue() << "' [" << diag.getTimestamp() << "]" );

        _diagLog.send ( msg );
    }
}

const HashMap<String, Log::Diagnostic> & Diagnostics::getDiagnostics() const
{
    return _diagnostics;
}
