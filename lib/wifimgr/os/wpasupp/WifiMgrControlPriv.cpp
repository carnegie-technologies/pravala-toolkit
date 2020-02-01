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

#include "WifiMgrControlPriv.hpp"

using namespace Pravala;

const String WifiMgrControlPriv::SetNetworkPrefix ( "SET_NETWORK " );
const String WifiMgrControlPriv::RemoveNetworkPrefix ( "REMOVE_NETWORK " );
const String WifiMgrControlPriv::EnableNetworkPrefix ( "ENABLE_NETWORK " );
const String WifiMgrControlPriv::DisableNetworkPrefix ( "DISABLE_NETWORK " );

const String WifiMgrControlPriv::AddNetworkCmd ( "ADD_NETWORK" );
const String WifiMgrControlPriv::ScanCmd ( "SCAN" );
const String WifiMgrControlPriv::ListNetworksCmd ( "LIST_NETWORKS" );
const String WifiMgrControlPriv::ScanResultsCmd ( "SCAN_RESULTS" );
const String WifiMgrControlPriv::StatusCmd ( "STATUS" );
const String WifiMgrControlPriv::SaveCmd ( "SAVE_CONFIG" );

const String WifiMgrControlPriv::OkResult ( "OK\n" );
const String WifiMgrControlPriv::FailResult ( "FAIL\n" );

WifiMgrControlPriv::WifiMgrControlPriv ( const String & ctrlName ): WpaSuppCore ( ctrlName )
{
}

WifiMgrControlPriv::~WifiMgrControlPriv()
{
}

bool WifiMgrControlPriv::executeSetNetworkCommand ( int id, const String & params )
{
    String resp;
    String cmd = WifiMgrControlPriv::SetNetworkPrefix;
    cmd.append ( String::number ( id ) ).append ( params );

    ERRCODE eCode = executeCommand ( cmd, resp );

    if ( NOT_OK ( eCode ) || resp == FailResult )
    {
        LOG_ERR ( L_ERROR, eCode, "Unable to add network. Failed to set params: '" << params
                  << "'. Return: " << resp );

        cmd = RemoveNetworkPrefix;
        cmd.append ( String::number ( id ) );

        executeCommand ( cmd, resp );
        return false;
    }

    return true;
}

bool WifiMgrControlPriv::executeBoolCommand ( const String & cmd )
{
    String resp;
    ERRCODE eCode = executeCommand ( cmd, resp );

    if ( NOT_OK ( eCode ) || resp != OkResult )
    {
        return false;
    }

    return true;
}
