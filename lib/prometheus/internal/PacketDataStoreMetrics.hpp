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

#include "prometheus/PrometheusCounter.hpp"
#include "prometheus/PrometheusGauge.hpp"

namespace Pravala
{
/// @brief Class which appends packet data store block metric information.
class PacketDataStoreBlocksGauge: public PrometheusGauge
{
    public:
        /// @brief The block type this gauge should be reporting about.
        enum BlockType
        {
            TypeFree,      ///< Free blocks.
            TypeAllocated  ///< Allocated blocks.
        };

        /// @brief Constructor for packet data store blocks
        PacketDataStoreBlocksGauge ( BlockType type );

    protected:
        const BlockType _type; ///< The block type this gauge is reporting about.

        virtual int64_t getValue();

        /// @brief Returns the name of the block type.
        /// The name is suitable for using as prometheus ID.
        /// @param [in] type The block type to return the name of.
        /// @return The name of given block type.
        static String getBlockTypeName ( BlockType type );
};

/// @brief Custom counter metric for packet data store misses
class PacketDataStoreMissesCounter: public PrometheusCounter
{
    public:
        /// @brief Constructor for packet data store misses
        PacketDataStoreMissesCounter();

    protected:
        virtual uint64_t getValue();
};
}
