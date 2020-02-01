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

#include <gtest/gtest.h>

#include "basic/SeqNum.hpp"

using namespace Pravala;

/// @brief SeqNum test
class SeqNumTest: public ::testing::Test
{
};

TEST_F ( SeqNumTest, SimpleTest )
{
    uint16_t seqNumA = 2;

    decrementSeqNum ( seqNumA );

    EXPECT_EQ ( 1U, seqNumA );

    decrementSeqNum ( seqNumA );

    EXPECT_EQ ( 0xFFFF, seqNumA );

    decrementSeqNum ( seqNumA );

    EXPECT_EQ ( 0xFFFF - 1, seqNumA );

    seqNumA = 1;

    incrementSeqNum ( seqNumA );
    incrementSeqNum ( seqNumA );
    incrementSeqNum ( seqNumA );

    EXPECT_EQ ( 4U, seqNumA );

    uint16_t seqNumB = 4;

    EXPECT_EQ ( 0, compareSeqNums ( seqNumB, seqNumA ) );
    EXPECT_EQ ( 0, compareSeqNums ( seqNumA, seqNumB ) );

    seqNumB = 15;

    EXPECT_EQ ( 11, compareSeqNums ( seqNumB, seqNumA ) );
    EXPECT_EQ ( -11, compareSeqNums ( seqNumA, seqNumB ) );

    // seq num 0 is invalid, so difference between MAX and 1 is 1
    EXPECT_EQ ( 1, compareSeqNums ( 1, 0xFFFF ) );
    EXPECT_EQ ( -1, compareSeqNums ( 0xFFFF, 1 ) );

    seqNumB = 0xFFFF;

    incrementSeqNum ( seqNumB );

    // 0 is invalid
    EXPECT_EQ ( 1U, seqNumB );

    seqNumB = 0xFFFF;

    // seq num 0 is invalid, so difference between MAX and 4 is 4
    EXPECT_EQ ( 4, compareSeqNums ( seqNumA, seqNumB ) );
    EXPECT_EQ ( -4, compareSeqNums ( seqNumB, seqNumA ) );

    seqNumB = 0xFFFF - 5;

    // seq num 0 is invalid, so difference between MAX and 4 is 4
    EXPECT_EQ ( 9, compareSeqNums ( seqNumA, seqNumB ) );
    EXPECT_EQ ( -9, compareSeqNums ( seqNumB, seqNumA ) );

    seqNumA += 10000;

    EXPECT_EQ ( 10009, compareSeqNums ( seqNumA, seqNumB ) );
    EXPECT_EQ ( -10009, compareSeqNums ( seqNumB, seqNumA ) );

    seqNumA = 1;
    seqNumB = 0xFFFF / 2;

    EXPECT_EQ ( ( int32_t ) ( 0xFFFF / 2 - 1 ), compareSeqNums ( seqNumB, seqNumA ) );
    EXPECT_EQ ( -( int32_t ) ( 0xFFFF / 2 - 1 ), compareSeqNums ( seqNumA, seqNumB ) );

    incrementSeqNum ( seqNumB );

    EXPECT_EQ ( ( int32_t ) ( 0xFFFF / 2 ), compareSeqNums ( seqNumB, seqNumA ) );
    EXPECT_EQ ( -( int32_t ) ( 0xFFFF / 2 ), compareSeqNums ( seqNumA, seqNumB ) );

    incrementSeqNum ( seqNumB );

    EXPECT_EQ ( -( int32_t ) ( 0xFFFF / 2 ), compareSeqNums ( seqNumB, seqNumA ) );
    EXPECT_EQ ( ( int32_t ) ( 0xFFFF / 2 ), compareSeqNums ( seqNumA, seqNumB ) );

    incrementSeqNum ( seqNumB );

    EXPECT_EQ ( -( int32_t ) ( 0xFFFF / 2 - 1 ), compareSeqNums ( seqNumB, seqNumA ) );
    EXPECT_EQ ( ( int32_t ) ( 0xFFFF / 2 - 1 ), compareSeqNums ( seqNumA, seqNumB ) );

    seqNumA = 0xFFFF;
    seqNumB = 0xFFFF / 2 - 1;

    EXPECT_EQ ( ( int32_t ) ( 0xFFFF / 2 - 1 ), compareSeqNums ( seqNumB, seqNumA ) );
    EXPECT_EQ ( -( int32_t ) ( 0xFFFF / 2 - 1 ), compareSeqNums ( seqNumA, seqNumB ) );

    incrementSeqNum ( seqNumB );

    EXPECT_EQ ( ( int32_t ) ( 0xFFFF / 2 ), compareSeqNums ( seqNumB, seqNumA ) );
    EXPECT_EQ ( -( int32_t ) ( 0xFFFF / 2 ), compareSeqNums ( seqNumA, seqNumB ) );

    incrementSeqNum ( seqNumB );

    EXPECT_EQ ( -( int32_t ) ( 0xFFFF / 2 ), compareSeqNums ( seqNumB, seqNumA ) );
    EXPECT_EQ ( ( int32_t ) ( 0xFFFF / 2 ), compareSeqNums ( seqNumA, seqNumB ) );

    incrementSeqNum ( seqNumB );

    EXPECT_EQ ( -( int32_t ) ( 0xFFFF / 2 - 1 ), compareSeqNums ( seqNumB, seqNumA ) );
    EXPECT_EQ ( ( int32_t ) ( 0xFFFF / 2 - 1 ), compareSeqNums ( seqNumA, seqNumB ) );
}
