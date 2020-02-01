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

#include "error/Error.hpp"
#include "basic/MemHandle.hpp"

namespace Pravala
{
/// @brief Class that contains static compression utility methods
class Compression
{
    public:
        /// @brief Compress a buffer using gzip
        /// @param [in] data Buffer containing data to compress. An empty buffer will be returned if this is empty, and
        /// eCode will be set to InvalidParameter.
        /// @param [out] eCode If used, the status of the operation will be stored there.
        /// @return MemHandle with the compressed gzip output. If an error occurs, this will be empty.
        static MemHandle gzip ( const MemHandle & data, ERRCODE * eCode = 0 );
};
}
