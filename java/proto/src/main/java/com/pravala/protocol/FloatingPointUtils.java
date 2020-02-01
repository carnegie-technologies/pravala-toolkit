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

package com.pravala.protocol;

/**
 * Contains functions for manipulating floating point values.
 */
public class FloatingPointUtils
{
    /**
     * Packs a float value into int according to IEEE 754.
     * It handles special values (zero, NaN, and +/- infinity)
     * @param value The float value to pack.
     * @return Integer representation of the float.
     */
    public static int pack754 ( Float value )
    {
        if ( value == null || value == 0.0 )
        {
            return 0;
        }
        else if ( value == Float.NaN )
        {
            return SET_Q_NAN_32;
        }
        else if ( value == Float.POSITIVE_INFINITY )
        {
            return P_INF_32;
        }
        else if ( value == Float.NEGATIVE_INFINITY )
        {
            return N_INF_32;
        }
        else
        {
            return ( int ) int_pack754 ( value, FL_BITS_32, FL_EXPBITS_32 );
        }
    }

    /**
     * Unpacks a float value from integer according to IEEE 754.
     * It handles special values (zero, NaN, and +/- infinity).
     * @param value The integer value to unpack.
     * @return Unpacked float value.
     */
    public static float unpack754 ( Integer value )
    {
        if ( value == null || value == 0 )
        {
            return 0.0f;
        }
        else if ( value == P_INF_32 )
        {
            return Float.POSITIVE_INFINITY;
        }
        else if ( value == N_INF_32 )
        {
            return Float.NEGATIVE_INFINITY;
        }
        else if ( ( value & NAN_32 ) == NAN_32 && ( value & NAN_MASK_32 ) != 0 )
        {
            // NAN_MASK_32 test should not be needed, since NAN_32 is the same as P_INF_32,
            // but let's keep it for clarity.

            return Float.NaN;
        }
        else
        {
            return ( float ) int_unpack754 ( value, FL_BITS_32, FL_EXPBITS_32 );
        }
    }

    /**
     * Packs a double value into long according to IEEE 754.
     * It handles special values (zero, NaN, and +/- infinity)
     * @param value The float value to pack.
     * @return Long representation of the float.
     */
    public static long pack754 ( Double value )
    {
        if ( value == null || value == 0.0 )
        {
            return 0;
        }
        else if ( value == Double.NaN )
        {
            return SET_Q_NAN_64;
        }
        else if ( value == Double.POSITIVE_INFINITY )
        {
            return P_INF_64;
        }
        else if ( value == Double.NEGATIVE_INFINITY )
        {
            return N_INF_64;
        }
        else
        {
            return int_pack754 ( value, FL_BITS_64, FL_EXPBITS_64 );
        }
    }

    /**
     * Unpacks a double value from long according to IEEE 754.
     * It handles special values (zero, NaN, and +/- infinity).
     * @param value The long value to unpack.
     * @return Unpacked double value.
     */
    public static double unpack754 ( Long value )
    {
        if ( value == null || value == 0 )
        {
            return 0.0;
        }
        else if ( value == P_INF_64 )
        {
            return Double.POSITIVE_INFINITY;
        }
        else if ( value == N_INF_64 )
        {
            return Double.NEGATIVE_INFINITY;
        }
        else if ( ( value & NAN_64 ) == NAN_64 && ( value & NAN_MASK_64 ) != 0 )
        {
            // NAN_MASK_32 test should not be needed, since NAN_64 is the same as P_INF_64,
            // but let's keep it for clarity.

            return Double.NaN;
        }
        else
        {
            return int_unpack754 ( value, FL_BITS_64, FL_EXPBITS_64 );
        }
    }

    private static final int FL_BITS_32 = 32;
    private static final int FL_EXPBITS_32 = 8;

    private static final int FL_BITS_64 = 64;
    private static final int FL_EXPBITS_64 = 11;

    private static final int P_INF_32 = 0x7F800000;
    private static final int N_INF_32 = 0xFF800000;
    private static final long P_INF_64 = 0x7FF0000000000000L;
    private static final long N_INF_64 = 0xFFF0000000000000L;

    // NaNs ignore the sign bit; They have all following bits set to 1:
    private static final int NAN_32 = 0x7F800000;
    private static final long NAN_64 = 0x7FF0000000000000L;

    // If this is 1 then it's a quiet NaN, otherwise it's a signalling NaN:
    private static final int Q_NAN_BIT_32 = 0x400000;
    private static final long Q_NAN_BIT_64 = 0x8000000000000L;

    // And for a number to be NaN (and not infinity) there have to be some 1s in this mask:
    // (the S_NAN_BIT is sufficient for this)
    private static final int NAN_MASK_32 = 0x7FFFFF;
    private static final long NAN_MASK_64 = 0xFFFFFFFFFFFFFL;

    // And, for convenience, this is what we actually sent (we don't really use signalling NaNs at this point):
    private static final int SET_Q_NAN_32 = ( NAN_32 | Q_NAN_BIT_32 );
    private static final long SET_Q_NAN_64 = ( NAN_64 | Q_NAN_BIT_64 );

    /**
     * Function taken from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#serialization
     * That code is in public domain.
     */
    private static long int_pack754 ( double f, int bits, int expbits )
    {
        double fnorm;
        int shift;
        long sign, exp, significand;
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
        significand = ( long ) ( fnorm * ( ( 1L << significandbits ) + 0.5 ) );

        // get the biased exponent
        exp = shift + ( ( 1 << ( expbits - 1 ) ) - 1 ); // shift + bias

        // return the final answer
        return ( sign << ( bits - 1 ) ) | ( exp << ( bits - expbits - 1 ) ) | significand;
    }

    /**
     * Function taken from http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#serialization
     * That code is in public domain.
     */
    private static double int_unpack754 ( long i, int bits, int expbits )
    {
        double result;
        long shift;
        int bias;
        int significandbits = bits - expbits - 1; // -1 for sign bit

        if ( i == 0 ) return 0.0;

        // pull the significand
        result = ( i & ( ( 1L << significandbits ) - 1 ) ); // mask
        result /= ( 1L << significandbits ); // convert back to float
        result += 1.0f; // add the one back on

        // deal with the exponent
        bias = ( 1 << ( expbits - 1 ) ) - 1;
        shift = ( ( i >>> significandbits ) & ( ( 1L << expbits ) - 1 ) ) - bias;

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
        result *= ( ( ( i >>> ( bits - 1 ) ) & 1 ) != 0 ) ? -1.0 : 1.0;

        return result;
    }
}
