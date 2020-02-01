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

#pragma once

#include <ostream>
#include <gtest/gtest.h>

#include "error/Error.hpp"
#include "basic/String.hpp"
#include "basic/IpAddress.hpp"

#define EXPECT_ERRCODE_EQ( errcode, expression ) \
    do { \
        ERRCODE tmpErrCode = ( errcode ); \
        ERRCODE tmpErrCodeRet = ( expression ); \
        EXPECT_EQ ( tmpErrCode.value(), tmpErrCodeRet.value() ) \
        << "Expected: " << tmpErrCode.toString() \
        << "\nActual: " << tmpErrCodeRet.toString(); \
    } while ( false )

#define ASSERT_ERRCODE_EQ( errcode, expression ) \
    do { \
        ERRCODE tmpErrCode = ( errcode ); \
        ERRCODE tmpErrCodeRet = ( expression ); \
        ASSERT_EQ ( tmpErrCode.value(), tmpErrCodeRet.value() ) \
        << "Expected: " << tmpErrCode.toString() \
        << "\nActual: " << tmpErrCodeRet.toString(); \
    } while ( false )

namespace Pravala
{
inline std::ostream & operator<< ( std::ostream & out, const String & str )
{
    out << str.c_str();
    return out;
}

inline std::ostream & operator<< ( std::ostream & out, const IpAddress & ipAddr )
{
    out << ipAddr.toString().c_str();
    return out;
}

inline std::ostream & operator<< ( std::ostream & out, const SockAddr & sa )
{
    out << sa.toString().c_str();
    return out;
}
}
