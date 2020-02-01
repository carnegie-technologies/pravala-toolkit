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

#include "curl/CurlWrapper.hpp"

namespace Pravala
{
/// @brief Used for testing curl by retrieving NCSI
class CurlTest: public CurlWrapper::Receiver
{
    public:
        /// @brief Constructor
        CurlTest();

        /// @brief Destructor
        ~CurlTest();

        /// @brief Start CurlTest
        /// @param [in] url The URL to retrieve
        /// @return Standard error code
        ERRCODE start ( const String & url );

    protected:
        virtual void curlTransferFinished ( CurlWrapper * curlw, RwBuffer & buf, long httpCode );

    private:
        /// @brief CurlWrapper used by this object
        CurlWrapper _curl;
};
}
