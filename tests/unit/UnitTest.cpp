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
/// @brief Google Test Framework test runner program
/// See the options that can be passed to configure which tests are run at
/// http://code.google.com/p/googletest/wiki/GoogleTestAdvancedGuide
///

#include <gtest/gtest.h>

#include "basic/Random.hpp"

using namespace Pravala;

int main ( int argc, char ** argv )
{
    ::testing::InitGoogleTest ( &argc, argv );

    Random::init ( ::testing::UnitTest::GetInstance()->random_seed() );

    return RUN_ALL_TESTS();
}
