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

///
/// @file
/// @brief Sample test suite for Google Test Framework tests
///

// All test files need this include
#include <gtest/gtest.h>

/// @brief Tests are derived from ::testing::Test
/// Note that this class is re-initialized for every test run against it;
/// if you need state between tests, you'll need to use static variables/methods
/// (and even then, tests have no run-order guarantees)
class SampleTest: public ::testing::Test
{
    protected:
        // You can remove any or all of the following functions if its body
        // is empty.

        SampleTest()
        {
            // You can do set-up work for each test here.
        }

        virtual ~SampleTest()
        {
            // You can do clean-up work that doesn't throw exceptions here.
        }

        // If the constructor and destructor are not enough for setting up
        // and cleaning up each test, you can define the following methods:

        virtual void SetUp()
        {
            // Code here will be called immediately after the constructor (right
            // before each test).
        }

        virtual void TearDown()
        {
            // Code here will be called immediately after each test (right
            // before the destructor).
        }

        // Objects declared here can be used by all tests in the test case for Sample.
};

// Register tests against this suite
// First parameter is the test class, second is the test name
// (use format {COMPONENT}_{TESTNAME})
TEST_F ( SampleTest, Sample_TestOne )
{
    ASSERT_TRUE ( true );
}

TEST_F ( SampleTest, Sample_TestTwo )
{
    // Non-fatal failure; test fails, but continues executing
    EXPECT_EQ ( 0, 0 );
    // Fatal failure; test fails AND stops executing
    ASSERT_EQ ( 1, 1 );
}
