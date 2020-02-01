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

#include "DnsInternal.h"

#ifdef __cplusplus
extern "C" {
#endif

// We need to make public DNS fields visible to others.
// DNS_PUBLIC is used by dnsc/src/dns.h to make their public API visible if it is hidden by default.
// TODO: We may be able to avoid doing that by moving more functionality into LibDns itself.
//       This will need to include at least parts of vtunsocket/VDns and most (all?) of VDnsCache.
//       For now, we just make public DNS API visible.
#define DNS_PUBLIC    PRAV_PUBLIC_API

#include "dnsc/src/dns.h"

#if ( !defined __clang__ ) && ( __GNUC__ >= 9 )
// gcc >= 9 does not need dns_quietinit workaround that is used in dns.h.
// That workaround, however, creates its own build issues.
// So when building with gcc >= 9, we need to replace it:
#undef dns_quietinit
#define dns_quietinit( ... )    __VA_ARGS__
#endif

/// @brief Free the DNS packet.
/// This function is used to expose dns.c's internal functionality that is not public (but it should be).
/// @param [in] p The packet to deallocate.
void dnsPacketFree ( struct dns_packet * p );

#ifdef __cplusplus
}
#endif
