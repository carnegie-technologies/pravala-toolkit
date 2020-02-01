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

#include "sys/Time.hpp"

using namespace Pravala;

/// @brief Time class test

class TimeTest: public ::testing::Test
{
};

/// @brief isGreaterThan1000Milliseconds test
TEST_F ( TimeTest, IsGreaterThan1000Milliseconds )
{
    Time myTime, myTime2;

    myTime.setSeconds ( 22 );
    myTime2.setSeconds ( 21 );

    // Expect 22 seconds is not greater than 22 seconds
    EXPECT_FALSE ( myTime.isGreaterThanMilliseconds ( myTime2, 1000 ) );

    // Expect 22 seconds is greater than 21.999 seconds
    EXPECT_TRUE ( myTime.isGreaterThanMilliseconds ( myTime2, 999 ) );
}

/// @brief Constructor test
TEST_F ( TimeTest, Constructor )
{
    Time myTime;

    EXPECT_EQ ( 0, myTime.getSeconds() );
    EXPECT_EQ ( 0, myTime.getMilliSeconds() );

    myTime = Time ( 0 );

    EXPECT_EQ ( 0, myTime.getSeconds() );
    EXPECT_EQ ( 0, myTime.getMilliSeconds() );

    myTime = Time ( 1 );

    EXPECT_EQ ( 1, myTime.getSeconds() );
    EXPECT_EQ ( 0, myTime.getMilliSeconds() );

    myTime = Time ( 0, 1 );

    EXPECT_EQ ( 0, myTime.getSeconds() );
    EXPECT_EQ ( 1, myTime.getMilliSeconds() );

    myTime = Time ( 1, 2 );

    EXPECT_EQ ( 1, myTime.getSeconds() );
    EXPECT_EQ ( 2, myTime.getMilliSeconds() );

    myTime = Time ( 100, 999 );

    EXPECT_EQ ( 100, myTime.getSeconds() );
    EXPECT_EQ ( 999, myTime.getMilliSeconds() );

    myTime = Time ( 0, 4321 );

    EXPECT_EQ ( 4, myTime.getSeconds() );
    EXPECT_EQ ( 321, myTime.getMilliSeconds() );

    myTime = Time ( 1, 1000 );

    EXPECT_EQ ( 2, myTime.getSeconds() );
    EXPECT_EQ ( 0, myTime.getMilliSeconds() );

    myTime = Time ( 0, 3000 );

    EXPECT_EQ ( 3, myTime.getSeconds() );
    EXPECT_EQ ( 0, myTime.getMilliSeconds() );

    myTime = Time ( 100, 2500 );

    EXPECT_EQ ( 102, myTime.getSeconds() );
    EXPECT_EQ ( 500, myTime.getMilliSeconds() );
}

/// @brief decreaseMilliseconds test
TEST_F ( TimeTest, DecreaseMilliseconds )
{
    Time myTime = Time ( 1, 0 );

    EXPECT_TRUE ( myTime.decreaseMilliseconds ( 1000 ) );
    EXPECT_EQ ( 0, myTime.getSeconds() );
    EXPECT_EQ ( 0, myTime.getMilliSeconds() );

    myTime = Time ( 0, 1 );

    EXPECT_TRUE ( myTime.decreaseMilliseconds ( 1 ) );
    EXPECT_EQ ( 0, myTime.getSeconds() );
    EXPECT_EQ ( 0, myTime.getMilliSeconds() );

    myTime = Time ( 1, 2 );

    EXPECT_TRUE ( myTime.decreaseMilliseconds ( 1002 ) );
    EXPECT_EQ ( 0, myTime.getSeconds() );
    EXPECT_EQ ( 0, myTime.getMilliSeconds() );

    myTime = Time ( 2, 0 );

    EXPECT_TRUE ( myTime.decreaseMilliseconds ( 1000 ) );
    EXPECT_EQ ( 1, myTime.getSeconds() );
    EXPECT_EQ ( 0, myTime.getMilliSeconds() );

    myTime = Time ( 2, 0 );

    EXPECT_TRUE ( myTime.decreaseMilliseconds ( 1 ) );
    EXPECT_EQ ( 1, myTime.getSeconds() );
    EXPECT_EQ ( 999, myTime.getMilliSeconds() );

    myTime = Time ( 2, 0 );

    EXPECT_TRUE ( myTime.decreaseMilliseconds ( 1001 ) );
    EXPECT_EQ ( 0, myTime.getSeconds() );
    EXPECT_EQ ( 999, myTime.getMilliSeconds() );

    myTime = Time ( 10, 100 );

    EXPECT_TRUE ( myTime.decreaseMilliseconds ( 50 ) );
    EXPECT_EQ ( 10, myTime.getSeconds() );
    EXPECT_EQ ( 50, myTime.getMilliSeconds() );

    myTime = Time ( 10, 100 );

    EXPECT_TRUE ( myTime.decreaseMilliseconds ( 300 ) );
    EXPECT_EQ ( 9, myTime.getSeconds() );
    EXPECT_EQ ( 800, myTime.getMilliSeconds() );

    myTime = Time ( 10, 100 );

    EXPECT_TRUE ( myTime.decreaseMilliseconds ( 1050 ) );
    EXPECT_EQ ( 9, myTime.getSeconds() );
    EXPECT_EQ ( 50, myTime.getMilliSeconds() );

    myTime = Time ( 10, 100 );

    EXPECT_TRUE ( myTime.decreaseMilliseconds ( 1300 ) );
    EXPECT_EQ ( 8, myTime.getSeconds() );
    EXPECT_EQ ( 800, myTime.getMilliSeconds() );

    myTime = Time ( 0xFFFFFFFF, 999 );

    EXPECT_TRUE ( myTime.decreaseMilliseconds ( 1 ) );
    EXPECT_EQ ( 0xFFFFFFFF, myTime.getSeconds() );
    EXPECT_EQ ( 998, myTime.getMilliSeconds() );

    myTime = Time ( 4294967295U, 999 );

    EXPECT_TRUE ( myTime.decreaseMilliseconds ( 4294967295U ) );
    EXPECT_EQ ( ( 4294967295U - 4294967 ), myTime.getSeconds() );
    EXPECT_EQ ( ( 999 - 295 ), myTime.getMilliSeconds() );

    // Failed cases:

    myTime = Time ( 0, 1 );

    EXPECT_FALSE ( myTime.decreaseMilliseconds ( 2 ) );
    EXPECT_EQ ( 0, myTime.getSeconds() );
    EXPECT_EQ ( 0, myTime.getMilliSeconds() );

    myTime = Time ( 1, 0 );

    EXPECT_FALSE ( myTime.decreaseMilliseconds ( 1001 ) );
    EXPECT_EQ ( 0, myTime.getSeconds() );
    EXPECT_EQ ( 0, myTime.getMilliSeconds() );

    myTime = Time ( 2, 100 );

    EXPECT_FALSE ( myTime.decreaseMilliseconds ( 8000 ) );
    EXPECT_EQ ( 0, myTime.getSeconds() );
    EXPECT_EQ ( 0, myTime.getMilliSeconds() );

    myTime = Time ( 2, 100 );

    EXPECT_FALSE ( myTime.decreaseMilliseconds ( 2300 ) );
    EXPECT_EQ ( 0, myTime.getSeconds() );
    EXPECT_EQ ( 0, myTime.getMilliSeconds() );
}
