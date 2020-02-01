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

#include "CurlMWrapper.hpp"
#include "CurlWrapper.hpp"
#include "core/EasyCurl.hpp"

using namespace Pravala;

TextLog CurlMWrapper::_log ( "curl_mwrapper" );

CurlMWrapper::CurlMWrapper():
    _multiTimer ( *this ),
    _curlm ( 0 ),
    _running ( 0 )
{
    assert ( _curlWrapperMap.size() == 0 );
    assert ( !_curlm );

    EasyCurl::globalInit();

    _curlm = curl_multi_init();

    if ( !_curlm )
    {
        LOG ( L_ERROR, "curl_multi_init() failed" );
        return;
    }

    CURLMcode mErr = CURLM_OK;

    UNTIL_CURL_ERROR ( mErr, curl_multi_setopt ( _curlm, CURLMOPT_SOCKETFUNCTION, CurlMWrapper::curlMSocketCallback ) );
    UNTIL_CURL_ERROR ( mErr, curl_multi_setopt ( _curlm, CURLMOPT_SOCKETDATA, this ) );

    UNTIL_CURL_ERROR ( mErr, curl_multi_setopt ( _curlm, CURLMOPT_TIMERFUNCTION, CurlMWrapper::curlMTimerCallback ) );
    UNTIL_CURL_ERROR ( mErr, curl_multi_setopt ( _curlm, CURLMOPT_TIMERDATA, this ) );

    if ( mErr != CURLM_OK )
    {
        LOG ( L_ERROR, "One of the curl_multi_setopt() calls failed" );

        curl_multi_cleanup ( _curlm );
        _curlm = 0;
        return;
    }
}

CurlMWrapper::~CurlMWrapper()
{
    curl_multi_cleanup ( _curlm );
    _curlm = 0;
}

int CurlMWrapper::curlMTimerCallback ( CURLM * multi, long timeout_ms, void * userp )
{
    ( void ) multi;
    assert ( multi != 0 );

    if ( timeout_ms >= 0 )
    {
        // Timeout < 0 means "delete the timer".
        // We keep it as a field in CurlMWrapper, so there is nothing to delete.

        CurlMWrapper * const curlm = ( CurlMWrapper * ) userp;

        assert ( curlm != 0 );

        curlm->_multiTimer.start ( timeout_ms );
    }

    return 0;
}

int CurlMWrapper::curlMSocketCallback (
        CURL * easy, curl_socket_t s, int what,
        void * userp, void * /* socketp */ )
{
    ( void ) easy;
    assert ( easy != 0 );

    int events = 0;

    switch ( what )
    {
        case CURL_POLL_IN:
            LOG ( L_DEBUG4, "Set fd read" );
            events = EventManager::EventRead;
            break;

        case CURL_POLL_OUT:
            LOG ( L_DEBUG4, "Set fd write" );
            events = EventManager::EventWrite;
            break;

        case CURL_POLL_INOUT:
            LOG ( L_DEBUG4, "Set fd read/write" );
            events = EventManager::EventRead | EventManager::EventWrite;
            break;

        case CURL_POLL_REMOVE:
            LOG ( L_DEBUG4, "Removing FdHandler for fd = " << s );
            EventManager::removeFdHandler ( s );
            return 0;

        case CURL_POLL_NONE:
        default:
            break;
    }

    LOG ( L_DEBUG4, "Adding FdHandler for fd = " << s );

    CurlMWrapper * const curlm = ( CurlMWrapper * ) userp;

    assert ( curlm != 0 );

    EventManager::setFdHandler ( s, curlm, events );
    return 0;
}

void CurlMWrapper::timerExpired ( Timer * timer )
{
    ( void ) timer;
    assert ( timer == &_multiTimer );

    CURLMcode mErr;

    do
    {
        mErr = curl_multi_socket_action ( _curlm, CURL_SOCKET_TIMEOUT, 0, &_running );
    }
    while ( mErr == CURLM_CALL_MULTI_PERFORM );

    curlGetMsgs();

    if ( _running == 0 )
    {
        curlCleanup();
    }
}

void CurlMWrapper::receiveFdEvent ( int fd, short events )
{
    int ev_bitmask = 0;

    // We need to handle any FD errors like reads so that we detect the failure and clean up the request.
    if ( ( events & EventManager::EventRead ) != 0 )
    {
        ev_bitmask |= CURL_CSELECT_IN;
    }

    if ( ( events & EventManager::EventWrite ) != 0 )
    {
        ev_bitmask |= CURL_CSELECT_OUT;
    }

    assert ( ev_bitmask != 0 );

    CURLMcode mErr;

    do
    {
        mErr = curl_multi_socket_action ( _curlm, fd, ev_bitmask, &_running );
    }
    while ( mErr == CURLM_CALL_MULTI_PERFORM );

    curlGetMsgs();

    if ( _running == 0 )
    {
        curlCleanup();
    }
}

void CurlMWrapper::curlGetMsgs()
{
    int queueLen = -1;
    CURLMsg * msg = 0;

    LOG ( L_DEBUG4, "called" );

    while ( ( msg = curl_multi_info_read ( _curlm, &queueLen ) ) != 0 )
    {
        LOG ( L_DEBUG4, "got message, type = " << msg->msg );

        if ( msg->msg != CURLMSG_DONE ) // we only care about DONE messages
            continue;

        LOG ( L_DEBUG3, "got DONE message" );

        if ( _curlWrapperMap.contains ( msg->easy_handle ) )
        {
            LOG ( L_DEBUG3, "matched easyHandle, calling callback" );

            CurlWrapper * curlw = _curlWrapperMap[ msg->easy_handle ];

            stop ( curlw );

            curlw->transferFinished();
        }
        else
        {
            LOG ( L_DEBUG3, "easyHandle not found" );
        }
    }
}

void CurlMWrapper::curlCleanup()
{
    if ( _running == 0 && _curlWrapperMap.isEmpty() )
    {
        // only do stuff if nothing is running

        // get all the messages first
        curlGetMsgs();

        _multiTimer.stop();
    }
}

CURLMcode CurlMWrapper::start ( CurlWrapper * curlWrapper )
{
    assert ( curlWrapper != 0 );
    assert ( curlWrapper->_curl != 0 );

    CURLMcode mCode = curl_multi_add_handle ( _curlm, curlWrapper->_curl );

    if ( mCode == CURLM_OK )
    {
        assert ( !_curlWrapperMap.contains ( curlWrapper->_curl ) );

        _curlWrapperMap[ curlWrapper->_curl ] = curlWrapper;
    }

    return mCode;
}

void CurlMWrapper::stop ( CurlWrapper * curlWrapper )
{
    if ( !curlWrapper || !curlWrapper->_curl )
    {
        return;
    }

    CURLMcode mErr = curl_multi_remove_handle ( _curlm, curlWrapper->_curl );

    if ( mErr != CURLM_OK && mErr != CURLM_BAD_EASY_HANDLE )
    {
        LOG ( L_ERROR, "cURL multi handle cleanup error: " << mErr );
    }

    _curlWrapperMap.remove ( curlWrapper->_curl );
}
