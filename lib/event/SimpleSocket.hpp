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
#include "sys/SocketApi.hpp"
#include "EventManager.hpp"

namespace Pravala
{
/// @brief A wrapper around POSIX sockets.
/// @todo TODO This class should be moved to LibSocket, once SocketPair is moved.
class SimpleSocket: public NoCopy, public SocketApi
{
    public:
        /// @brief Default constructor
        SimpleSocket();

        /// @brief Destructor
        ~SimpleSocket();

        /// @brief Returns the internal socket descriptor
        /// @return Internal socket descriptor
        inline int getSock() const
        {
            return _sock;
        }

        /// @brief Returns true if the socket has been initialized.
        /// @return True if the socket has a valid descriptor; False otherwise.
        inline bool isInitialized() const
        {
            return ( _sock >= 0 );
        }

        /// @brief Returns the internal socket descriptor, and unsets it in the socket.
        /// After this it is caller's responsibility to close that descriptor;
        /// @return Internal socket descriptor
        inline int takeSock()
        {
            int s = _sock;
            _sock = -1;

            return s;
        }

        /// @brief Returns socket type
        /// @return socket type
        inline SocketType getType() const
        {
            return _type;
        }

        /// @brief Initializes (creates) an underlying socket descriptor
        /// If the underlying socket already exists, it is closed first.
        /// @param [in] sockType The type of the socket to create
        /// @return Standard error code
        ERRCODE init ( SocketType sockType );

        /// @brief Initializes this socket as a listening TCP socket
        /// @param [in] localAddr Local address to use
        /// @param [in] localPort Local port to use
        /// @param [in] backLog Maximum connection backlog allowed
        /// @return Standard error code
        ERRCODE initListeningTcpSocket ( const IpAddress & localAddr, uint16_t localPort, int backLog );

        /// @brief Closes the socket
        /// It uses the EventManager, so if something was registered for receiving events from it,
        /// it is unregistered.
        void close();

        /// @brief Binds underlying socket
        /// @param [in] addr Local address to bind it to
        /// @param [in] port Local port number to bind it to
        /// @return Standard error code
        inline ERRCODE bind ( const IpAddress & addr, uint16_t port )
        {
            if ( _sock < 0 )
            {
                return Error::NotInitialized;
            }

            if ( !addr.isValid() )
            {
                return Error::InvalidAddress;
            }

            return SocketApi::bind ( _sock, addr, port ) ? ( Error::Success ) : ( Error::BindFailed );
        }

        /// @brief Binds the local socket to the given name
        /// @param [in] name The name to bind the socket to. If the first character is '@' then
        ///                   the abstract namespace is used (and the first character is removed).
        ///                  To use abstract namespace with a name starting with '@', use it twice.
        /// @return Standard error code
        inline ERRCODE bind ( const String & name )
        {
            if ( _sock < 0 )
            {
                return Error::NotInitialized;
            }

            if ( name.length() < 1 || name.length() > 100 )
            {
                return Error::InvalidParameter;
            }

            return SocketApi::bind ( _sock, name ) ? ( Error::Success ) : ( Error::BindFailed );
        }

        /// @brief Connects underlying socket
        /// @param [in] addr Remote address to connect to
        /// @param [in] port Remote port number to connect to
        /// @return Standard error code; Error::ConnInProgress means that the connection attempt has been
        ///          started.
        inline ERRCODE connect ( const IpAddress & addr, uint16_t port )
        {
            return SocketApi::connect ( _sock, addr, port );
        }

        /// @brief Connects underlying socket
        /// @param [in] addr Remote socket address to connect to
        /// @return Standard error code; Error::ConnInProgress means that the connection attempt has been
        ///          started.
        inline ERRCODE connect ( const SockAddr & addr )
        {
            return SocketApi::connect ( _sock, addr );
        }

        /// @brief Connects underlying socket
        /// @param [in] name The name/path to connect to. If the first character is '@' then
        ///                   the abstract namespace is used (and the first character is removed).
        ///                  To use abstract namespace with a name starting with '@', use it twice.
        /// @return Standard error code; Error::ConnInProgress means that the connection attempt has been
        ///          started.
        inline ERRCODE connect ( const String & name )
        {
            return SocketApi::connect ( _sock, name );
        }

        /// @brief Starts listening on the socket for incoming connections
        /// @param [in] backlog The maximum length to which the queue of pending connections may grow
        /// @return Standard error code
        inline ERRCODE listen ( int backlog )
        {
            if ( _sock < 0 )
            {
                return Error::NotInitialized;
            }

            if ( backlog < 1 )
            {
                return Error::InvalidParameter;
            }

            return SocketApi::listen ( _sock, backlog ) ? ( Error::Success ) : ( Error::ListenFailed );
        }

        /// @brief Modifies the 'blocking' mode of the socket
        /// @param [in] socket The socket descriptor
        /// @param [in] nonBlocking If true (default), the descriptor will be set in 'non-blocking' mode.
        ///                         If false, it will be set in 'blocking' mode.
        /// @return Standard error code
        inline ERRCODE setNonBlocking ( bool nonBlocking = true )
        {
            if ( _sock < 0 )
            {
                return Error::NotInitialized;
            }

            return SocketApi::setNonBlocking ( _sock, nonBlocking ) ? ( Error::Success ) : ( Error::FcntlFailed );
        }

        /// @brief Sets the socket option
        /// @param [in] level the protocol level at which the option resides
        /// @param [in] name specifies an option to set
        /// @param [in] value the value to set
        /// @return Standard error code
        template<typename T> inline ERRCODE setOption ( int level, int name, const T & value )
        {
            if ( _sock < 0 )
            {
                return Error::NotInitialized;
            }

            return SocketApi::setOption ( _sock, level, name, ( const void * ) &value, sizeof ( value ) )
                   ? ( Error::Success ) : ( Error::SetSockOptFailed );
        }

        /// @brief Sets the socket option
        /// @param [in] level the protocol level at which the option resides
        /// @param [in] name specifies an option to set
        /// @param [in] value the value to set
        /// @return Standard error code
        template<typename T> inline ERRCODE getOption ( int level, int name, T & value )
        {
            if ( _sock < 0 )
            {
                return Error::NotInitialized;
            }

            /// @todo Maybe we want to allow the different length?

            socklen_t len = sizeof ( value );

            if ( !SocketApi::getOption ( _sock, level, name, ( void * ) &value, &len ) )
            {
                return Error::GetSockOptFailed;
            }

            if ( len != sizeof ( value ) )
            {
                return Error::InvalidDataSize;
            }

            return Error::Success;
        }

        /// @brief Reads the socket name (using getsockname) from the socket
        /// @param [out] sockAddr The address read
        /// @return Standard error code
        inline ERRCODE getName ( SockAddr & sockAddr )
        {
            if ( _sock < 0 )
            {
                return Error::NotInitialized;
            }

            return SocketApi::getName ( _sock, sockAddr ) ? ( Error::Success ) : ( Error::GetSockOptFailed );
        }

        /// @brief Accepts a connection on the internet socket
        /// @param [out] addr The IP address of the incoming socket
        /// @param [out] port The port number of the incoming socket
        /// @return The descriptor of the new socket, or -1 on error.
        inline int accept ( IpAddress & addr, uint16_t & port ) const
        {
            // accept() itself deals with _sock < 0
            return SocketApi::accept ( _sock, addr, port );
        }

        /// @brief Accepts a connection on the local socket
        /// @param [out] name The name on which it was accepted.
        ///                    If the abstract namespace was used, the name is prepended with '@' character.
        /// @return The new socket descriptor, or -1 on error
        inline int accept ( String & name ) const
        {
            // accept() itself deals with _sock < 0
            return SocketApi::accept ( _sock, name );
        }

        /// @brief Accepts a connection and initializes the given socket object using the new socket file descriptor.
        /// @note If the accepted socket is already configured, it will be closed and re-initialized AFTER the accept.
        ///       So it is possible to call accept() on a listening socket passing itself as accepted socket,
        ///       have it replaced with the new, accepted socket, and the original, listening, file descriptor closed.
        /// @param [in] acceptedSock The socket that will hold the new accepted socket file descriptor.
        /// @param [out] addr The IP address of the incoming socket
        /// @param [out] port The port number of the incoming socket
        /// @return Standard error code.
        ERRCODE accept ( SimpleSocket & acceptedSock, IpAddress & addr, uint16_t & port );

        /// @brief Accepts a connection and initializes given socket object using the new socket file descriptor.
        /// @note If the accepted socket is already configured, it will be closed and re-initialized AFTER the accept.
        ///       So it is possible to call accept() on a listening socket passing itself as accepted socket,
        ///       have it replaced with the new, accepted socket, and the original, listening, file descriptor closed.
        /// @param [in] acceptedSock The socket that will hold the new accepted socket file descriptor.
        /// @param [out] name The name on which it was accepted.
        ///                    If the abstract namespace was used, the name is prepended with '@' character.
        /// @return Standard error code.
        ERRCODE accept ( SimpleSocket & acceptedSock, String & name );

    private:
        int _sock; ///< This socket's internal descriptor
        SocketType _type; ///< The type of the socket

        /// @brief Private copy constructor
        /// Not implemented
        SimpleSocket ( const SimpleSocket & );

        /// @brief Private operator=
        /// Not implemented
        SimpleSocket & operator= ( const SimpleSocket & );
};
}
