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

#include "basic/Timestamp.hpp"

using namespace Pravala;

/// @brief Timestamp test
class TimestampTest: public ::testing::Test
{
};

TEST_F ( TimestampTest, SimpleTest )
{
    Timestamp::TimeDesc tmp;
    Timestamp::TimeDesc tDesc;

    EXPECT_FALSE ( tDesc.isValid() );

    tDesc.month = 1;
    tDesc.day = 1;
    EXPECT_TRUE ( tDesc.isValid() );

    tmp = tDesc;
    tmp.month = 13;
    EXPECT_FALSE ( tmp.isValid() );

    tmp = tDesc;
    tmp.day = 32;
    EXPECT_FALSE ( tmp.isValid() );

    tmp = tDesc;
    tmp.month = 2;
    tmp.day = 30;
    EXPECT_FALSE ( tmp.isValid() );

    tmp = tDesc;
    tmp.hour = 24;
    EXPECT_FALSE ( tmp.isValid() );

    tmp = tDesc;
    tmp.minute = 60;
    EXPECT_FALSE ( tmp.isValid() );

    tmp = tDesc;
    tmp.second = 60; // leap second
    EXPECT_TRUE ( tmp.isValid() );

    tmp = tDesc;
    tmp.second = 61;
    EXPECT_FALSE ( tmp.isValid() );

    tmp = tDesc;
    tmp.millisecond = 1000;
    EXPECT_FALSE ( tmp.isValid() );

    tDesc.year = 2017;
    tDesc.month = 2;
    tDesc.day = 28;
    tDesc.hour = 23;
    tDesc.minute = 59;
    tDesc.second = 59;
    tDesc.millisecond = 999;

    EXPECT_TRUE ( tDesc.isValid() );

    tDesc.minute = 46;

    EXPECT_TRUE ( tDesc.isValid() );

    Timestamp tStamp;

    EXPECT_TRUE ( tStamp.setUtcTime ( tDesc ) );
    EXPECT_STREQ ( "2017-02-28T23:46:59.999Z", tStamp.toString().c_str() );

    EXPECT_TRUE ( tStamp.setUtcTime ( Timestamp::TimeDesc ( 0, 1, 1, 0, 0, 0, 0 ) ) );
    EXPECT_STREQ ( "0000-01-01T00:00:00.000Z", tStamp.toString().c_str() );
    EXPECT_EQ ( Timestamp::MinBinValue, tStamp.getBinValue() );
}
