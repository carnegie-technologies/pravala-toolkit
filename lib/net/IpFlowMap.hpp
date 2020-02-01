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

#include "basic/NoCopy.hpp"
#include "IpFlow.hpp"

namespace Pravala
{
/// @brief A hash map for storing IpFlow objects.
/// @note This only allocates the memory for the first level of the hash map, and provides the getIndex() method.
///       The second level is done by the IpFlow object and, at least for now, is implemented as a linked list.
class IpFlowMap: public NoCopy
{
    public:
        /// @brief Controls whether expiration status is checked while looking up flows.
        enum ExpiryMode
        {
            DontExpireFlows, ///< Flows will not be automatically expired while performing look-ups.

            /// @brief When performing look-ups, flows' expiration status is checked.
            /// If any flow in the same bucket is expired, the entire bucket is cleaned up.
            ExpireFlows
        };

        /// @brief Constructor.
        /// @param [in] bitSize The size of the FlowMap in BITS.
        ///                     Value '10' means 10 bits, and represents map with 1024 entries.
        ///                     Allowed range: 8 - 30. On 64 bit value 24 will use 128MB of memory; 28 will use 2GB.
        IpFlowMap ( uint8_t bitSize );

        /// @brief Destructor.
        /// It deallocates the map, which should have been emptied
        /// (and filled with zeroes) by the time this gets called!
        ~IpFlowMap();

        /// @brief Checks if the flow map is empty.
        /// @return True if the flow map is empty; False otherwise.
        inline bool isEmpty() const
        {
            return ( _usedBuckets < 1 );
        }

        /// @brief Clears the flow map.
        /// It will send 'SignalRemove' to all the flows.
        void clearMap();

        /// @brief Returns a pointer to IP flow object matching given descriptor.
        /// @param [in] flowDesc The flow descriptor to match against.
        /// @param [in] descType The type of the descriptor to match against.
        /// @param [in] expMode Whether flows should be automatically expired. By default they are expired.
        /// @return IP flow object that represents given descriptor, or 0 if it was not found in the map
        IpFlow * findFlow ( const FlowDesc & flowDesc, uint8_t descType, ExpiryMode expMode = ExpireFlows );

        /// @brief Returns a pointer to IP flow object matching given descriptor.
        /// @param [in] flowDesc The flow descriptor to match against.
        /// @param [in] descType The type of the descriptor to match against.
        /// @return IP flow object that represents given descriptor, or 0 if it was not found in the map
        const IpFlow * findFlow ( const FlowDesc & flowDesc, uint8_t descType ) const;

    protected:
        const uint8_t _bitSize; ///< The bit size of the map
        const uint32_t _mapSize; ///< The size of the map
        const uint32_t _bitMask; ///< The bitmask to apply to hash values for XOR-folding

        IpFlow ** const _flows; ///< Content of the map.

        /// @brief Returns the index of the hash bucket that should be used for particular FlowDesc.
        /// @param [in] flowDesc The FlowDesc to find.
        /// @return Index to the hash bucket to be used for this FlowDesc.
        inline uint32_t getIndex ( const FlowDesc & flowDesc ) const
        {
            const uint32_t hash = flowDesc.getHash();

            // 32 bits is too much for sure! Let's use XOR-folding:
            return ( ( ( hash >> _bitSize ) ^ hash ) & _bitMask );
        }

        /// @brief Returns the number of used flow buckets.
        /// This is NOT the number of flow objects stored.
        /// @return The number of used flow buckets.
        inline uint32_t getNumUsedBuckets() const
        {
            return _usedBuckets;
        }

        /// @brief Sends 'SignalCleanup' to each flow in the given bucket.
        /// @param [in] index The index to check. MUST be < _mapSize.
        void cleanupFlows ( uint32_t index );

        /// @brief Inserts given IP flow into the map.
        /// Inserting flow for the second time will succeed.
        /// If this returns false, it means that the flow is not part of the map.
        /// @param [in] flow The IP flow to insert.
        /// @return True if it has been successfully inserted; False otherwise.
        inline bool insertFlow ( IpFlow * flow )
        {
            return ( flow != 0 && flow->mapInsert ( *this ) );
        }

        /// @brief Removes given IP flow the map.
        /// This does NOT destroy the object itself.
        /// @param [in] flow The IP flow to remove from the map.
        inline void removeFlow ( IpFlow * flow )
        {
            if ( flow != 0 )
            {
                flow->mapRemove ( *this );
            }
        }

    private:
        /// @brief The number of non-empty flow buckets.
        /// This is NOT the number of flow objects, this is the number of buckets that have at least one flow in them.
        uint32_t _usedBuckets;

        /// @brief Internal function to insert flows into the map.
        /// It is safe to call this even if the flow is already a part of the map.
        /// @param [in] flow The flow object to insert.
        /// @param [in] flowDesc The flow descriptor to use for this flow.
        /// @return True if the flow was inserted into the map (or was already a part of it);
        ///         False if there already is a different flow using the same descriptor,
        ///         or the arguments were invalid.
        bool flowInsert ( IpFlow * flow, const FlowDesc & flowDesc );

        /// @brief Internal to remove flows from the map.
        /// @param [in] flow The flow object being removed.
        /// @param [in] flowDesc The flow descriptor to use while locating the flow.
        void flowRemove ( IpFlow * flow, const FlowDesc & flowDesc );

        friend class IpFlow;
        friend class DualIpFlow;
};
}
