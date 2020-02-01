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

extern "C"
{
#include <curl/curl.h>
}

#include "basic/Math.hpp"
#include "basic/Buffer.hpp"
#include "basic/Mutex.hpp"
#include "sys/Compression.hpp"
#include "EasyCurl.hpp"

using namespace Pravala;

/// @brief Copy up to size*nitems bytes from a MemHandle to buffer.
/// @note This is used as a callback for POSTing data.
/// @param [out] buffer A buffer to copy to.
/// @param [in] size The size of the items to copy.
/// @param [in] nitems The number of items to copy.
/// @param [in] instream A pointer to the MemHandle to copy from, cast to void*.
/// @return The number of bytes copied.
static size_t curlReadFromMemHandle ( char * buffer, size_t size, size_t nitems, void * instream )
{
    MemHandle * const handle = static_cast<MemHandle *> ( instream );

    if ( !handle )
    {
        // This will stop the POST immediately.
        return CURL_READFUNC_ABORT;
    }

    const size_t copySize = min ( size * nitems, handle->size() );

    memcpy ( buffer, handle->get(), copySize );

    handle->consume ( copySize );

    return copySize;
}

/// @brief Copy up to size*nmemb bytes from a pointer to a Buffer.
/// @note This is used as a callback for GETing data.
/// @param [out] ptr Pointer to the data to write.
/// @param [in] size The size of the items to copy.
/// @param [in] nmemb The number of items to copy.
/// @param [in] userdata A pointer to the Buffer to copy to, cast to void*.
/// @return The number of bytes copied.
static size_t curlWriteToBuffer ( char * ptr, size_t size, size_t nmemb, void * userdata )
{
    Buffer * const buf = static_cast<Buffer *> ( userdata );

    if ( !buf )
    {
        return 0;
    }

    const size_t orgSize = buf->size();

    buf->appendData ( ptr, size * nmemb );

    return buf->size() - orgSize;
}

EasyCurl::EasyCurl (
        const String & url, HttpMethod method, uint8_t flags,
        const MemHandle & sendData, const String & contentType ):
    Method ( method ),
    Url ( url ),
    _curl ( 0 ),
    _curlHeaders ( 0 ),
    _contentType ( contentType ),
    _respCode ( 0 ),
    _flags ( flags )
{
    if ( globalInit ( &_errStr ) != CURLE_OK )
        return;

    CURL * const curl = curl_easy_init();

    if ( !curl )
    {
        _errStr = "Error initializing cURL";
        return;
    }

    switch ( Method )
    {
        case MethodGet:
        case MethodDelete:
            break;

        case MethodPost:
        case MethodPut:
            _sendData = sendData;
            break;
    }

    _curl = curl;
}

EasyCurl::~EasyCurl()
{
    if ( _curlHeaders != 0 )
    {
        curl_slist_free_all ( _curlHeaders );
        _curlHeaders = 0;
    }

    if ( _curl != 0 )
    {
        curl_easy_cleanup ( ( CURL * ) _curl );
        _curl = 0;
    }
}

bool EasyCurl::doCurl ( int curlCode )
{
    if ( curlCode == CURLE_OK )
        return true;

    _errStr = curl_easy_strerror ( ( CURLcode ) curlCode );
    return false;
}

bool EasyCurl::setUserAgent ( const String & userAgent )
{
    return ( _curl != 0 && doCurl ( curl_easy_setopt ( ( CURL * ) _curl, CURLOPT_USERAGENT, userAgent.c_str() ) ) );
}

bool EasyCurl::setUserPassword ( const String & user, const String & password )
{
    return ( _curl != 0
             && doCurl ( curl_easy_setopt ( ( CURL * ) _curl,
                                            CURLOPT_USERPWD,
                                            String ( "%1:%2" ).arg ( user, password ).c_str() ) ) );
}

bool EasyCurl::appendHeader ( const String & name, const String & value )
{
    const String n = name.simplified();
    const String v = value.simplified();

    if ( n.isEmpty() || v.isEmpty() )
        return false;

    _curlHeaders = curl_slist_append ( _curlHeaders, String ( "%1: %2" ).arg ( n, v ).c_str() );

    return ( _curlHeaders != 0 );
}

bool EasyCurl::setTimeout ( unsigned int timeout )
{
    return ( _curl != 0 && doCurl ( curl_easy_setopt ( ( CURL * ) _curl, CURLOPT_TIMEOUT, timeout ) ) );
}

ERRCODE EasyCurl::run()
{
    _respCode = 0;
    _rcvdData.clear();
    _rcvdHeaders.clear();

    if ( !_curl )
    {
        _errStr = "EasyCurl failed to initialize properly";
        return Error::NotInitialized;
    }

    if ( Url.isEmpty() )
    {
        _errStr = "Empty URL specified";
        return Error::ConfigError;
    }

    // For convenience:
    CURL * const curl = ( CURL * ) _curl;

    if ( !_sendData.isEmpty() )
    {
        if ( !_contentType.isEmpty() && !appendHeader ( "Content-Type", _contentType ) )
        {
            _errStr = String ( "Could not append content-type header: '%1'" ).arg ( _contentType );
            return Error::ConfigError;
        }

        if ( ( _flags & FlagCompress ) != 0 )
        {
            const MemHandle out = Compression::gzip ( _sendData );

            if ( !out.isEmpty() && appendHeader ( "Content-Encoding", "gzip" ) )
            {
                _sendData = out;
            }
        }

        if (
            // curlReadFromMemHandle will be called until it has copied CURLOPT_INFILESIZE bytes of data.
            // &_sendData will be passed as the instream parameter to curlReadFromMemHandle.
            !doCurl ( curl_easy_setopt ( curl, CURLOPT_READFUNCTION, curlReadFromMemHandle ) )
            || !doCurl ( curl_easy_setopt ( curl, CURLOPT_READDATA, &_sendData ) )

            // CURLOPT_INFILESIZE is the amount of data libcurl should read from _sendData
            || !doCurl ( curl_easy_setopt ( curl, CURLOPT_INFILESIZE, _sendData.size() ) )
        )
        {
            return Error::InternalError;
        }
    }

    bool validMethod = false;

    switch ( Method )
    {
        case MethodGet:
            validMethod = true;
            break;

        case MethodPost:
            validMethod = true;

            if (
                // Do a POST request
                !doCurl ( curl_easy_setopt ( curl, CURLOPT_POST, 1 ) )

                // CURLOPT_POSTFIELDSIZE is the length of the data to send.
                || !doCurl ( curl_easy_setopt ( curl, CURLOPT_POSTFIELDSIZE, _sendData.size() ) )
            )
            {
                // For POST operations cURL usually uses CURLOPT_POSTFIELDS or CURLOPT_COPYPOSTFIELDS options
                // to specify what data to post.
                // But if these are not set, it can also use CURLOPT_READFUNCTION method.
                // This is also what we have to do for PUT method. To simplify the code we use the same for both.
                // However, for POST to work properly, we still need to set CURLOPT_POSTFIELDSIZE.

                return Error::InternalError;
            }
            break;

        case MethodPut:
            validMethod = true;

            // Do a PUT request (cURL uses name 'UPLOAD' for that)
            if ( !doCurl ( curl_easy_setopt ( curl, CURLOPT_UPLOAD, 1 ) ) )
            {
                return Error::InternalError;
            }
            break;

        case MethodDelete:
            validMethod = true;

            // Do a DELETE request
            if ( !doCurl ( curl_easy_setopt ( curl, CURLOPT_CUSTOMREQUEST, "DELETE" ) ) )
            {
                return Error::InternalError;
            }
            break;
    }

    if ( !validMethod )
    {
        _errStr = String ( "Invalid HTTP method selected: %1" ).arg ( ( int ) Method );
        return Error::ConfigError;
    }

    Buffer rcvBuf;

    if (
        !doCurl ( curl_easy_setopt ( curl, CURLOPT_URL, Url.c_str() ) )

        // We want to read headers as well. They will be written to the same buffer.
        || !doCurl ( curl_easy_setopt ( curl, CURLOPT_HEADER, 1 ) )

        // Pass read data to 'rcvBuf'.
        || !doCurl ( curl_easy_setopt ( curl, CURLOPT_WRITEFUNCTION, curlWriteToBuffer ) )
        || !doCurl ( curl_easy_setopt ( curl, CURLOPT_WRITEDATA, &rcvBuf ) )

        // Don't check the CA that signed the cert.
        // This means that anyone can MITM the connection, but means we don't need
        // a pre-shared cert.
        /// @todo TODO This should be changed to use CURLOPT_CAPATH to specify a cert.
        || !doCurl ( curl_easy_setopt ( curl, CURLOPT_SSL_VERIFYPEER, 0 ) )
        || !doCurl ( curl_easy_setopt ( curl, CURLOPT_SSL_VERIFYHOST, 0 ) )

        || ( ( _flags & FlagFollowRedirects )
             && !doCurl ( curl_easy_setopt ( curl, CURLOPT_FOLLOWLOCATION, 1 ) ) )

        || ( _curlHeaders != 0
             && !doCurl ( curl_easy_setopt ( curl, CURLOPT_HTTPHEADER, _curlHeaders ) ) )
    )
    {
        return Error::InternalError;
    }

    const CURLcode res = curl_easy_perform ( curl );

    if ( res != CURLE_OK )
    {
        _errStr = curl_easy_strerror ( res );

        return ( res == CURLE_OPERATION_TIMEDOUT ) ? ( Error::Timeout ) : ( Error::SocketFailed );
    }

    long rCode = 0;

    if ( !doCurl ( curl_easy_getinfo ( curl, CURLINFO_RESPONSE_CODE, &rCode ) ) )
    {
        return Error::InternalError;
    }

    _respCode = ( int ) rCode;

    if ( !rcvBuf.isEmpty() )
    {
        // Let's get the headers from the buffer.
        // Everything after the headers will be the actual data.

        const size_t hdrSize = detectHeadersSize ( rcvBuf.get(), rcvBuf.size() );

        if ( hdrSize > 0 )
        {
            StringList lines = rcvBuf.getHandle ( 0, hdrSize ).toStringList ( "\r\n" );

            for ( size_t i = 0; i < lines.size(); ++i )
            {
                const String & hdr = lines.at ( i );
                const int colon = hdr.findFirstOf ( ":" );

                if ( colon > 0 )
                {
                    const String hName = hdr.substr ( 0, colon ).simplified();

                    if ( !hName.isEmpty() )
                    {
                        _rcvdHeaders.insert ( hName, hdr.substr ( colon + 1 ).trimmed() );
                    }
                }
            }
        }

        _rcvdData = rcvBuf.getHandle ( hdrSize );
    }

    return Error::Success;
}
