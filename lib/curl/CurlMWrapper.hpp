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

#pragma once

extern "C"
{
#include <curl/curl.h>
}

#include "log/TextLog.hpp"
#include "event/EventManager.hpp"
#include "event/Timer.hpp"

namespace Pravala
{
class CurlWrapper;

/// @brief Wraps curl_multi_* operations
/// Each CurlWrapper class needs to work within a context of a CurlMWrapper.
/// A single CurlMWrapper can be explicitly created and shared by multiple CurlWrapper objects.
/// If a multi wrapper is not provided while constructing a CurlWrapper,
/// an internal version will automatically be created and used only by that single CurlWrapper.
class CurlMWrapper: public Timer::Receiver, public EventManager::FdEventHandler
{
    public:
        /// @brief Default constructor.
        CurlMWrapper();

        /// @brief Destructor
        ~CurlMWrapper();

    protected:
        /// @brief Timer callback in the object.
        /// Alarm callback to handle cURL timer expiry
        /// @param [in] timer The pointer to the timer that expired
        virtual void timerExpired ( Timer * timer );

        /// @brief EventManager - receiveFdEvent
        /// Handles events on cURL FDs
        /// @param [in] fd File descriptor that genereted this event.
        /// @param [in] events A bit sum of Event* values and describes what kind of events were detected.
        virtual void receiveFdEvent ( int fd, short events );

    private:
        static TextLog _log; ///< Logging stream

        SimpleTimer _multiTimer; ///< Timer that handles cURL's internal timer requests

        CURLM * _curlm; ///< cURL multi handle

        int _running; ///< Active cURL easy handles running

        /// @brief HashMap of <CURL *, CurlWrapper *>
        /// Used to map a cURL callback to the correct CurlWrapper instance
        HashMap<CURL *, CurlWrapper *> _curlWrapperMap;

        /// @brief Starts cURL transaction
        /// This is NOT safe to call on a CurlWrapper instance that is already started.
        /// @param [in] curlWrapper CurlWrapper instance to start, this cannot be null.
        /// Also curlWrapper->_curl cannot be null.
        /// @return CURLMCode result
        CURLMcode start ( CurlWrapper * curlWrapper );

        /// @brief Stops a cURL transaction
        /// This is safe to call on a CurlWrapper instance that is already stopped.
        /// @param [in] curlWrapper CurlWrapper instance to stop
        void stop ( CurlWrapper * curlWrapper );

        /// @brief Cleans up easy handles in cURL multi
        /// Will only do anything if _running == 0 and _curlWrapperMap is empty
        void curlCleanup();

        /// @brief Called when cURL needs to set/update its timer
        /// This can be called as a result of curl_multi_add_handle (to create a new connection) or
        /// curl_multi_socket_action
        /// @param [in] multi CURLM that wants this timer
        /// @param [in] timeout_ms Timeout in ms
        /// @param [in] userp pointer to the owning Wispr object
        static int curlMTimerCallback ( CURLM * multi, long timeout_ms, void * userp );

        /// @brief Called when cURL needs events from a socket
        /// This can be called as a result of curl_multi_add_handle (to create a new connection) or
        /// curl_multi_socket_action
        /// @param [in] easy CURL (easy) that owns this socket
        /// @param [in] s FD of socket
        /// @param [in] what CURL_POLL_{NONE,IN,OUT,INOUT,REMOVE}
        /// IN = read, OUT = write, NONE = sock exists, but don't care about r/w
        /// @param [in] userp pointer to the owning Wispr object
        /// @param [in,out] socketp not used
        static int curlMSocketCallback ( CURL * easy, curl_socket_t s, int what, void * userp, void * socketp );

        /// @brief Get cURL messages (figures out what finished and lets the callback know)
        void curlGetMsgs();

        friend class CurlWrapper;
};
}
