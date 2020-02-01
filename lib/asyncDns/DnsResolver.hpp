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
#include "dns/Dns.h"
}

#include "basic/NoCopy.hpp"
#include "basic/String.hpp"
#include "basic/IpAddress.hpp"
#include "event/AsyncQueue.hpp"
#include "log/TextLog.hpp"

namespace Pravala
{
/// @brief Abstract class that performs a DNS lookup on a thread.
/// This class should only be created and used on MasClient's thread,
/// with the exception of notifyLookupComplete() which will be called on a different thread.
class DnsResolver: public NoCopy
{
    public:
        /// @brief DNS lookup for A (IPv4) records.
        static const uint8_t ReqTypeA = ( 1 << 0 );

        /// @brief DNS lookup for AAAA (IPv6) records.
        static const uint8_t ReqTypeAAAA = ( 1 << 1 );

        /// @brief DNS lookup for SRV records.
        static const uint8_t ReqTypeSRV = ( 1 << 2 );

        /// @brief When passed to start(), DNS requests will be performed using TCP instead of UDP.
        /// By default, all requests are performed using UDP.
        static const uint8_t ReqFlagUseTcp = ( 1 << 0 );

        /// @brief When passed to start() (and ReqFlagUseTcp is not), DNS requests will not be retried using TCP.
        /// By default, all requests are performed using UDP, and when the answer is truncated, they are retried
        /// using TCP protocol. Passing this flag disables that behaviour.
        static const uint8_t ReqFlagDontUseTcp = ( 1 << 1 );

        /// @brief Max allowed requst timeout (in seconds).
        static const uint16_t MaxTimeout = 120;

        /// @brief Configuration for interface/network binding.
        struct IfaceConfig
        {
            /// @brief If set, sockets created to query IPv4 DNS servers will be bound to this interface name.
            String bindToIfaceV4;

            /// @brief If set, sockets created to query IPv6 DNS servers will be bound to this interface name.
            String bindToIfaceV6;

            /// @brief If >= 0, sockets created to query DNS servers will be bound to given network ID.
            /// @note This is, at least for now, Android-specific functionality.
            int64_t bindToNetwork;

            /// @brief Default constructor.
            inline IfaceConfig(): bindToNetwork ( -1 )
            {
            }

            /// @brief Checks if at least one of the binding settings is used.
            /// @return True if at least one of the binding settings is used; False if none are used.
            inline bool isUsed() const
            {
                return ( ( bindToNetwork >= 0 ) || !bindToIfaceV4.isEmpty() || !bindToIfaceV6.isEmpty() );
            }
        };

        /// @brief Contains a single DNS SRV record.
        struct SrvRecord
        {
            String target;     ///< Target name.

            uint32_t ttl;      ///< TTL of the result, in seconds.
            uint16_t priority; ///< Priority of the record.
            uint16_t weight;   ///< Weight of the record.
            uint16_t port;     ///< Service port.

            /// @brief Default constructor.
            SrvRecord(): ttl ( 0 ), priority ( 0 ), weight ( 0 ), port ( 0 )
            {
            }

            /// @brief Checks if the record is valid.
            /// @return True if the record has a non-empty target and port number greater than 0.
            inline bool isValid() const
            {
                return ( port > 0 && !target.isEmpty() );
            }

            /// @brief Returns a string with description of this object.
            /// @return String with description of this object.
            String toString() const;
        };

        /// @brief To be inherited by the object that wants to receive notifications when name resolutions complete.
        class Owner
        {
            protected:
                /// @brief Called when both of the A and AAAA lookups succeed or fail (if both were requested).
                /// It is safe to delete the resolver inside this callback.
                /// @param [in] resolver The resolver that generated the callback.
                /// @param [in] name The name that was looked up.
                /// @param [in] results The combined list of IP results from both lookups.
                ///                     This will be an empty list if both lookups failed to return results.
                virtual void dnsLookupComplete (
                    DnsResolver * resolver, const String & name, const List<IpAddress> & results ) = 0;

                /// @brief Called when a SRV lookup is complete, on success or failure.
                /// It is safe to delete the resolver inside this callback.
                /// @param [in] resolver The resolver that generated the callback.
                /// @param [in] name The name that was looked up.
                /// @param [in] results The list of SRV records for that name.
                ///                     This will be an empty list if the lookup failed to return results.
                virtual void dnsLookupComplete (
                    DnsResolver * resolver, const String & name, const List<SrvRecord> & results ) = 0;

                /// @brief Destructor.
                virtual ~Owner()
                {
                }

                friend class DnsResolver;
        };

        /// @brief Constructor.
        /// @param [in] owner The owner that will receive the callbacks.
        DnsResolver ( Owner & owner );

        /// @brief Destructor
        ~DnsResolver();

        /// @brief Starts a DNS resolution.
        /// It will call dnsLookupComplete() in the owner once all requested lookups are complete.
        /// If there is another operation already in progress, it will be abandoned.
        ///
        /// @param [in] dnsServers The DNS servers to be used.
        /// @param [in] reqType The bitmask with the request types to run. Not all the types can be requested together.
        ///                     ReqTypeA and ReqTypeAAAA can be requested independently or together.
        ///                     ReqTypeSRV has to be requested separately.
        ///                     If (ReqTypeA | ReqTypeAAAA) is requested, a callback will be called once
        ///                     both resolutions are completed.
        /// @param [in] name The name to lookup.
        /// @param [in] flags Optional flags. A bitmask of ReqFlag* values.
        /// @param [in] ifaceConfig The interface configuration for binding requests to interfaces.
        /// @param [in] timeout Timeout for the operation, in seconds.
        ///                     If 0 or larger than MaxTimeout, then MaxTimeout value will be used instead.
        ERRCODE start (
            const HashSet<SockAddr> & dnsServers,
            uint8_t reqType,
            const String & name,
            uint8_t flags = 0,
            const IfaceConfig * ifaceConfig = 0,
            uint16_t timeout = 30 );

        /// @brief Stops DNS lookup in progress.
        /// No callbacks will called after this.
        void stop();

        /// @brief Exposes whether there is a DNS lookup in progress.
        /// @return True if there is a DNS lookup in progress; False otherwise.
        inline bool isInProgress() const
        {
            return ( _currentId > 0 );
        }

        /// @brief Get an entire DnsRecord as a string.
        /// For logging purposes.
        /// @param [in] record The DnsRecord to dump as a string.
        /// @return The entire DnsRecord as a string.
        static String getRecordDesc ( const struct DnsRecord & record );

        /// @brief Comparator used for sorting the list of SRV records.
        /// It will put the record with the highest priority (lowest priority value) in front of the list.
        /// @note This function also randomizes order of records with equal priorities,
        ///       taking into account their weights.
        /// @param [in] a The first SRV entry.
        /// @param [in] b The second SRV entry.
        /// @return True if the first record should be before the second one.
        static bool compareRecords ( const SrvRecord & a, const SrvRecord & b );

    protected:
        static TextLog _log; ///< Log stream.

    private:
        /// @brief A helper class to deliver results of a lookup on the main thread.
        class LookupCompleteTask: public AsyncQueue::Task
        {
            public:
                /// @brief Constructor.
                /// @param [in] resolver Pointer to the DnsResolver instance.
                /// @param [in] lookupId The ID of the lookup.
                /// @param [in] qType The type of DNS lookup that completed.
                /// @param [in] results The pointer to the results. Deallocated when this is destroyed.
                /// @param [in] numResults The number of results from the lookup (can be 0); -1 if there was an error.
                LookupCompleteTask (
                    DnsResolver * resolver, uint32_t lookupId, enum DnsRecordType qType,
                    struct DnsRecord * results, int numResults );

                /// @brief Destructor
                virtual ~LookupCompleteTask();

            protected:
                virtual void runTask();

            private:
                DnsResolver * const _resolver; ///< Pointer to the DnsResolver instance.

                /// @brief The pointer to the results.
                /// This is a continuous memory segment that will be deallocated using free().
                struct DnsRecord * const _results;

                const uint32_t _lookupId; ///< The ID of the DNS lookup.
                const int _numResults; ///< The number of results from the lookup (can be 0); -1 if there was an error.

                enum DnsRecordType _type; ///< The type of DNS lookup.
        };

        Owner & _owner; ///< The owner that will receive the callbacks.

        String _currentName; ///< The name currently being looked up.
        uint32_t _currentId; ///< The ID of currently running lookup.
        uint32_t _lastId;    ///< The last lookup ID used.

        /// @brief A bitmask of request types being performed.
        /// When performing multiple requests, it is used to determine what else is still running.
        uint8_t _reqType;

        /// @brief Pending results for A and AAAA lookups.
        /// It is used when both A and AAAA lookups are performed at the same time.
        HashSet<IpAddress> _pendingResults;

        /// @brief Called by LookupCompleteTask on the main thread.
        /// @param [in] id The ID of the lookup.
        /// @param [in] qType The type of DNS lookup that completed.
        /// @param [in] results The pointer to the results.
        ///                     This will be deallocated by LookupCompleteTask after this call.
        /// @param [in] numResults The number of results from the lookup (can be 0); -1 if there was an error.
        void lookupComplete ( uint32_t id, enum DnsRecordType qType, struct DnsRecord * results, int numResults );

        /// @brief The function to be run by DNS lookup threads.
        /// @param [in] ptr A pointer to user data. Should be pointing to internal ThreadConfig object.
        /// @return NULL.
        static void * threadMain ( void * ptr );
};

/// @brief Generates a string with the list of SrvRecord objects.
/// @param [in] list The list of the SrvRecord objects to describe.
/// @return The description of all the elements in the list.
String toString ( const List<DnsResolver::SrvRecord> & list );
}
