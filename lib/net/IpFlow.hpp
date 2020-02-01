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

#include "net/FlowDesc.hpp"

namespace Pravala
{
class IpPacket;
class IpFlowMap;

/// @brief A base class describing an IP flow stored in IpFlowMap.
class IpFlow
{
    public:
        /// @brief The type of the descriptor used for matching against this object's flow descriptor.
        static const uint8_t DefaultDescType = 0;

        /// @brief Exposes the default flow descriptor.
        /// @return The default flow descriptor.
        inline const FlowDesc & getDefaultFlowDesc() const
        {
            return _flowDesc;
        }

        /// @brief Returns ID used for logging.
        /// @return ID for logging.
        virtual String getLogId() const = 0;

        /// @brief Checks if this IP flow matches given descriptor.
        /// @param [in] flowDesc The flow descriptor to check. If it is invalid, this function will always return false.
        /// @param [in] descType The type of the descriptor. All flows have at least one descriptor,
        ///                      some flows can have more. This type determines which descriptor to match against.
        ///                      If a flow object doesn't support the type specified, it will match against the default
        ///                      type (=DefaultDescType).
        virtual bool matchFlow ( const FlowDesc & flowDesc, uint8_t descType = DefaultDescType ) const;

        /// @brief Passes an IP packet to a flow.
        /// @note The flow object may be removed inside this call!
        /// @param [in] packet Received IP packet.
        /// @param [in] userData Optional user-specific data.
        /// @param [in] userPtr Optional user-specific pointer.
        /// @return Standard error code.
        virtual ERRCODE packetReceived ( IpPacket & packet, int32_t userData = 0, void * userPtr = 0 ) = 0;

    protected:
        /// @brief Default constructor.
        IpFlow();

        /// @brief Constructor.
        /// @param [in] flowDesc The default flow descriptor to set.
        IpFlow ( const FlowDesc & flowDesc );

        /// @brief Destructor.
        virtual ~IpFlow();

        /// @brief Returns the next IP flow object in the list.
        /// @return The next IP flow object in the list.
        inline const IpFlow * getNext() const
        {
            return _next;
        }

        /// @brief Configures flow's default descriptor.
        /// @warning This is potentially dangerous function.
        ///          Flow descriptor must NOT be changed when the IpFlow object is a part of an IpFlowMap.
        ///          This function does not verify that's the case, it's up to the caller to ensure that.
        /// @param [in] flowDesc The flow descriptor to set.
        void setDefaultFlowDesc ( const FlowDesc & flowDesc );

        /// @brief Checks if this flow conflicts with another flow.
        /// Conflicts are determined on descriptors used by the flows.
        /// @note To be sure, two checks need to be done: A->conflictsWith(B) and B->conflictsWith(A).
        /// @param [in] otherFlow The flow to compare against.
        /// @return True if this flows conflicts with the other one; False otherwise.
        virtual bool conflictsWith ( IpFlow * otherFlow );

        /// @brief Inserts this IP flow into the given IP flow map.
        /// Inserting flow for the second time will succeed.
        /// If this returns false, it means that the flow is not part of the map.
        /// @param [in] flowMap The IP flow map to insert this object into.
        /// @return True if it has been successfully inserted; False otherwise.
        virtual bool mapInsert ( IpFlowMap & flowMap );

        /// @brief Removes this IP flow from the given IP flow map.
        /// This does NOT destroy the object itself.
        /// @param [in] flowMap The IP flow map to remove this object from.
        virtual void mapRemove ( IpFlowMap & flowMap );

        /// @brief Called when the flow is removed from the map.
        /// This could happen due to a cleanup, after the flow returns true from isExpired(),
        /// or when the map is being cleared or destroyed.
        /// It is called when the flow is no longer a part of the map.
        /// The flow can choose to destroy itself.
        /// Default implementation does nothing.
        /// @warning This must NOT remove from the map any other flows.
        virtual void flowRemoved();

        /// @brief Checks if the flow is expired and should be removed.
        /// Default implementation returns 'false'.
        /// @return True if the flow is expired and should be removed
        ///         (in which case a call to 'flowRemove' will follow); False otherwise.
        virtual bool isExpired();

    private:
        FlowDesc _flowDesc; ///< The flow descriptor for this object.
        IpFlow * _next; ///< Pointer to the next IpFlow object in the same hash bucket.

        /// @brief Returns the next IP flow object in the list.
        /// @return The next IP flow object in the list.
        inline IpFlow * getNext()
        {
            return _next;
        }

        /// @brief Clears the "next" pointer in this object, and returns its previous content.
        /// @return The next IP flow object in the list (which is no longer stored in this object).
        inline IpFlow * stealNext()
        {
            IpFlow * const next = _next;
            _next = 0;

            return next;
        }

        /// @brief Sets the 'next' pointer.
        /// It only works if current 'next' pointer is unset.
        /// @param [in] next The pointer to set.
        /// @return True if the next has been modified (even if set to 0);
        ///         False if this object already has a valid 'next' pointer (which was not modified).
        inline bool setNext ( IpFlow * next )
        {
            if ( !_next )
            {
                _next = next;
                return true;
            }

            return false;
        }

        friend class DualIpFlow;
        friend class IpFlowMap;
};
}
