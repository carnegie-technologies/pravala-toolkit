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

#include "basic/MemHandle.hpp"
#include "error/Error.hpp"

namespace Pravala
{
/// @brief Protocol decoder.
class ProtoDec
{
    public:
        /// @brief Constructor.
        /// @param [in] buf The buffer to use for decoding.
        ProtoDec ( const MemHandle & buf );

        /// @brief Decodes the data.
        /// @param [out] output The output lines. It is cleared first.
        /// @return Standard error code.
        ERRCODE decode ( StringList & output );

    private:
        const MemHandle _buf; ///< The memory to use.
        size_t _fieldIdWidth; ///< The width of the field ID value.
        size_t _fieldSizeWidth; ///< The width of the field size value.

        /// @brief Contains a single output entry
        struct Entry
        {
            String data; ///< The content line.
            size_t offFrom; ///< Start offset of the field/payload.
            size_t offTo; ///< End offset of the field/payload.
            size_t indent; ///< Indent level.
            bool isHdr; ///< Whether this contains a field header, or a payload

            /// @brief Constructor.
            /// @param [in] offF Start offset.
            /// @param [in] offT End offset.
            /// @param [in] i Indent level.
            /// @param [in] isH Whether this is a field header or a payload.
            Entry ( size_t offF, size_t offT, size_t i, bool isH ):
                offFrom ( offF ), offTo ( offT ), indent ( i ), isHdr ( isH )
            {
            }

            /// @brief Sets just the content line.
            /// @param [in] val The new content to set.
            /// @return A reference to this entry.
            Entry & set ( const String & val )
            {
                data = val;
                return *this;
            }
        };

        /// @brief Decode a data starting at a given offset.
        /// @param [in] idPath The ID path so far.
        /// @param [in] offset Start offset of the data.
        /// @param [in] dataSize The size of the data to decode (starting from the given offset).
        /// @param [in] indent Indent level.
        /// @param [out] output The output to append to. It is not cleared.
        /// @return Standard error code.
        ERRCODE decodeData (
            const String & idPath,
            size_t offset, size_t dataSize,
            size_t indent, List<Entry> & output );

        /// @brief Adds the content of a single field to output.
        /// @param [in] idPath The ID/ID path of the field.
        /// @param [in] hdrOffset Start offset of field's header.
        /// @param [in] offset Start offset of field's payload.
        /// @param [in] fieldId The ID of this field.
        /// @param [in] fieldSize The size of this field's payload.
        /// @param [in] wireType Wire type used by this field.
        /// @param [in] indent Indent level.
        /// @param [out] output The output to append to. It is not cleared.
        void dumpField (
            const String & idPath,
            size_t hdrOffset, size_t offset, uint8_t fieldId, size_t fieldSize, uint8_t wireType,
            size_t indent, List<Entry> & output );

        /// @brief Adds the content of a value of the field to the output.
        /// @param [in] idPath The ID/ID path of the field.
        /// @param [in] hdrOffset Start offset of field's header.
        /// @param [in] offset Start offset of field's payload.
        /// @param [in] fieldId The ID of this field.
        /// @param [in] fieldSize The size of this field's payload.
        /// @param [in] wireType Wire type used by this field.
        /// @param [in] indent Indent level.
        /// @param [out] output The output to append to. It is not cleared.
        /// @param [out] inlineValue Set to true if the value generated can be used in inline mode; False otherwise.
        void dumpFieldValue (
            const String & idPath,
            size_t hdrOffset, size_t offset, uint8_t fieldId, size_t fieldSize, uint8_t wireType,
            size_t indent, List<Entry> & output, bool & inlineValue );

        /// @brief Adds the binary dump of a field's value to output.
        /// @param [in] hdrOffset Start offset of field's header.
        /// @param [in] offset Start offset of field's payload.
        /// @param [in] fieldSize The size of this field's payload.
        /// @param [in] indent Indent level.
        /// @param [out] output The output to append to. It is not cleared.
        void dumpData (
            size_t hdrOffset, size_t offset, size_t fieldSize,
            size_t indent, List<Entry> & output );

        /// @brief Returns the name of the wire type.
        /// @param [in] wireType the wire type.
        /// @return The name of the given wire type.
        static const char * getWireType ( uint8_t wireType );
};
}
