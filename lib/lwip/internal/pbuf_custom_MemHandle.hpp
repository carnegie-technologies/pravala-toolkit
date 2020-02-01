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

extern "C"
{
#include "lwip/pbuf.h"
}

#include "basic/MemHandle.hpp"

namespace Pravala
{
/// @brief Wrapper class for lwIP's pbuf_custom that allows attaching a MemHandle.
/// The MemHandle will be correctly unref'd when lwIP calls our custom free function.
/// This class is used to avoid copying memory when passing IP packets into lwIP's stack.
///
/// lwIP provides a C struct pbuf_custom, that allows custom data allocation, with a custom free function.
/// However since we need to store a MemHandle somewhere, we use this class that directly inherits a pbuf_custom.
/// This works because the memory at the beginning of this class is identical to pbuf_custom, and the MemHandle
/// will be after all the pbuf and pbuf_custom stuff. See lwip/src/include/lwip/pbuf.h
/// Therefore we can safely cast this to struct pbuf * and use it in lwIP.
///
/// Just don't create virtual methods in this class...
class pbuf_custom_MemHandle: public pbuf_custom
{
    public:
        /// @brief Constructor
        /// @param [in] mh The MemHandle to attach
        inline pbuf_custom_MemHandle ( const MemHandle & mh ): _data ( mh )
        {
            this->custom_free_function = &customFreeFunc;

            // Note: this doesn't allocate any memory, it just initializes fields in pbuf_custom
            // Also we use PBUF_REF instead of PBUF_ROM here; lwIP really shouldn't copy our data,
            // and doesn't appear to do so in this direction. But having PBUF_REF is probably safer,
            // in case lwIP needs to copy and modify our data.
            struct pbuf * buffer = pbuf_alloced_custom (
                PBUF_RAW, _data.size(), PBUF_REF,
                this, const_cast<char *> ( _data.get() ), _data.size() );

            assert ( buffer == &this->pbuf );
            assert ( ( void * ) this == ( void * ) &this->pbuf );
            ( void ) buffer;
        }

        /// @brief Exposes the internal MemHandle object.
        /// @return Internal MemHandle object.
        inline const MemHandle & getData() const
        {
            return _data;
        }

    private:
        /// @brief The MemHandle that this pbuf references.
        /// This MemHandle will be deleted when customFreeFunc is called.
        const MemHandle _data;

        /// @brief C function to free a pbuf_custom_MemHandle
        /// @param [in] buffer The pbuf_custom_MemHandle instance. Cannot not be 0!
        inline static void customFreeFunc ( struct pbuf * buffer )
        {
            assert ( buffer != 0 );

            if ( !buffer )
                return;

            assert ( ( buffer->flags & PBUF_FLAG_IS_CUSTOM ) != 0 );

            pbuf_custom_MemHandle * custom = reinterpret_cast<pbuf_custom_MemHandle *> ( buffer );

            assert ( ( void * ) custom == ( void * ) &custom->pbuf );

            delete custom;
        }
};
}
