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

#include "base64/Base64.hpp"
#include "basic/MemHandle.hpp"

// Decoded and encoded versions of strings

#define DEC1    "asdfasdfasdf"
#define ENC1    "YXNkZmFzZGZhc2Rm"

#define DEC2    "asdfasdfasd"
#define ENC2    "YXNkZmFzZGZhc2Q="

using namespace Pravala;

/// @brief Base64 class test
class Base64Test: public ::testing::Test
{
};

TEST_F ( Base64Test, SimpleTest )
{
    // No padding
    MemHandle e1 = Base64::encode ( DEC1, strlen ( DEC1 ) );

    EXPECT_EQ ( e1.size(), strlen ( ENC1 ) );
    EXPECT_STREQ ( e1.toString().c_str(), ENC1 );

    MemHandle d1 = Base64::decode ( e1 );

    EXPECT_STREQ ( d1.toString().c_str(), DEC1 );

    // Requires padding
    MemHandle e2 = Base64::encode ( DEC2, strlen ( DEC2 ) );

    EXPECT_EQ ( e2.size(), strlen ( ENC2 ) );
    EXPECT_STREQ ( e2.toString().c_str(), ENC2 );

    MemHandle d2 = Base64::decode ( e2 );

    EXPECT_STREQ ( d2.toString().c_str(), DEC2 );
}
