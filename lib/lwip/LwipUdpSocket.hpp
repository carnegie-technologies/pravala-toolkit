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

#include <cerrno>

#include "LwipSocket.hpp"
#include "LwipBufferIterator.hpp"

struct udp_pcb;
struct pbuf;

namespace Pravala
{
/// @brief Wrapper class for an lwIP UDP socket
class LwipUdpSocket: public LwipSocket
{
    public:
        /// @brief Class that should be inherited by classes receiving data and events from an LwipUdpSocket
        class Receiver
        {
            protected:
                /// @brief Called when a UDP socket receives data
                /// @param [in] sock The LwipUdpSocket that received the data
                /// @param [in] iter The iterator to the data received
                /// @param [in] addr The source address of the data
                virtual void lwipUdpSocketReceivedData (
                    LwipUdpSocket * sock, const LwipBufferIterator & iter, const SockAddr & addr ) = 0;

                /// @brief Destructor.
                virtual ~Receiver()
                {
                }

                friend class LwipUdpSocket;
        };

        /// @brief Constructor
        /// @param [in] receiver The receiver object that will be called to receive UDP data and events
        LwipUdpSocket ( Receiver & receiver );

        /// @brief Destructor
        ~LwipUdpSocket();

        /// @brief Close the UDP socket
        void close();

        virtual int32_t getOption ( int32_t level, int32_t optName, MemHandle & optVal );
        virtual int32_t bind ( const SockAddr & address );
        virtual int32_t connect ( const SockAddr & address );
        virtual int32_t send ( const MemHandle & data );
        virtual int32_t sendTo ( const MemHandle & data, const SockAddr & sockAddr );
        virtual LwipUdpSocket * getUDP();
        virtual const char * getTypeName() const;

    private:
        /// @brief Receives callbacks from this LwipUdpSocket
        Receiver & _receiver;

        /// @brief Pointer to the lwIP UDP socket, 0 if the socket is closed
        /// It is stored in a union, to be able to treat it as ip_pcb pointer.
        /// We can do that, because beginning of udp_pcb is exactly the same as the entire ip_pcb.
        /// In C++ udp_pcb would inherit ip_pcb, but this is C code.
        union
        {
            ip_pcb * ip; ///< Pointer to lwIP socket, as ip_pcb.
            udp_pcb * udp; ///< Pointer to lwIP socket, as udp_pcb.
        } _lwipSock;

        /// @brief Called when the UDP socket has received some data, and its receiver should be notified.
        /// @param buffer The received data buffer. This can't be null.
        /// @param addr The src address of the data. This can't be null.
        /// @param port The src port of the data.
        void readEvent ( pbuf * buffer, const ip_addr_t * addr, uint16_t port );

        /// @brief Called by lwIP when UDP data has been received.
        /// This is an lwIP callback.
        ///
        /// @param arg Pointer to the LwipUdpSocket that should receive this event
        /// @param udp The lwIP UDP socket which received data
        /// @param buffer The received data buffer. This can't be null.
        /// @param addr The src address of the data. This can't be null.
        /// @param port The src port of the data.
        static void recvFunc ( void * arg, udp_pcb * udp, pbuf * buffer, const ip_addr_t * addr, uint16_t port );
};
}
