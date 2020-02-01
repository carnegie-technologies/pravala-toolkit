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

#include "basic/WString.hpp"
#include "basic/MsvcSupport.hpp"

#include "WinUtils.hpp"

extern "C"
{
#include <winsock2.h>
#include <windows.h>
}

#include <comdef.h>

#pragma comment(lib, "rpcrt4.lib")

#define CASE_FAC( f ) \
    case FACILITY_ ## f: \
        fDesc = #f; break

namespace Pravala
{
ERRCODE mapWindowsErrorToERRCODE ( long err )
{
    // fill in with ERROR_* -> Error::* mappings
    switch ( err )
    {
        case ERROR_SUCCESS:
            return Error::Success;

        case ERROR_NOT_ENOUGH_MEMORY:
            return Error::MemoryError;

        case ERROR_INVALID_PARAMETER:
        case ERROR_INVALID_HANDLE:
            return Error::InvalidParameter;

        case ERROR_INVALID_STATE:
            return Error::WrongState;

        case ERROR_ACCESS_DENIED:
            return Error::AccessDenied;

        case ERROR_ADDRESS_NOT_ASSOCIATED:
            return Error::NotAvailable;

        case ERROR_ALREADY_EXISTS:
            return Error::AlreadyExists;

        case ERROR_BAD_PROFILE:
            return Error::ConfigError;

        case ERROR_NO_DATA:
        case ERROR_NOT_FOUND:
            return Error::NotFound;

        case ERROR_INSUFFICIENT_BUFFER:
        case ERROR_BUFFER_OVERFLOW:
            return Error::TooMuchData;

        case ERROR_NO_MATCH:
        case ERROR_GEN_FAILURE:
            return Error::Unsupported;
    }

    return Error::InternalError;
}

String getWindowsErrorDesc ( long err )
{
    LPTSTR errString = 0;
    // This is correct usage of &errString when using FORMAT_MESSAGE_ALLOCATE_BUFFER.
    // See the GetFormattedMessage example here:
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ms679351%28v=vs.85%29.aspx
    FormatMessage ( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    0, err, 0, ( LPTSTR ) &errString, 0, 0 );

    if ( !errString )
        return String::EmptyString;

#ifdef UNICODE
    bool isOk;
    const String str = String ( "%1 [%2]" )
                       .arg ( WString ( ( const wchar_t * ) errString ).toString ( isOk ) ).arg ( err );
#else
    const String str = String ( "%1 [%2]" ).arg ( ( const char * ) errString ).arg ( err );
#endif

    LocalFree ( errString );

    return str;
}

String getLastWindowsErrorDesc()
{
    return getWindowsErrorDesc ( GetLastError() );
}

ERRCODE mapWindowsComErrorToERRCODE ( unsigned long hresult )
{
    // fill in with E_* -> Error::* mappings
    switch ( hresult )
    {
        case S_OK:
            return Error::Success;

        case E_OUTOFMEMORY:
            return Error::MemoryError;

        case E_HANDLE:
        case E_INVALIDARG:
        case E_POINTER:
            return Error::InvalidParameter;

        case E_NOTIMPL:
            return Error::NotImplemented;

        case E_ACCESSDENIED:
            return Error::AccessDenied;

        case E_NOINTERFACE:
        case __HRESULT_FROM_WIN32 ( ERROR_SERVICE_NOT_ACTIVE ):
            return Error::NotAvailable;

        case __HRESULT_FROM_WIN32 ( ERROR_NOT_FOUND ):
            return Error::NotFound;

        case __HRESULT_FROM_WIN32 ( ERROR_NOT_READY ):
            return Error::NotInitialized;

        case __HRESULT_FROM_WIN32 ( ERROR_NOT_SUPPORTED ):
            return Error::Unsupported;
    }

    return Error::InternalError;
}

String getWindowsComErrorDesc ( unsigned long hresult )
{
    _com_error comError ( hresult );
    LPCTSTR errString = comError.ErrorMessage(); // Don't need to free this

    if ( !errString )
        return String::EmptyString;

    // Description of HRESULT bits:
    // http://msdn.microsoft.com/en-us/library/cc231198.aspx

    String bits;

    if ( ( hresult & 0x80000000UL ) != 0 ) bits.append ( "S" ); // Severity
    if ( ( hresult & 0x40000000UL ) != 0 ) bits.append ( "R" ); // Reserved
    if ( ( hresult & 0x20000000UL ) != 0 ) bits.append ( "C" ); // Customer
    if ( ( hresult & 0x10000000UL ) != 0 ) bits.append ( "N" ); // NTSTATUS
    if ( ( hresult & 0x8000000UL ) != 0 ) bits.append ( "X" );  // Reserved

    if ( !bits.isEmpty() )
        bits.append ( ";" );

    unsigned long fac = ( ( hresult & 0x7FF0000UL ) >> 16 );

    String facility = String::number ( fac );
    String fDesc;

    switch ( fac )
    {
        CASE_FAC ( NULL );
        CASE_FAC ( RPC );
        CASE_FAC ( DISPATCH );
        CASE_FAC ( STORAGE );
        CASE_FAC ( ITF );
        CASE_FAC ( WIN32 );
        CASE_FAC ( WINDOWS );
        CASE_FAC ( SECURITY );
        CASE_FAC ( CONTROL );
        CASE_FAC ( CERT );
        CASE_FAC ( INTERNET );
        CASE_FAC ( MEDIASERVER );
        CASE_FAC ( MSMQ );
        CASE_FAC ( SETUPAPI );
        CASE_FAC ( SCARD );
        CASE_FAC ( COMPLUS );
        CASE_FAC ( AAF );
        CASE_FAC ( URT );
        CASE_FAC ( ACS );
        CASE_FAC ( DPLAY );
        CASE_FAC ( UMI );
        CASE_FAC ( SXS );
        CASE_FAC ( WINDOWS_CE );
        CASE_FAC ( HTTP );
        CASE_FAC ( USERMODE_COMMONLOG );
        CASE_FAC ( USERMODE_FILTER_MANAGER );
        CASE_FAC ( BACKGROUNDCOPY );
        CASE_FAC ( CONFIGURATION );
        CASE_FAC ( STATE_MANAGEMENT );
        CASE_FAC ( METADIRECTORY );
        CASE_FAC ( WINDOWSUPDATE );
        CASE_FAC ( DIRECTORYSERVICE );
        CASE_FAC ( GRAPHICS );
        CASE_FAC ( SHELL );
        CASE_FAC ( TPM_SERVICES );
        CASE_FAC ( TPM_SOFTWARE );
        CASE_FAC ( PLA );
        CASE_FAC ( FVE );
        CASE_FAC ( FWP );
        CASE_FAC ( WINRM );
        CASE_FAC ( NDIS );
        CASE_FAC ( USERMODE_HYPERVISOR );
        CASE_FAC ( CMI );
        CASE_FAC ( USERMODE_VIRTUALIZATION );
        CASE_FAC ( USERMODE_VOLMGR );
        CASE_FAC ( BCD );
        CASE_FAC ( USERMODE_VHD );
        CASE_FAC ( SDIAG );
        CASE_FAC ( WEBSERVICES );
        CASE_FAC ( WINDOWS_DEFENDER );
        CASE_FAC ( OPC );
    }

    if ( !fDesc.isEmpty() )
    {
        facility.append ( String ( "<%1>" ).arg ( fDesc ) );
    }

    String codeDesc = String ( "[%1:%2%3;%4]" ).arg ( hresult ).arg ( bits, facility ).arg ( hresult & 0xFFFFUL );

    if ( hresult == 0 )
    {
        codeDesc = "[0]";
    }

#ifdef UNICODE
    bool isOk;
    return String ( "%1 %2" )
           .arg ( WString ( ( const wchar_t * ) errString ).toString ( isOk ) ).arg ( codeDesc );
#else
    return String ( "%1 %2" ).arg ( ( const char * ) errString ).arg ( codeDesc );
#endif
}

String getWindowsRpcStatusDesc ( unsigned long status )
{
    LPCTSTR errString[ DCE_C_ERROR_STRING_LEN ];

    DceErrorInqText ( status, ( RPC_CSTR ) errString );

#ifdef UNICODE
    bool isOk;
    return String ( "%1 [%2]" )
           .arg ( WString ( ( const wchar_t * ) errString ).toString ( isOk ) ).arg ( status );
#else
    return String ( "%1 [%2]" ).arg ( ( const char * ) errString ).arg ( status );
#endif
}

String getWindowsGuidString ( const GUID & guid )
{
    // Format defined here:
    // http://msdn.microsoft.com/en-us/library/windows/desktop/aa373931%28v=vs.85%29.aspx
    // The braces are used because it appears to be a common string format for the adapter name
    // as returned in IP_ADAPTER_ADDRESSES by GetAdaptersAddresses and IP_ADAPTER_INDEX_MAP by GetInterfaceInfo

    // Format: "{01234567-0123-0123-0123-012345678901}" - 38 characters

    char buf[ 40 ];

    //                                       A    B    C    D   E    F   G   H   I   J   K
    const int len = snprintf ( buf, 40, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                               guid.Data1,    // A
                               guid.Data2,    // B
                               guid.Data3,    // C
                               guid.Data4[ 0 ], // D
                               guid.Data4[ 1 ], // E
                               guid.Data4[ 2 ], // F
                               guid.Data4[ 3 ], // G
                               guid.Data4[ 4 ], // H
                               guid.Data4[ 5 ], // I
                               guid.Data4[ 6 ], // J
                               guid.Data4[ 7 ]  // K
                    );

    const String str ( buf, len );

    assert ( str == str.toLower() );

    return str;
}

ERRCODE getWindowsGuidFromString ( const String & str, GUID & guid )
{
    String cleaned = str.replace ( "{", "" ).replace ( "}", "" );
    const RPC_STATUS status = UuidFromStringA ( ( RPC_CSTR ) cleaned.c_str(), &guid );

    return ( status == RPC_S_OK ) ? ( Error::Success ) : ( Error::InvalidParameter );
}

ERRCODE getWindowsGuidFromString ( const WString & wstr, GUID & guid )
{
    bool isOk = false;
    String str = wstr.toString ( &isOk );

    if ( !isOk )
        return Error::InvalidParameter;

    return getWindowsGuidFromString ( str, guid );
}

ERRCODE generateGuid ( GUID & guid )
{
    const RPC_STATUS status = UuidCreateSequential ( &guid );

    return ( status == RPC_S_OK || status == RPC_S_UUID_LOCAL_ONLY ) ? ( Error::Success ) : ( Error::InternalError );
}

String getWindowsIidString ( REFIID iid )
{
    LPOLESTR wstr = 0;
    HRESULT hresult = StringFromIID ( iid, &wstr );
    String str;

    if ( SUCCEEDED ( hresult ) )
        str = WString ( ( const wchar_t * ) wstr ).toString();

    CoTaskMemFree ( wstr );

    return str;
}

bool isWindows8OrNewer()
{
    OSVERSIONINFOEX ver;
    DWORDLONG condMask = 0;

    ZeroMemory ( &ver, sizeof ( OSVERSIONINFOEX ) );
    ver.dwOSVersionInfoSize = sizeof ( OSVERSIONINFOEX );

    // Windows 8 is 6.2
    ver.dwMajorVersion = 6;
    ver.dwMinorVersion = 2;

    VER_SET_CONDITION ( condMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
    VER_SET_CONDITION ( condMask, VER_MINORVERSION, VER_GREATER_EQUAL );

    // If VerifyVersionInfo returns true, it means that Windows is the specified version or greater,
    // i.e. we are running Windows 8 or newer.

    return VerifyVersionInfo ( &ver, VER_MAJORVERSION | VER_MINORVERSION, condMask ) != 0;
}
}
