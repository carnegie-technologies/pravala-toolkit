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

#include "basic/MemHandle.hpp"
#include "sys/Time.hpp"
#include "SslContext.hpp"

namespace Pravala
{
class UdpSocket;

/// @brief Our special BIO object/wrapper for DTLS with OpenSSL
class DtlsBio
{
    public:
        /// @brief Initializes global DtlsBio state.
        /// It is done automatically by the first DtlsBio object created, but not in a thread-safe way.
        /// If DtlsBio objects are to be used on multiple threads, this should be called before creating any threads
        /// that may be doing so.
        /// @return True if the global state has been initialized successfully (or was already initialized);
        ///         False otherwise.
        static bool initBio();

    protected:
        UdpSocket * _udpSocket; ///< Underlying UDP socket.

        /// @brief Constructor.
        /// @param [in] udpSocket UDP socket to use. It is set and stored in this object because we need to use it.
        ///                       However, DtlsBio does NOT create a reference to this socket object!
        ///                       Reference handling, ownership and unsetting this pointer is a responsibility
        ///                       of whoever inherits this object!
        DtlsBio ( UdpSocket * udpSocket );

        /// @brief Destructor.
        ~DtlsBio();

        /// @brief Sets the internal read buffer.
        /// @param [in] data The data to set. It will be cleared!
        inline void setBioReadBuffer ( MemHandle & data )
        {
            _bioReadBuffer = data;
            data.clear();
        }

        /// @brief Configures given SSL object to use DtlsBio.
        /// @param [in] ssl SSL object to configure. Nothing happens if this, or internal BIO pointer is invalid.
        void setupBio ( SSL * ssl );

        /// @brief Returns the timeout value for this DTLS object.
        /// DTLS sets timeouts for re-transmissions. Since we are working with non-blocking
        /// sockets we have to deal with those timeouts ourselves. This function can return
        /// one of the following values:
        /// -1: this means that DTLS' timer is not running, and there is no need for worrying about the timeouts
        ///  0: this means that DTLS' timer is running and is already due - this means it should be dealt
        ///     with right away.
        /// >0: this means that DTLS' timer is running, and will be due in the returned number of milliseconds.
        /// @return The timeout value in ms
        int getDtlsTimeoutMs() const;

        /// @brief Configures the object to perform a test write.
        /// This will set test write flag, and clear the read buffer.
        void startTestWrite();

        /// @brief Ends the test write.
        /// This will unset test write flag, and clear the read buffer.
        /// @return The size of the data (in bytes) that would be sent over the network.
        size_t endTestWrite();

        /// @brief Generates a SSL cookie
        ///
        /// This function is used as a callback from SSL code.
        /// Internally it simply calls generateCookie() in the relevant DtlsBio object
        ///
        /// @param [in] ssl The SSL object that requests the cookie.
        ///                 This function will return 0 if the SSL object doesn't use a BIO object that
        ///                 is associated with a DtlsBio object that, in turn, uses the same BIO pointer.
        /// @param [out] cookie The memory to write the generated cookie to
        /// @param [in,out] cookieLen The length of the cookie (in bytes). The meaning of this is not clear -
        ///                            it depends on the way SSL uses it, its documentation doesn't exist
        ///                            and the code is messed up.
        /// @return 1, if the cookie was generated correctly; 0 otherwise
        static int generateCookie ( SSL * ssl, unsigned char * cookie, unsigned int * cookieLen );

        /// @brief Verifies a SSL cookie
        ///
        /// This function is used as a callback from SSL code.
        /// Internally it calls generateCookie() in the relevant DtlsBio object and compares the result with
        /// the cookie provided. We need two versions of this function, because different OpenSSL versions
        /// use different callback signatures.
        ///
        /// @param [in] ssl The SSL object that requests the cookie.
        ///                 This function will return 0 if the SSL object doesn't use a BIO object that
        ///                 is associated with a DtlsBio object that, in turn, uses the same BIO pointer.
        /// @param [in] cookie The cookie to verify
        /// @param [in] cookieLen The length of the cookie (in bytes)
        /// @return 1, if the cookie is correct; 0 otherwise
        static int verifyCookie ( SSL * ssl, const unsigned char * cookie, unsigned int cookieLen );

        /// @brief Verifies a SSL cookie
        ///
        /// This function is used as a callback from SSL code.
        /// Internally it calls generateCookie() in the relevant DtlsBio object and compares the result with
        /// the cookie provided. We need two versions of this function, because different OpenSSL versions
        /// use different callback signatures.
        ///
        /// @param [in] ssl The SSL object that requests the cookie.
        ///                 This function will return 0 if the SSL object doesn't use a BIO object that
        ///                 is associated with a DtlsBio object that, in turn, uses the same BIO pointer.
        /// @param [in] cookie The cookie to verify
        /// @param [in] cookieLen The length of the cookie (in bytes)
        /// @return 1, if the cookie is correct; 0 otherwise
        static int verifyCookie ( SSL * ssl, unsigned char * cookie, unsigned int cookieLen );

    private:
        static TextLog _bLog; ///< Log stream

        /// @brief Set whenever the datagram we tried to send was too big.
        static const uint8_t BioFlagMtuExceeded = ( 1 << 0 );

        /// @brief When set, BIO is in "peek" mode (which doesn't clear the buffer on read).
        static const uint8_t BioFlagPeekMode = ( 1 << 1 );

        /// @brief When set, BIO is in "test" mode.
        /// In this mode it will not actually send data over the network,
        /// but instead store the data written in the read buffer.
        static const uint8_t BioFlagTestWrite = ( 1 << 2 );

        /// @brief SSL's BIO method description of DtlsBio objects.
        /// It contains the type, description, and callback pointers.
        static BIO_METHOD * _bioMethod;

        /// @brief The time at which the next 'DTLS timeout' should happen.
        /// Used only with DTLS.
        /// It is set by SSL, and used by getTimeoutMs() function.
        Time _dtlsNextTimeout;

        MemHandle _bioReadBuffer; ///< Buffer with the data to be read by OpenSSL.

        BIO * _bio; ///< Pointer to internal BIO object generated by SSL.

        uint8_t _bioFlags; ///< Additional flags.

        /// @brief Writes data to the socket
        ///
        /// Depending on the flags, it either uses send() or sendto().
        /// When sendto() is used the data is sent to the _addr address.
        ///
        /// @param [in] buf The buffer with the data
        /// @param [in] size The size of the data in the buffer
        /// @return The value returned by the appropriate system call - the number of bytes written, or -1 on error.
        int bioWrite ( const char * buf, int size );

        /// @brief Reads data for the SSL.
        /// @param [out] buf The buffer where the data read is placed
        /// @param [in] size The max size of the buffer
        /// @return The value returned by the appropriate system call - the number of bytes read, or -1 on error.
        int bioRead ( char * const buf, const int size );

        /// @brief Performs one of the SSL control operations
        /// @param [in] cmd The command to perform
        /// @param [in] arg1 The first argument of the command
        /// @param [in,out] arg2 The second argument of the command (it's a pointer!)
        /// @return The value that depends on the command type.
        long bioCtrl ( int cmd, long arg1, void * arg2 );

        /// @brief Called when the BIO object is removed.
        /// @return 1 when _bio object was set, 0 otherwise.
        int bioDestroy();

        /// @brief Generates a cookie
        ///
        /// The cookie is generated based on the remote peer's address, and some random data.
        ///
        /// @param [out] cookie The memory to write the generated cookie to
        /// @param [in,out] cookieLen The length of the cookie (in bytes). The meaning of this is not clear -
        ///                            it depends on the way SSL uses it, its documentation doesn't exist
        ///                            and the code is messed up.
        /// @return 1 if the cookie was generated correctly, 0 otherwise
        int generateCookie ( unsigned char * cookie, unsigned int * cookieLen ) const;

        /// @brief Helper function that converts BIO to a DtlsBio object.
        /// It also performs checks (using assert) to make sure that DtlsBio object uses that specific bio pointer.
        /// @param [in] bio SSL's BIO object. MUST be valid and associated with a DtlsBio object which,
        ///                 in turn, is also associated with the same BIO pointer.
        /// @return The DtlsBio object that handles given bio pointer.
        static inline DtlsBio * getDtlsBio ( BIO * bio );

        /// @brief Callback used by SSL for writing data
        /// Internally it uses the other bioWrite() method.
        /// @param [in] bio The BIO object for which the operation is performed.
        ///                 It MUST be associated with a DtlsBio object configured to use that BIO pointer.
        /// @param [in] buf The buffer with the data
        /// @param [in] size The size of the data in the buffer
        /// @return The value returned by the appropriate system call - the number of bytes written, or -1 on error.
        static int bioWrite ( BIO * bio, const char * buf, int size );

        /// @brief Callback used by SSL for reading data
        /// Internally it uses the other bioRead() method.
        /// @param [in] bio The BIO object for which the operation is performed.
        ///                 It MUST be associated with a DtlsBio object configured to use that BIO pointer.
        /// @param [out] buf The buffer to put the data in
        /// @param [in] size The max size of the buffer
        /// @return The value returned by the appropriate system call - the number of bytes read, or -1 on error.
        static int bioRead ( BIO * bio, char * buf, int size );

        /// @brief Callback used by SSL for writing data
        /// Internally it uses the other bioPuts() method.
        /// @param [in] bio The BIO object for which the operation is performed.
        ///                 It MUST be associated with a DtlsBio object configured to use that BIO pointer.
        /// @param [in] str The C string to write
        /// @return The value returned by the appropriate system call - the number of bytes written, or -1 on error.
        static int bioPuts ( BIO * bio, const char * str );

        /// @brief Callback used by SSL for control operations
        /// Internally it uses the other bioCtrl() method.
        /// @param [in] bio The BIO object for which the operation is performed.
        ///                 It MUST be associated with a DtlsBio object configured to use that BIO pointer.
        /// @param [in] cmd The command to perform
        /// @param [in] arg1 The first argument of the command
        /// @param [in,out] arg2 The second argument of the command (it's a pointer!)
        /// @return The value that depends on the command type.
        static long bioCtrl ( BIO * bio, int cmd, long arg1, void * arg2 );

        /// @brief Callback used by SSL for cleaning up the BIO object
        /// Internally it uses the other bioDestroy() method.
        /// @param [in] bio The BIO object for which the operation is performed.
        ///                 It MUST be associated with a DtlsBio object configured to use that BIO pointer.
        /// @return 1 on success, 0 on error (if the bio pointer is 0)
        static int bioDestroy ( BIO * bio );

        friend class SslContext;
};
}
