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

#ifndef _MSC_VER
#pragma GCC diagnostic push
#if ( __GNUC__ >= 6 ) || ( defined __clang__ )
#pragma GCC diagnostic ignored "-Wpedantic"
#else
#pragma GCC diagnostic ignored "-pedantic"
#endif
#endif

extern "C"
{
#include <openssl/x509v3.h>
#include <openssl/x509_vfy.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>
}

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#include <ctime>

#include "X509Utils.hpp"

using namespace Pravala;

TextLog X509Utils::_log ( "x509" );

/// @brief Convert an ASN1_TIME structure to mostly complete tm struct
/// @param [in] a ASN1_TIME to convert
/// @param [out] t struct tm to put the data. It's always memset to 0 first.
/// The year, month, day, hour, minute, second fields are filled out.
/// @return True if the conversion succeeded, false otherwise
static bool ASN1toTM ( const ASN1_TIME * a, struct tm & t )
{
    memset ( &t, 0, sizeof ( t ) );

    if ( !a )
    {
        return false;
    }

    int pos = 0;
    int year = 0;

    // This really doesn't modify it, but OpenSSL is stupid
    if ( !ASN1_TIME_check ( const_cast<ASN1_TIME *> ( a ) ) )
    {
        return false;
    }

    // ASN1_TIME's data field is really just an ASCII string

    if ( a->type == V_ASN1_UTCTIME )
    {
        // YYMMDDHHMMSSZ
        year = 10 * ( a->data[ pos++ ] - '0' );
        year += ( a->data[ pos++ ] - '0' );

        assert ( year < 100 );

        // If ret >= 50, assume year is 19XX, otherwise assume 20XX.
        // Apparently this is in the ASN.1 specs on how to interpret UTCTime (sliding window vs "simple" proposal)...
        // We're just going to assume it's between 1950 - 2049.
        // See http://www.umich.edu/~x509/ssleay/asn1-time.html
        if ( year >= 50 )
        {
            year += 1900;
        }
        else
        {
            year += 2000;
        }
    }
    else if ( a->type == V_ASN1_GENERALIZEDTIME )
    {
        // YYYYMMDDHHMMSSZ
        year = 1000 * ( a->data[ pos++ ] - '0' );
        year += 100 * ( a->data[ pos++ ] - '0' );
        year += 10 * ( a->data[ pos++ ] - '0' );
        year += ( a->data[ pos++ ] - '0' );
    }
    else
    {
        // Don't understand this
        return false;
    }

    // tm_year is year - 1900
    t.tm_year = year - 1900;

    t.tm_mon = 10 * ( a->data[ pos++ ] - '0' );
    t.tm_mon += ( a->data[ pos++ ] - '0' );

    t.tm_mday = 10 * ( a->data[ pos++ ] - '0' );
    t.tm_mday += ( a->data[ pos++ ] - '0' );

    t.tm_hour = 10 * ( a->data[ pos++ ] - '0' );
    t.tm_hour += ( a->data[ pos++ ] - '0' );

    t.tm_min = 10 * ( a->data[ pos++ ] - '0' );
    t.tm_min += ( a->data[ pos++ ] - '0' );

    t.tm_sec = 10 * ( a->data[ pos++ ] - '0' );
    t.tm_sec += ( a->data[ pos++ ] - '0' );

    return true;
}

/// @brief Compare two times
/// @param [in] a First tm struct to compare
/// @param [in] b Second tm struct to compare
/// @return Result of the comparison.
/// >0 if b > a (b is AFTER a)
/// 0  if b == a (when comparing with year, month, day, hour, minute, and seconds)
/// <0 if b < a (b is BEFORE a)
static int compareTimes ( const struct tm & tmA, const struct tm & tmB )
{
    if ( tmA.tm_year != tmB.tm_year )
    {
        return tmB.tm_year - tmA.tm_year;
    }
    else if ( tmA.tm_mon != tmB.tm_mon )
    {
        return tmB.tm_mon - tmA.tm_mon;
    }
    else if ( tmA.tm_mday != tmB.tm_mday )
    {
        return tmB.tm_mday - tmA.tm_mday;
    }
    else if ( tmA.tm_hour != tmB.tm_hour )
    {
        return tmB.tm_hour - tmA.tm_hour;
    }
    else if ( tmA.tm_min != tmB.tm_min )
    {
        return tmB.tm_min - tmA.tm_min;
    }
    else if ( tmA.tm_sec != tmB.tm_sec )
    {
        return tmB.tm_sec - tmA.tm_sec;
    }

    // They are exactly the same!
    assert ( memcmp ( &tmA, &tmB, sizeof ( tmA ) ) == 0 );

    return 0;
}

String X509Utils::getCertSubject ( X509 * cert )
{
    String ret;

    if ( !cert )
        return ret;

    BIO * memBio = BIO_new ( BIO_s_mem() );

    assert ( memBio != 0 );

    X509_NAME_print_ex ( memBio, X509_get_subject_name ( cert ), 0, XN_FLAG_RFC2253 );

    BUF_MEM * bptr;
    BIO_get_mem_ptr ( memBio, &bptr );

    ret.append ( bptr->data, bptr->length );

    BIO_free ( memBio );

    return ret;
}

String X509Utils::getCertIssuer ( X509 * cert )
{
    String ret;

    if ( !cert )
        return ret;

    BIO * memBio = BIO_new ( BIO_s_mem() );

    assert ( memBio != 0 );

    X509_NAME_print_ex ( memBio, X509_get_issuer_name ( cert ), 0, XN_FLAG_RFC2253 );

    BUF_MEM * bptr;
    BIO_get_mem_ptr ( memBio, &bptr );

    ret.append ( bptr->data, bptr->length );

    BIO_free ( memBio );

    return ret;
}

MemHandle X509Utils::getCertSerialNumber ( X509 * cert )
{
    ASN1_INTEGER * serial = X509_get_serialNumber ( cert );

    if ( !serial )
        return MemHandle();

    // This must be freed before returning!
    BIGNUM * bn = ASN1_INTEGER_to_BN ( serial, 0 );

    if ( !bn )
        return MemHandle();

    const size_t len = BN_num_bytes ( bn );

    // Allocate sn with the number of bytes needed
    MemHandle sn ( len );

    if ( !sn.isEmpty() )
    {
        char * w = sn.getWritable();

        assert ( w != 0 );

        const size_t bnLen = BN_bn2bin ( bn, ( unsigned char * ) w );

        // This should be the same as the size
        assert ( bnLen == sn.size() );

        if ( bnLen != sn.size() )
            sn.clear();
    }

    BN_free ( bn );

    return sn;
}

List<IpAddress> X509Utils::getCertIpAddresses ( X509 * cert )
{
    List<IpAddress> ret;

    if ( !cert )
        return ret;

    GENERAL_NAMES * subAltNames = ( GENERAL_NAMES * ) X509_get_ext_d2i ( cert, NID_subject_alt_name, 0, 0 );

    if ( !subAltNames )
        return ret;

    const int count = sk_GENERAL_NAME_num ( subAltNames );

    for ( int i = 0; i < count; i++ )
    {
        const GENERAL_NAME * altName = sk_GENERAL_NAME_value ( subAltNames, i );

        if ( altName->type != GEN_IPADD )
            continue;

        if ( altName->d.iPAddress->length == 4 )
        {
            in_addr a;
            memcpy ( &a, altName->d.iPAddress->data, 4 );

            ret.append ( IpAddress ( a ) );
        }
        else if ( altName->d.iPAddress->length == 16 )
        {
            in6_addr a;
            memcpy ( &a, altName->d.iPAddress->data, 16 );

            ret.append ( IpAddress ( a ) );
        }
    }

    return ret;
}

List<X509 *> X509Utils::pemReadCertificates ( const char * data, size_t dataSize, size_t maxCerts )
{
    List<X509 *> certs;

    if ( !data || dataSize < 1 )
    {
        return certs;
    }

    BIO * memBio = BIO_new ( BIO_s_mem() );

    const int ret = BIO_write ( memBio, data, dataSize );

    if ( ret < 0 || ( size_t ) ret != dataSize )
    {
        BIO_free ( memBio );

        memBio = 0;

        return certs;
    }

    X509 * cert = 0;

    while ( ( maxCerts < 1 || certs.size() < maxCerts ) && ( cert = PEM_read_bio_X509 ( memBio, 0, 0, 0 ) ) != 0 )
    {
        certs.append ( cert );
    }

    BIO_free ( memBio );

    memBio = 0;

    return certs;
}

void X509Utils::pemClearCertificates ( List<X509 *> & certs )
{
    while ( !certs.isEmpty() )
    {
        X509_free ( certs.first() );
        certs.removeFirst();
    }
}

bool X509Utils::validateCertDates ( X509 * cert )
{
    if ( !cert )
        return false;

    // These functions do not allocate ASN1_TIME objects, don't free these!
    const ASN1_TIME * notBefore = X509_get_notBefore ( cert );
    const ASN1_TIME * notAfter = X509_get_notAfter ( cert );

    // This allocates an ASN1_TIME object that we'll have to free later!
    ASN1_TIME * now = ASN1_TIME_set ( 0, ::time ( 0 ) );

    if ( !now )
    {
        LOG ( L_ERROR, "Could not get current time" );

        return false;
    }

    struct tm notBeforeTm;
    struct tm notAfterTm;
    struct tm nowTm;

    if ( !ASN1toTM ( notBefore, notBeforeTm ) || !ASN1toTM ( notAfter, notAfterTm ) || !ASN1toTM ( now, nowTm ) )
    {
        LOG ( L_ERROR, "Could not convert certificate times" );

        // An ASN1_TIME is freed by ASN1_STRING_free
        ASN1_STRING_free ( now );
        now = 0;

        return false;
    }

    // An ASN1_TIME is freed by ASN1_STRING_free
    ASN1_STRING_free ( now );
    now = 0;

    const int compBefore = compareTimes ( notBeforeTm, nowTm );
    const int compAfter = compareTimes ( nowTm, notAfterTm );

    if ( compBefore < 0 )
    {
        LOG ( L_WARN, "Certificate is NOT VALID YET; Subject: " << getCertSubject ( cert )
              << "; Issuer: " << getCertIssuer ( cert ) << "; Current time: "
              << String::number ( nowTm.tm_year + 1900, String::Int_Dec, 4, true )
              << "-" << String::number ( nowTm.tm_mon, String::Int_Dec, 2, true )
              << "-" << String::number ( nowTm.tm_mday, String::Int_Dec, 2, true )
              << "_" << String::number ( nowTm.tm_hour, String::Int_Dec, 2, true )
              << ":" << String::number ( nowTm.tm_min, String::Int_Dec, 2, true )
              << ":" << String::number ( nowTm.tm_sec, String::Int_Dec, 2, true )
              << "; Not-Before time: "
              << String::number ( notBeforeTm.tm_year + 1900, String::Int_Dec, 4, true )
              << "-" << String::number ( notBeforeTm.tm_mon, String::Int_Dec, 2, true )
              << "-" << String::number ( notBeforeTm.tm_mday, String::Int_Dec, 2, true )
              << "_" << String::number ( notBeforeTm.tm_hour, String::Int_Dec, 2, true )
              << ":" << String::number ( notBeforeTm.tm_min, String::Int_Dec, 2, true )
              << ":" << String::number ( notBeforeTm.tm_sec, String::Int_Dec, 2, true ) );

        return false;
    }
    else if ( compAfter <= 0 )
    {
        LOG ( L_WARN, "Certificate is NOT VALID ANY MORE; Subject: " << getCertSubject ( cert )
              << "; Issuer: " << getCertIssuer ( cert ) << "; Current time: "
              << String::number ( nowTm.tm_year + 1900, String::Int_Dec, 4, true )
              << "-" << String::number ( nowTm.tm_mon, String::Int_Dec, 2, true )
              << "-" << String::number ( nowTm.tm_mday, String::Int_Dec, 2, true )
              << "_" << String::number ( nowTm.tm_hour, String::Int_Dec, 2, true )
              << ":" << String::number ( nowTm.tm_min, String::Int_Dec, 2, true )
              << ":" << String::number ( nowTm.tm_sec, String::Int_Dec, 2, true )
              << "; Not-After time: "
              << String::number ( notAfterTm.tm_year + 1900, String::Int_Dec, 4, true )
              << "-" << String::number ( notAfterTm.tm_mon, String::Int_Dec, 2, true )
              << "-" << String::number ( notAfterTm.tm_mday, String::Int_Dec, 2, true )
              << "_" << String::number ( notAfterTm.tm_hour, String::Int_Dec, 2, true )
              << ":" << String::number ( notAfterTm.tm_min, String::Int_Dec, 2, true )
              << ":" << String::number ( notAfterTm.tm_sec, String::Int_Dec, 2, true ) );

        return false;
    }

    // compBefore >= 0 means that the notBefore date is BEFORE "now", so the before date is valid.
    // compAfter > 0 means that the notAfter date is AFTER "now", so the after date is valid.
    //
    // If it's compAfter == 0, this means the cert will be invalid shortly, so we'll just treat it as invalid.

    assert ( compBefore >= 0 && compAfter > 0 );

    LOG ( L_DEBUG, "Certificate is VALID; Subject: " << getCertSubject ( cert )
          << "; Issuer: " << getCertIssuer ( cert ) );

    return true;
}
