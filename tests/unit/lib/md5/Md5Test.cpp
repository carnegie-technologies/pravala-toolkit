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

#include "md5/Md5.hpp"

using namespace Pravala;

/// @brief MD5 class test
class Md5Test: public ::testing::Test
{
};

TEST_F ( Md5Test, SimpleTest )
{
    Md5 md;

    EXPECT_STREQ ( "d41d8cd98f00b204e9800998ecf8427e", md.getResult().c_str() );
    EXPECT_STREQ ( "d41d8cd98f00b204e9800998ecf8427e", md.getResult().c_str() );
    EXPECT_STREQ ( "D41D8CD98F00B204E9800998ECF8427E", md.getResult ( false ).c_str() );

    md.reset();
    EXPECT_TRUE ( md.update ( "This is a simple string" ) );

    EXPECT_STREQ ( "0f13e02ea41fb763b0ad09daa72a4b6e", md.getResult().c_str() );
    EXPECT_STREQ ( "0F13E02EA41FB763B0AD09DAA72A4B6E", md.getResult ( false ).c_str() );
    EXPECT_STREQ ( "0f13e02ea41fb763b0ad09daa72a4b6e", md.getResult().c_str() );

    md.reset();
    EXPECT_TRUE ( md.update ( "String appended several times" ) );
    EXPECT_TRUE ( md.update ( "String appended several times" ) );
    EXPECT_TRUE ( md.update ( "String appended several times" ) );
    EXPECT_TRUE ( md.update ( "String appended several times" ) );
    EXPECT_TRUE ( md.update ( "String appended several times" ) );
    EXPECT_TRUE ( md.update ( "String appended several times" ) );
    EXPECT_TRUE ( md.update ( "String appended several times" ) );
    EXPECT_TRUE ( md.update ( "String appended several times" ) );

    EXPECT_STREQ ( "e5eda5eab46c1b311eaf974e44c195ae", md.getResult().c_str() );
    EXPECT_STREQ ( "E5EDA5EAB46C1B311EAF974E44C195AE", md.getResult ( false ).c_str() );
    EXPECT_STREQ ( "e5eda5eab46c1b311eaf974e44c195ae", md.getResult().c_str() );
}
