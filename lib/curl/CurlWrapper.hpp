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

#include "core/Curl.hpp"
#include "CurlMWrapper.hpp"

#define UNTIL_CURL_ERROR( errCodeVal, expression ) \
    do { \
        if ( errCodeVal == 0 ) errCodeVal = ( expression ); \
    } while ( false )

namespace Pravala
{
/// @brief Class to wrap cURL operations
///
/// Usage:
/// 1. Implement a class that inherits Receiver
/// 2. curlw.setUrl, curlw.setPostData, curlw.setHeader, etc.
/// 3. curlw.start()
class CurlWrapper: public Curl, protected Timer::Receiver
{
    public:
        /// @brief Class that should be inherited by classes receiving callbacks from CurlWrapper
        class Receiver
        {
            protected:
                /// @brief Called when cURL receives some data from the remote end
                ///
                /// buffer is a reference to this CurlWrapper's read buffer. Newly received data
                /// will be appended to it after this function when new data is received. The caller
                /// may want to clear it (for example, if they are streaming a file or downloading).
                ///
                /// @param [in] curlw CurlWrapper instance calling this function
                /// @param [in] buffer Current read buffer
                /// @param [in] bytes Number of bytes received since the last call of curlDataReceived
                /// @note CurlWrapper should NOT be destroyed inside this callback.
                virtual void curlDataReceived ( CurlWrapper * curlw, RwBuffer & buffer, int bytes );

                /// @brief Called when a cURL transfer is finished
                /// @param [in] curlw CurlWrapper instance calling this function
                /// @param [in] buf Buffer of data received (or leftover if previously cleared by curlDataReceived)
                /// This may also be empty if the request didn't return any data
                /// or if the request has failed. To verify request failure and/or to get extended
                /// error data (e.g. status line), you can enable the outputHdrs option when calling
                /// start(), and a lack of headers would then imply a failure to connect.
                /// @param [in] httpCode The HTTP status code from the last transaction, -1 if it could not be obtained.
                /// @note It is safe to destroyed the CurlWrapper object inside this callback.
                /// @todo Properly plumb through error callbacks
                virtual void curlTransferFinished ( CurlWrapper * curlw, RwBuffer & buf, long httpCode ) = 0;

                /// @brief Destructor.
                virtual ~Receiver()
                {
                }

                friend class CurlWrapper;
        };

        /// @brief Constructor.
        /// @param [in] receiver The object to receive the callbacks.
        CurlWrapper ( Receiver & receiver );

        /// @brief Constructor.
        /// @param [in] receiver The object to receive the callbacks.
        /// @param [in] mCurl Multi curl wrapper to use. Can be shared between multiple cUrl wrappers.
        CurlWrapper ( Receiver & receiver, CurlMWrapper & mCurl );

        /// @brief Destructor
        virtual ~CurlWrapper();

        /// @brief Returns true if a request is currently in progress
        /// @return true if a request is currently in progress
        inline bool isInProgress() const
        {
            return _inProgress;
        }

        /// @brief Set the maximum transfer length
        /// Transfer will stop after it has exceeded this length.
        /// This can be set while the transfer is in progress.
        /// @param [in] maxLen Maximum length, 0 for no limit
        inline void setMaxTransferLength ( size_t maxLen )
        {
            _maxLen = maxLen;
        }

        /// @brief Sets some data to upload if the method is POST or PUT
        /// @param [in] data Data to upload if method is POST or PUT
        /// @param [in] contentType The content type to use (optional).
        /// @return True if value was set, false if request in progress
        bool setPayload ( const String & data, const String & contentType = String() );

        /// @brief Sets some data to upload if the method is POST or PUT
        /// @param [in] data Data to upload if method is POST or PUT
        /// @param [in] contentType The content type to use (optional).
        /// @return True if value was set, false if request in progress
        bool setPayload ( const MemHandle & data, const String & contentType = String() );

        /// @brief Sets the user agent
        /// @param [in] userAgent User agent to use
        /// @return True if value was set, false if request in progress
        bool setUserAgent ( const String & userAgent );

        /// @brief Set a header field
        /// @param [in] headerKey The key (left side of colon) of the header
        /// @param [in] headerValue The value (right side of colon) of the header
        /// @return True if value was set, false if request in progress
        bool setHeader ( const String & headerKey, const String & headerValue );

        /// @brief Sets a username/password pair to use
        /// @param [in] user Username to use
        /// @param [in] pass Password to u se
        /// @return True if value was set, false if request in progress
        bool setUserPass ( const String & user, const String & pass );

        /// @brief Enables or disables CURLOPT_FOLLOWLOCATION option.
        /// @param [in] follow If true, CURLOPT_FOLLOWLOCATION will be enabled; Otherwise it will be disabled).
        ///                    By default following is disabled.
        void setFollowLocation ( bool follow );

        /// @brief Sets a timeout value (in seconds) for the entire request.
        /// @param [in] timeout A timeout value (in seconds) for the entire request. No timeout if this is <= 0.
        void setTimeout ( long timeout );

        /// @brief Starts cURL transaction
        /// @param [in] url URL to access
        /// @param [in] method HTTP Method to use
        /// @param [in] timeout Maximum request time in seconds, 0 to not time out
        /// After this amount of time, the request is aborted and curlTransferFinished is called.
        /// @param [in] outputHdrs If true, include headers in the return payload; if false, do not
        /// @param [out] curlMCode CURLMCode result, if set to CURLM_LAST and return is InternalError,
        /// This means that curl_easy_init failed.
        /// @return Standard error code
        ERRCODE start (
            const String & url, HttpMethod method = MethodGet, uint32_t timeout = 0,
            bool outputHdrs = false, CURLMcode * curlMCode = 0 );

        /// @brief Stops cURL transaction and resets all of the data structures
        void stop();

    protected:
        /// @brief Creates the cURL easy handle for the transaction.
        /// The default implementation simply calls curl_easy_init().
        /// @return A CURL easy handle on success, or 0 on any error.
        virtual CURL * initCurlEasyHandle();

    private:
        static TextLog _log; ///< Logging stream

        Receiver & _receiver; ///< Receives callbacks from this CurlWrapper

        CurlMWrapper * _curlm; ///< This wrapper's multi-wrapper.

        /// @brief Maximum time for this transfer
        /// This is primarily to work around issues where cURL doesn't time out on its own when
        /// it is stuck waiting for DNS (even with c-ares DNS).
        SimpleTimer _abortTimer;

        /// @brief cURL Easy Handle
        CURL * _curl;

        /// @brief Headers to transmit
        /// Can be 0 if there are no extra/special headers
        struct curl_slist * _curlHeaders;

        String _userAgent; ///< User agent to use

        String _url; ///< URL to access

        HashMap<String, String> _headers; ///< Headers <String headerKey, String headerValue>

        /// @brief Authentication information in the form of "username:password"
        /// If set, authentication will be enabled.
        String _userPass;

        /// @brief Data to upload if the request is a POST or PUT
        MemHandle _payloadData;

        /// @brief Content type to use for _payloadData.
        String _contentType;

        /// @brief Data received from the remote host
        /// 0 if no data has been received yet
        RwBuffer _getData;

        /// @brief Maximum number of bytes of data to transfer
        size_t _maxLen;

        long _timeout; ///< Timeout (in seconds) for the entire request. No timeout if this is <= 0.
        bool _inProgress; ///< True if a request is in progress
        bool _doFollow; ///< Whether CURLOPT_FOLLOWLOCATION should be set to 1.

        /// @brief When set to true, the multi wrapper was created by this object and will be removed in the destructor.
        /// This is used to distinguish between external (and potentially shared) multi wrapper,
        /// and one that was created in the this wrapper's constructor.
        bool _privateCurlM;

#ifndef NDEBUG
        /// @brief Curl error message buffer
        char _curlErrMsg[ CURL_ERROR_SIZE ];
#endif

        /// @brief Called by cURL when data is received
        /// @param [in] ptr Pointer containing data (len = size*nmemb)
        /// @param [in] size block size
        /// @param [in] nmemb number of blocks
        /// @param [in] userdata pointer to the owning CurlWrapper object
        /// @return Number of bytes processed, should be size*nmemb, other values indicate error
        /// e.g. read buffer size > _maxLen
        static size_t curlDataSink ( void * ptr, size_t size, size_t nmemb, void * userdata );

        /// @brief Called by cURL when it needs to put some data
        /// cURL handles POST internally
        /// @param [in] ptr Pointer to write data to (len = size*nmemb)
        /// @param [in] size block size
        /// @param [in] nmemb number of blocks
        /// @param [in] userdata pointer to the owning CurlWrapper object
        /// @return Number of bytes processed written to ptr
        static size_t curlPut ( void * ptr, size_t size, size_t nmemb, void * userdata );

        /// @brief Called by CurlMWrapper when the transfer is complete
        void transferFinished();

        /// @brief Timer callback in the object.
        /// @param [in] timer The pointer to the timer that expired
        void timerExpired ( Timer * timer );

        friend class CurlMWrapper;
};
}
