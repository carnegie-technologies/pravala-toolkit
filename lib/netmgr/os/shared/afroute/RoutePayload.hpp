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
#include <net/route.h>
}

#include "basic/IpAddress.hpp"
#include "basic/Buffer.hpp"
#include "log/TextLog.hpp"

namespace Pravala
{
/// @brief A wrapper class to access fields from an AF_ROUTE response message/network sysctl output data.
/// The format of the AF_ROUTE reply message (and also of BSD sysctl call returns, they have the same format)
/// is documented in UNIX Network Programming, Vol 1, 3rd Edition, by Stevens, pg 492.
///
/// The response message has a well-defined format with fields which may or may not be present,
/// and the format of any given field can depend on data contained in the field.
/// This class does not attempt to extract each field, it simplifies accessing each field. Understanding
/// which type of field is present, and whether it is valid in the context of other received fields, is left
/// up to the caller of this class.
class RoutePayload
{
    public:
        /// @brief Possible fields contained in the response message. Not all are likely to exist.
        enum Field
        {
            FieldDestination = RTAX_DST, ///< The destination address of a route.
            FieldGateway = RTAX_GATEWAY, ///< The gateway of a route. May be an IP or LINK structure.
            FieldNetmask = RTAX_NETMASK, ///< The netmask of the message.
            FieldGenmask = RTAX_GENMASK, ///< The cloning mask of the message.
            FieldInterface = RTAX_IFP, ///< The interface details in the message (LINK, not IP)
            FieldInterfaceAddress = RTAX_IFA, ///< The interface address.
            FieldAuthor = RTAX_AUTHOR, ///< The author of the message.
            FieldBroadcast = RTAX_BRD, ///< The broadcast addr or p2p addr in a NEWADDR message.
            FieldMax = RTAX_MAX ///< The size of the RTAX array (i.e. this value is not valid for comparisons).
        };

        /// @brief Constructor.
        RoutePayload();

        /// @brief Setup this class based on a given payload structure.
        /// This creates a copy of the message payload which is stored in this class;
        /// so it can be accessed even after the message is freed.
        /// @param [in] fieldsPresent A bitmask of fields which are present in the payload. See rt_msghdr.rtm_addrs.
        /// @param [in] payload The payload to parse (i.e. first byte after the header). This object will hold a ref
        /// to the memory until the RoutePayload object goes out of scope, or parse is called again.
        /// @return True if the payload was successfully parsed; false on any error.
        bool setup ( int fieldsPresent, const MemHandle & payload );

        /// @brief Expose whether this object is usable.
        /// @return True if a payload has been successfully parsed; false otherwise.
        inline bool isValid() const
        {
            return _isValid;
        }

        /// @brief Expose whether a given field is present.
        /// @param [in] field The field to check for
        /// @return True if the field exists; false otherwise.
        inline bool contains ( Field field )
        {
            return ( _isValid && ( field < FieldMax ) && ( _ai.rti_addrs & ( 1 << field ) ) != 0 );
        }

        /// @brief Expose whether a given field is an IP-based object.
        /// @param [in] field The field to check.
        /// @return True if the field has an sa_family of AF_INET or AF_INET6; false otherwise.
        inline bool isFieldIp ( Field field )
        {
            return ( contains ( field ) && ( _ai.rti_info[ field ] != 0 )
                     && ( _ai.rti_info[ field ]->sa_family == AF_INET
                          || _ai.rti_info[ field ]->sa_family == AF_INET6 ) );
        }

        /// @brief Expose whether a given field is a LINK object.
        /// @param [in] field The field to check.
        /// @return True if the field has an sa_family of AF_LINK; false otherwise.
        inline bool isFieldLink ( Field field )
        {
            return ( contains ( field )
                     && ( _ai.rti_info[ field ] != 0 )
                     && ( _ai.rti_info[ field ]->sa_family == AF_LINK ) );
        }

        /// @brief Attempt to retrieve an IP address from the specified field.
        /// @param [in] field The field to retrieve the address from.
        /// @param [out] addr The parsed address if true is returned; not modified if false returned.
        /// @return True on success; false on failure. This will fail if the field doesn't store an IP address,
        /// or if the field is not present.
        bool getAddress ( Field field, IpAddress & addr );

        /// @brief Attempt to retrieve a netmask from the specified field.
        /// @param [in] field The field to retrieve the address from (one of Netmask or Genmask; others are invalid).
        /// @param [out] addr The parsed network address if true is returned; not modified if false returned.
        /// @return True on success; false on failure.  This will fail if the field doesn't store a subnet mask,
        /// or if the field is not present.
        bool getNetmask ( Field field, IpAddress & addr );

        /// @brief Attempt to retrieve the interface name from the specified field.
        /// @param [in] field The field to retrieve the name from (only valid if the field is an AF_LINK object).
        /// @param [out] str The parsed name if true is returned; not modified if false is returned.
        /// @return True on success; false on failure. This will fail if the field doesn't store a name,
        /// or if the field is not present.
        bool getName ( Field field, String & str );

        /// @brief Attempt to retrieve the interface link layer address from the specified field.
        /// @param [in] field The field to retrieve the lladdr from (only valid if the field is an AF_LINK object).
        /// @param [out] str The parsed addr string if true is returned; not modified if false is returned.
        /// @return True on success; false on failure. This will fail if the field doesn't store a link layer address,
        /// or if the field is not present.
        bool getLLAddr ( Field field, String & lladdr );

    private:
        static TextLog _log; ///< Log stream.

        bool _isValid; ///< True if the payload was parsed properly and _ai is properly set up; false otherwise.

        /// @brief Retrieving fields from a route message is done by indexing into the payload.
        /// The pointers to each field type are stored here, in _ai. Pointers are set from _ai
        /// to the corresponding location in _payload. These pointers are setup during a call to setup().
        struct rt_addrinfo _ai;

        MemHandle _payload; ///< The payload data. If this is cleared or unset, _ai must also be cleared.
};
}
