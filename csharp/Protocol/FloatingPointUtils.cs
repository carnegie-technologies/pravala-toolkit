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

namespace Pravala.Protocol
{
    using System.Diagnostics;
    using System.Text;

    class FloatingPointUtils
    {
        /// <summary>
        /// Packs a float value into uint according to IEEE 754.
        /// </summary>
        /// It handles special values (zero, NaN, and +/- infinity).
        /// <param name="value">The float value to pack.</param>
        /// <returns>Packed value as uint</returns>
        public static uint pack754 ( float ? value )
        {
            if ( value == null || value == 0.0 )
            {
                return 0;
            }
            else if ( System.Single.IsNaN ( ( float ) value ) )
            {
                return SET_Q_NAN_32;
            }
            else if ( System.Single.IsPositiveInfinity ( ( float ) value ) )
            {
                return P_INF_32;
            }
            else if ( System.Single.IsNegativeInfinity ( ( float ) value ) )
            {
                return N_INF_32;
            }
            else
            {
                return ( uint ) int_pack754 ( ( float ) value, FL_BITS_32, FL_EXPBITS_32 );
            }
        }

        /// <summary>
        /// Packs a double value into ulong according to IEEE 754.
        /// </summary>
        /// It handles special values (zero, NaN, and +/- infinity).
        /// <param name="value">The double value to pack.</param>
        /// <returns>Packed value as uint</returns>
        public static ulong pack754 ( double ? value )
        {
            if ( value == null || value == 0.0 )
            {
                return 0;
            }
            else if ( System.Double.IsNaN ( ( double ) value ) )
            {
                return SET_Q_NAN_64;
            }
            else if ( System.Double.IsPositiveInfinity ( ( double ) value ) )
            {
                return P_INF_64;
            }
            else if ( System.Double.IsNegativeInfinity ( ( double ) value ) )
            {
                return N_INF_64;
            }
            else
            {
                return ( ulong ) int_pack754 ( ( double ) value, FL_BITS_64, FL_EXPBITS_64 );
            }
        }

        /// <summary>
        /// Unpacks a float value from uint according to IEEE 754.
        /// </summary>
        /// It handles special values (zero, NaN, and +/- infinity).
        /// <param name="value">The uint value to unpack.</param>
        /// <returns>Unpacked float.</returns>
        public static float unpack754 ( uint ? value )
        {
            if ( value == null || value == 0 )
            {
                return ( float ) 0.0;
            }
            else if ( value == P_INF_32 )
            {
                return System.Single.PositiveInfinity;
            }
            else if ( value == N_INF_32 )
            {
                return System.Single.NegativeInfinity;
            }
            else if ( ( value & NAN_32 ) == NAN_32 && ( value & NAN_MASK_32 ) != 0 )
            {
                // NAN_MASK_32 test should not be needed, since NAN_32 is the same as P_INF_32,
                // but let's keep it for clarity.

                return System.Single.NaN;
            }
            else
            {
                return ( float ) int_unpack754 ( ( uint ) value, FL_BITS_32, FL_EXPBITS_32 );
            }
        }

        /// <summary>
        /// Unpacks a double value from ulong according to IEEE 754.
        /// </summary>
        /// It handles special values (zero, NaN, and +/- infinity).
        /// <param name="value">The uint value to unpack.</param>
        /// <returns>Unpacked double.</returns>
        public static double unpack754 ( ulong ? value )
        {
            if ( value == null || value == 0 )
            {
                return ( double ) 0.0;
            }
            else if ( value == P_INF_64 )
            {
                return System.Double.PositiveInfinity;
            }
            else if ( value == N_INF_64 )
            {
                return System.Double.NegativeInfinity;
            }
            else if ( ( value & NAN_64 ) == NAN_64 && ( value & NAN_MASK_64 ) != 0 )
            {
                // NAN_MASK_64 test should not be needed, since NAN_64 is the same as P_INF_64,
                // but let's keep it for clarity.

                return System.Double.NaN;
            }
            else
            {
                return ( double ) int_unpack754 ( ( ulong ) value, FL_BITS_64, FL_EXPBITS_64 );
            }
        }

        private const int FL_BITS_32 = 32;
        private const int FL_EXPBITS_32 = 8;

        private const int FL_BITS_64 = 64;
        private const int FL_EXPBITS_64 = 11;

        private const uint P_INF_32 = 0x7F800000U;
        private const uint N_INF_32 = 0xFF800000U;
        private const ulong P_INF_64 = 0x7FF0000000000000UL;
        private const ulong N_INF_64 = 0xFFF0000000000000UL;

        // NaNs ignore the sign bit; They have all following bits set to 1:
        private const uint NAN_32 = 0x7F800000U;
        private const ulong NAN_64 = 0x7FF0000000000000UL;

        // If this is 1 then it's a quiet NaN, otherwise it's a signalling NaN:
        private const uint Q_NAN_BIT_32 = 0x400000U;
        private const ulong Q_NAN_BIT_64 = 0x8000000000000UL;

        // And for a number to be NaN (and not infinity) there have to be some 1s in this mask:
        // (the S_NAN_BIT is sufficient for this)
        private const uint NAN_MASK_32 = 0x7FFFFFU;
        private const ulong NAN_MASK_64 = 0xFFFFFFFFFFFFFUL;

        // And, for convenience, this is what we actually sent (we don't really use signalling NaNs at this point):
        private const uint SET_Q_NAN_32 = ( NAN_32 | Q_NAN_BIT_32 );
        private const ulong SET_Q_NAN_64 = ( NAN_64 | Q_NAN_BIT_64 );

        /// <summary>
        /// Packs a double value according to IEEE 754.
        /// </summary>
        /// Function taken from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#serialization
        /// That code is in public domain.
        /// <param name="f">The value to pack.</param>
        /// <param name="bits">Bits.</param>
        /// <param name="expbits">Expbits.</param>
        /// <returns>Packed value.</returns>
        private static ulong int_pack754 ( double f, int bits, int expbits )
        {
            double fnorm;
            int shift;
            ulong sign, exp, significand;
            int significandbits = bits - expbits - 1; // -1 for sign bit

            if ( f == 0.0 ) return 0; // get this special case out of the way

            // check sign and begin normalization
            if ( f < 0 )
            {
                sign = 1;
                fnorm = -f;
            }
            else
            {
                sign = 0;
                fnorm = f;
            }

            // get the normalized form of f and track the exponent
            shift = 0;

            while ( fnorm >= 2.0 )
            {
                fnorm /= 2.0;
                shift++;
            }

            while ( fnorm < 1.0 )
            {
                fnorm *= 2.0;
                shift--;
            }

            fnorm = fnorm - 1.0;

            // calculate the binary form (non-float) of the significand data
            significand = ( ulong ) ( fnorm * ( ( 1UL << significandbits ) + 0.5f ) );

            // get the biased exponent
            exp = ( ulong ) ( shift + ( ( 1 << ( expbits - 1 ) ) - 1 ) ); // shift + bias

            // return the final answer
            return ( sign << ( bits - 1 ) ) | ( exp << ( bits - expbits - 1 ) ) | significand;
        }

        /// <summary>
        /// Unpacks a double value according to IEEE 754.
        /// </summary>
        /// Function taken from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#serialization
        /// That code is in public domain.
        /// <param name="i">The value to unpack.</param>
        /// <param name="bits">Bits.</param>
        /// <param name="expbits">Expbits.</param>
        /// <returns>Unpacked value.</returns>
        private static double int_unpack754 ( ulong i, int bits, int expbits )
        {
            double result;
            long shift;
            uint bias;
            int significandbits = bits - expbits - 1; // -1 for sign bit

            if ( i == 0 ) return 0.0;

            // pull the significand
            result = ( double ) ( i & ( ( 1UL << significandbits ) - 1 ) ); // mask
            result /= ( 1UL << significandbits ); // convert back to float
            result += 1.0f; // add the one back on

            // deal with the exponent
            bias = ( 1U << ( expbits - 1 ) ) - 1;
            shift = ( long ) ( ( ( i >> significandbits ) & ( ( 1UL << expbits ) - 1 ) ) - bias );

            while ( shift > 0 )
            {
                result *= 2.0;
                shift--;
            }

            while ( shift < 0 )
            {
                result /= 2.0;
                shift++;
            }

            // sign it
            result *= ( ( ( i >> ( bits - 1 ) ) & 1 ) == 1 ) ? -1.0 : 1.0;

            return result;
        }
    }
}
