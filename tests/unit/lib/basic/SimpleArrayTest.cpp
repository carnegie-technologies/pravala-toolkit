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

extern "C"
{
#include <stdint.h>
}

#include <gtest/gtest.h>

#include "basic/SimpleArray.hpp"

using namespace Pravala;

/// @brief List class test
/// @todo More tests should be added!
class SimpleArrayTest: public ::testing::Test
{
};

TEST_F ( SimpleArrayTest, Sort )
{
    SimpleArray<int> a;

    a.ensureSizeUsed ( 10 );

    a[ 0 ] = 99;
    a[ 1 ] = 505050;
    a[ 2 ] = 10;
    a[ 3 ] = 1;
    a[ 4 ] = 20;
    a[ 5 ] = 10000000;
    a[ 6 ] = -11;
    a[ 7 ] = 4;
    a[ 8 ] = -50;
    a[ 9 ] = 99;

    a.sortAscending();

    EXPECT_EQ ( -50, a[ 0 ] );
    EXPECT_EQ ( -11, a[ 1 ] );
    EXPECT_EQ ( 1, a[ 2 ] );
    EXPECT_EQ ( 4, a[ 3 ] );
    EXPECT_EQ ( 10, a[ 4 ] );
    EXPECT_EQ ( 20, a[ 5 ] );
    EXPECT_EQ ( 99, a[ 6 ] );
    EXPECT_EQ ( 99, a[ 7 ] );
    EXPECT_EQ ( 505050, a[ 8 ] );
    EXPECT_EQ ( 10000000, a[ 9 ] );

    a.sortDescending();

    EXPECT_EQ ( -50, a[ 9 ] );
    EXPECT_EQ ( -11, a[ 8 ] );
    EXPECT_EQ ( 1, a[ 7 ] );
    EXPECT_EQ ( 4, a[ 6 ] );
    EXPECT_EQ ( 10, a[ 5 ] );
    EXPECT_EQ ( 20, a[ 4 ] );
    EXPECT_EQ ( 99, a[ 3 ] );
    EXPECT_EQ ( 99, a[ 2 ] );
    EXPECT_EQ ( 505050, a[ 1 ] );
    EXPECT_EQ ( 10000000, a[ 0 ] );

    SimpleArray<uint64_t> b;

    b.getOrCreate ( 0 ) = 123456;
    b.getOrCreate ( 1 ) = 0x22000000000000ULL;
    b.getOrCreate ( 2 ) = 0xFFFFFFFFFFFFFFFFULL;
    b.getOrCreate ( 3 ) = 1;
    b.getOrCreate ( 4 ) = 0x1500000000000000ULL;

    b.sortDescending();

    EXPECT_EQ ( 0xFFFFFFFFFFFFFFFFULL, b[ 0 ] );
    EXPECT_EQ ( 0x1500000000000000ULL, b[ 1 ] );
    EXPECT_EQ ( 0x22000000000000ULL, b[ 2 ] );
    EXPECT_EQ ( 123456, b[ 3 ] );
    EXPECT_EQ ( 1, b[ 4 ] );

    b.sortAscending();

    EXPECT_EQ ( 0xFFFFFFFFFFFFFFFFULL, b[ 4 ] );
    EXPECT_EQ ( 0x1500000000000000ULL, b[ 3 ] );
    EXPECT_EQ ( 0x22000000000000ULL, b[ 2 ] );
    EXPECT_EQ ( 123456, b[ 1 ] );
    EXPECT_EQ ( 1, b[ 0 ] );
}
