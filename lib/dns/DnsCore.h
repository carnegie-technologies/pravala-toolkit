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

#include <sys/types.h>
#include <sys/socket.h>

struct dns_packet;

/// @brief A close() wrapper.
/// @param [in] fd Socket to close.
/// @return Same as native close()
int dnsw_close ( int fd );

/// @brief A send() wrapper.
/// @param [in] sockfd Socket to use.
/// @param [in] buf Pointer to the buffer to send
/// @param [in] len Length of the buffer
/// @param [in] flags Send flags
/// @return Same as native send()
ssize_t dnsw_send ( int sockfd, const void * buf, size_t len, int flags );

/// @brief A send() wrapper.
/// @param [in] sockfd Socket to use.
/// @param [in] buf Pointer to the buffer to receive data into
/// @param [in] len Length of the buffer
/// @param [in] flags Receive flags
/// @return Same as native recv()
ssize_t dnsw_recv ( int sockfd, void * buf, size_t len, int flags );

/// @brief A bind() wrapper.
/// @param [in] fd Socket to use.
/// @param [in] addr Pointer to the socket address
/// @param [in] addrLen Length of the socket address
/// @return Same as native bind()
int dnsw_bind ( int fd, const struct sockaddr * addr, socklen_t addrLen );

/// @brief A connect() wrapper.
/// @param [in] fd Socket to use.
/// @param [in] addr Pointer to the socket address
/// @param [in] addrLen Length of the socket address
/// @return Same as native connect()
int dnsw_connect ( int fd, const struct sockaddr * addr, socklen_t addrLen );

/// @brief A getpeername() wrapper.
/// @param [in] fd Socket to use.
/// @param [out] addr Pointer to the buffer to write the socket address.
/// @param [in,out] addrLen Pointer to the length of the socket address buffer.
/// @return Same as native getpeername()
int dnsw_getpeername ( int fd, struct sockaddr * addr, socklen_t * addrLen );

/// @brief A setsockopt() wrapper.
/// @param [in] fd Socket to use.
/// @param [in] level The protocol level of the option.
/// @param [in] optName The socket option to set..
/// @param [in] optVal Pointer to the option value
/// @param [in] optLen Length of the option value.
/// @return Same as native setsockopt()
int dnsw_setsockopt ( int fd, int level, int optName, const void * optVal, socklen_t optLen );

/// @brief A socket() wrapper.
/// @param [in] family The socket family.
/// @param [in] type The socket type.
/// @param [in] protocol The socket protocol.
/// @return Same as native socket()
int dnsw_socket ( int family, int type, int protocol );
