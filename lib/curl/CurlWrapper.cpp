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

#include "CurlWrapper.hpp"
#include "CurlMWrapper.hpp"
#include "basic/Math.hpp"

using namespace Pravala;

#define DEFAULT_MAX_TRANSFER_LENGTH    100000

TextLog CurlWrapper::_log ( "curl_wrapper" );

CurlWrapper::CurlWrapper ( Receiver & receiver ):
    _receiver ( receiver ),
    _curlm ( new CurlMWrapper ),
    _abortTimer ( *this ),
    _curl ( 0 ),
    _curlHeaders ( 0 ),
    _maxLen ( DEFAULT_MAX_TRANSFER_LENGTH ),
    _timeout ( 0 ),
    _inProgress ( false ),
    _doFollow ( false ),
    _privateCurlM ( true )
{
}

CurlWrapper::CurlWrapper ( CurlWrapper::Receiver & receiver, CurlMWrapper & mCurl ):
    _receiver ( receiver ),
    _curlm ( &mCurl ),
    _abortTimer ( *this ),
    _curl ( 0 ),
    _curlHeaders ( 0 ),
    _maxLen ( DEFAULT_MAX_TRANSFER_LENGTH ),
    _timeout ( 0 ),
    _inProgress ( false ),
    _doFollow ( false ),
    _privateCurlM ( false )
{
}

CurlWrapper::~CurlWrapper()
{
    stop();

    if ( _privateCurlM )
    {
        delete _curlm;
    }

    _curlm = 0;
}

bool CurlWrapper::setPayload ( const String & data, const String & contentType )
{
    if ( _inProgress )
    {
        return false;
    }

    if ( _payloadData.size() < ( size_t ) data.length() )
    {
        _payloadData = MemHandle ( data.length() );
    }

    _payloadData.truncate ( data.length() );

    if ( _payloadData.size() > 0 )
    {
        memcpy ( _payloadData.getWritable(), data.c_str(), _payloadData.size() );
    }
    else if ( data.length() > 0 )
    {
        return false;
    }

    _contentType = contentType;
    return true;
}

bool CurlWrapper::setPayload ( const MemHandle & data, const String & contentType )
{
    if ( _inProgress )
    {
        return false;
    }

    _payloadData = data;
    _contentType = contentType;
    return true;
}

bool CurlWrapper::setUserAgent ( const String & userAgent )
{
    if ( _inProgress )
    {
        return false;
    }

    _userAgent = userAgent;

    return true;
}

bool CurlWrapper::setHeader ( const String & headerKey, const String & headerValue )
{
    if ( _inProgress )
    {
        return false;
    }

    assert ( !headerKey.isEmpty() );

    _headers[ headerKey ] = headerValue;

    return true;
}

bool CurlWrapper::setUserPass ( const String & user, const String & password )
{
    if ( _inProgress )
    {
        return false;
    }

    _userPass.clear();
    _userPass.append ( user ).append ( ':' ).append ( password );

    return true;
}

void CurlWrapper::setFollowLocation ( bool follow )
{
    _doFollow = follow;
}

void CurlWrapper::setTimeout ( long timeout )
{
    _timeout = timeout;
}

CURL * CurlWrapper::initCurlEasyHandle()
{
    return curl_easy_init();
}

ERRCODE CurlWrapper::start (
        const String & url, HttpMethod method, uint32_t timeout,
        bool outputHdrs, CURLMcode * curlMCode )
{
    if ( _inProgress )
    {
        // wrapper running, method not available
        return Error::NotAvailable;
    }

    assert ( !_curl );

    _curl = initCurlEasyHandle();

    if ( !_curl )
    {
        if ( curlMCode != 0 )
        {
            *curlMCode = CURLM_LAST;
        }

        return Error::InternalError;
    }

    _url = url;

    CURLcode eCode = CURLE_OK;
    UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_VERBOSE, 0 ) );

    if ( outputHdrs )
        UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_HEADER, 1 ) );

    UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_NOPROGRESS, 1 ) );
    // we don't care about signals, timer cleans us up
    UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_NOSIGNAL, 1 ) );
    UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_WRITEFUNCTION, CurlWrapper::curlDataSink ) );
    UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_WRITEDATA, this ) );
    UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_URL, _url.c_str() ) );
    UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_FOLLOWLOCATION, _doFollow ? 1 : 0 ) );

    if ( _timeout > 0 )
    {
        UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_TIMEOUT, _timeout ) );
    }

#ifndef NDEBUG
    memset ( _curlErrMsg, 0, CURL_ERROR_SIZE );
    curl_easy_setopt ( _curl, CURLOPT_ERRORBUFFER, _curlErrMsg );
#endif

    if ( eCode == CURLE_OK
         && timeout > 0
         && curl_easy_setopt ( _curl, CURLOPT_TIMEOUT, timeout ) != CURLE_OK )
    {
        LOG ( L_FATAL_ERROR, "Error starting CurlWrapper; Timeout value (" << timeout << ") is too large" );

        return Error::TooBigValue;
    }

    if ( !_userAgent.isEmpty() )
    {
        UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_USERAGENT, _userAgent.c_str() ) );
    }

    if ( !_userPass.isEmpty() )
    {
        UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_USERPWD, _userPass.c_str() ) );
    }

    // enforce that curl doesn't reuse connections
    UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_FORBID_REUSE, 1 ) );

    // we don't handle verifying remote cert right now
    UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_SSL_VERIFYPEER, 0 ) );

    bool addContentType = false;

    switch ( method )
    {
        case MethodGet:
            // Do nothing, default are ok
            break;

        case MethodPost:
            UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_POST, 1 ) );
            UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_POSTFIELDS, _payloadData.get() ) );
            UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_POSTFIELDSIZE, _payloadData.size() ) );
            addContentType = true;
            break;

        case MethodPut:
            UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_UPLOAD, 1 ) );
            UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_READFUNCTION, CurlWrapper::curlPut ) );
            UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_READDATA, this ) );
            UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_INFILESIZE, _payloadData.size() ) );
            addContentType = true;
            break;

        case MethodDelete:
            UNTIL_CURL_ERROR ( eCode, curl_easy_setopt ( _curl, CURLOPT_CUSTOMREQUEST, "DELETE" ) );
            break;

        default:
            assert ( false );
            return Error::InvalidParameter;
    }

    if ( eCode != CURLE_OK )
    {
        LOG ( L_FATAL_ERROR, "Could not start CurlWrapper; Setting options failed; CurlCode: " << eCode );
        return Error::InternalError;
    }

    if ( addContentType && !_contentType.isEmpty() )
    {
        _curlHeaders = curl_slist_append ( _curlHeaders, String ( "Content-Type:%1" ).arg ( _contentType ).c_str() );
    }

    for ( HashMap<String, String>::Iterator it ( _headers ); it.isValid(); it.next() )
    {
        _curlHeaders = curl_slist_append ( _curlHeaders, String ( "%1:%2" ).arg ( it.key(), it.value() ).c_str() );
    }

    if ( _curlHeaders != 0 )
    {
        eCode = curl_easy_setopt ( _curl, CURLOPT_HTTPHEADER, _curlHeaders );

        if ( eCode != CURLE_OK )
        {
            curl_slist_free_all ( _curlHeaders );
            _curlHeaders = 0;
        }
    }
    else
    {
        eCode = curl_easy_setopt ( _curl, CURLOPT_HTTPHEADER, 0 );
    }

    if ( eCode != CURLE_OK )
    {
        LOG ( L_FATAL_ERROR, "Could not start CurlWrapper; Configuring headers failed; CurlCode: " << eCode );
        return Error::InternalError;
    }

    _inProgress = true;

    assert ( _curlm != 0 );

    CURLMcode mCode = _curlm->start ( this );

    if ( curlMCode != 0 )
    {
        *curlMCode = mCode;
    }

    if ( mCode == CURLM_OK )
    {
        if ( timeout > 0 )
        {
            _abortTimer.start ( timeout * 1000 );
        }

        return Error::Success;
    }

    _inProgress = false;

    return Error::InternalError;
}

void CurlWrapper::stop()
{
    assert ( _curlm != 0 );

    _curlm->stop ( this );

    if ( _curlHeaders != 0 )
    {
        curl_slist_free_all ( _curlHeaders );
        _curlHeaders = 0;
    }

    if ( _curl != 0 )
    {
        curl_easy_cleanup ( _curl );
        _curl = 0;
    }

    _headers.clear();
    _getData.clear();
    _payloadData.clear();
    _userPass.clear();
    _inProgress = false;

    _url.clear();

    _abortTimer.stop();
}

void CurlWrapper::transferFinished()
{
    assert ( _inProgress );

#ifndef NDEBUG
    LOG ( ( _curlErrMsg[ 0 ] != 0 ) ? L_ERROR : L_DEBUG2, "Curl transfer finished. Url: '" << _url
          << "' Error message: '" << _curlErrMsg << "'" );
#endif

    long httpCode = 0;

    if ( !_curl || curl_easy_getinfo ( _curl, CURLINFO_RESPONSE_CODE, &httpCode ) != CURLE_OK )
    {
        httpCode = -1;
    }

    // Let's create a new reference to the data before clearing it in stop().
    RwBuffer data ( _getData );

    stop();

    // It's ok if data here is 0, it means no data was received.
    // We have to return after calling that callback!
    _receiver.curlTransferFinished ( this, data, httpCode );
    return;
}

void CurlWrapper::timerExpired ( Timer * timer )
{
    ( void ) timer;
    assert ( timer == &_abortTimer );
    assert ( _curlm != 0 );

    _curlm->stop ( this );
    transferFinished();
}

size_t CurlWrapper::curlDataSink ( void * ptr, size_t size, size_t nmemb, void * userdata )
{
    assert ( userdata != 0 );

    CurlWrapper * curlw = ( CurlWrapper * ) userdata;

    const int dataLen = size * nmemb;

#ifndef NO_LOGGING
    if ( _log.shouldLog ( L_DEBUG3 ) )
    {
        String temp ( ( char * ) ptr, dataLen );
        LOG ( L_DEBUG3, "curlDataSink got: " << temp );
    }
#endif

    curlw->_getData.appendData ( ( char * ) ptr, dataLen );

    curlw->_receiver.curlDataReceived ( curlw, curlw->_getData, dataLen );

    if ( curlw->_maxLen != 0 && curlw->_getData.size() > curlw->_maxLen )
    {
        return 0;
    }

    return dataLen;
}

size_t CurlWrapper::curlPut ( void * ptr, size_t size, size_t nmemb, void * userdata )
{
    assert ( userdata != 0 );

    CurlWrapper * curlw = ( CurlWrapper * ) userdata;
    MemHandle & payload = curlw->_payloadData;

    const size_t maxLen = size * nmemb;

    int len = min ( maxLen, payload.size() );

    memcpy ( ptr, payload.get(), len );

    payload.consume ( len );

    return len;
}

void CurlWrapper::Receiver::curlDataReceived ( CurlWrapper *, RwBuffer &, int )
{
}
