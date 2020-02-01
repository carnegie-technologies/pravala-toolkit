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

#include "IpFlow.hpp"

namespace Pravala
{
/// @brief An IP flow that uses two descriptors.
/// This version can be used for flows that use NAT and should support look-ups using two different descriptors.
class DualIpFlow: public IpFlow
{
    public:
        /// @brief The type of the descriptor used for matching against this object's secondary flow descriptor.
        static const uint8_t SecondaryDescType = 1;

        /// @brief Exposes secondary flow descriptor.
        /// @return Secondary flow descriptor.
        inline const FlowDesc & getSecondaryFlowDesc() const
        {
            return _secondaryDesc;
        }

        virtual bool matchFlow ( const FlowDesc & flowDesc, uint8_t descType ) const;

    protected:
        /// @brief Default constructor.
        DualIpFlow();

        /// @brief Constructor.
        /// @param [in] defaultFlowDesc The default flow descriptor to set.
        DualIpFlow ( const FlowDesc & defaultFlowDesc );

        /// @brief Constructor.
        /// @param [in] defaultFlowDesc The default flow descriptor to set.
        /// @param [in] secFlowDesc Secondary flow descriptor to set.
        DualIpFlow ( const FlowDesc & defaultFlowDesc, const FlowDesc & secFlowDesc );

        /// @brief Destructor.
        virtual ~DualIpFlow();

        /// @brief Configures flow's secondary descriptor.
        /// @warning This is potentially dangerous function.
        ///          Flow descriptor must NOT be changed when the IpFlow object is a part of an IpFlowMap.
        ///          This function does not verify that's the case, it's up to the caller to ensure that.
        /// @param [in] flowDesc Secondary flow descriptor to set.
        void setSecondaryFlowDesc ( const FlowDesc & flowDesc );

        virtual bool conflictsWith ( IpFlow * otherFlow );
        virtual bool mapInsert ( IpFlowMap & flowMap );
        virtual void mapRemove ( IpFlowMap & flowMap );

    private:
        FlowDesc _secondaryDesc; ///< The flow descriptor for this object.
};
}
