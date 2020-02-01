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

#include "basic/String.hpp"
#include "basic/IpAddress.hpp"
#include "basic/MemHandle.hpp"
#include "log/TextLog.hpp"

typedef struct x509_st X509;

namespace Pravala
{
/// @brief Useful functions to manipulate OpenSSL's X509 structure
class X509Utils
{
    public:
        /// @brief Retrieve a certificate's subject
        /// @param [in] cert Certificate to retrieve subject from
        /// @return String containing subject (empty if not found or invalid cert)
        static String getCertSubject ( X509 * cert );

        /// @brief Retrieve a certificate's issuer
        /// @param [in] cert Certificate to retrieve issuer from
        /// @return String containing issuer (empty if not found or invalid cert)
        static String getCertIssuer ( X509 * cert );

        /// @brief Retrieve the certificate's serial number
        /// @param [in] cert Certificate to retrieve serial number from
        /// @return MemHandle containing the serial number in big-endian format. According to X.509 specifications,
        /// this can be up to 20 bytes.
        static MemHandle getCertSerialNumber ( X509 * cert );

        /// @brief Retrieve a certificate's IP addresses from its SubjectAltName field(s)
        /// @param [in] cert Certificate to retrieve IP addresses from
        /// @return List of IP addresses (empty if none found or invalid cert)
        static List<IpAddress> getCertIpAddresses ( X509 * cert );

        /// @brief Validate if a certificate is currently valid (i.e. current date is between notBefore and notAfter)
        /// @param [in] cert Certificate to validate
        /// @return True if certificate's date is valid, false if the date is invalid or an error occurred.
        static bool validateCertDates ( X509 * cert );

        /// @brief Extracts a list of certificates stored in PEM format.
        /// @param [in] data Pointer to PEM data.
        /// @param [in] dataSize The size of data.
        /// @param [in] maxCerts The max number of certificates to extract. If 0, all certificates will be extracted.
        ///                       Otherwise only up to maxCerts certificates will be read and added to the list.
        /// @return List of pointer to X509 certificates extracted from PEM data. Each element of this list
        ///          has to be freed by the caller using X509_free() SSL function.
        ///          Alternatively, the whole list can be freed using pemClearCertificates().
        static List<X509 *> pemReadCertificates ( const char * data, size_t dataSize, size_t maxCerts = 0 );

        /// @brief A helper function that deallocates all certificates in the list and clears the list.
        /// @param [in,out] certs The list of certificates to deallocate. The list itself will be cleared as well.
        static void pemClearCertificates ( List<X509 *> & certs );

    private:
        static TextLog _log; ///< Log stream.
};
}
