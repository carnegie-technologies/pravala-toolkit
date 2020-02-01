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

#include "curl/CurlWrapper.hpp"

namespace Pravala
{
/// @brief Class to wrap cURL operations using sockets bound to a specific interface.
/// @see CurlWrapper for the usage of this class.
class BoundCurlWrapper: public CurlWrapper
{
    public:
        /// @brief Constructor.
        /// @param [in] receiver The receiver that should receive callbacks from the BoundCurlWrapper.
        BoundCurlWrapper ( Receiver & receiver );

        /// @brief Constructor.
        /// @todo TODO: Check if bound curl wrapper is compatible with shared multi-curl wrapper (shared between
        ///             wrappers bound to different interfaces). If not, then this constructor should be removed.
        /// @param [in] receiver The receiver that should receive callbacks from the BoundCurlWrapper.
        /// @param [in] mCurl Multi curl wrapper to use. Can be shared between multiple cUrl wrappers.
        BoundCurlWrapper ( Receiver & receiver, CurlMWrapper & mCurl );

        /// @brief Destructor.
        virtual ~BoundCurlWrapper();

        /// @brief Get the interface name that this curl wrapper will bind sockets to.
        /// @return The interface name to bind curl sockets to.
        inline const String & getBindIfaceName() const
        {
            return _bindIfaceName;
        }

        /// @brief Set the interface name that this curl wrapper will bind sockets to.
        ///        This will only be applied the next time this curl wrapper is started;
        ///        it will not apply to the currently running request.
        /// @param [in] ifaceName The interface name to bind curl sockets to.
        ///                       This can be an empty string to disable binding.
        void setBindIfaceName ( const String & ifaceName );

    protected:
        /// @brief Creates a CURL easy handle and configures it to bind sockets to the configured interface name
        /// before connecting.
        ///
        /// The following CURLOPTs may NOT be set on any handles returned by this function,
        /// as doing so will break the bound socket functionality:
        ///   CURLOPT_GETADDRINFOFUNCTION,
        ///   CURLOPT_FREEADDRINFOFUNCTION,
        ///   CURLOPT_SETSOCKOPTFUNCTION,
        ///   CURLOPT_OPENSOCKETFUNCTION,
        ///   CURLOPT_OPENSOCKETDATA,
        ///   CURLOPT_SOCKOPTFUNCTION
        ///
        /// @return A CURL easy handle on success, or 0 on any error.
        virtual CURL * initCurlEasyHandle();

    private:
        String _bindIfaceName; ///< Interface name to bind curl sockets to.
};
}
