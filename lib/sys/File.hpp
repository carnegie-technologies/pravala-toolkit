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
/// @brief Class that contains static file utility methods
class File
{
    public:
        /// @brief Reads data from the file and appends it to the buffer
        /// @param [in] filePath The path of the file to read from
        /// @param [out] buffer The buffer to read the data into
        /// @return Standard error code
        inline static ERRCODE read ( const String & filePath, MemHandle & buffer )
        {
            return File::read ( filePath.c_str(), buffer );
        }

        /// @brief Reads data from the file and appends it to this buffer
        /// @param [in] filePath The path of the file to read from
        /// @param [out] buffer The buffer to read the data into
        /// @return Standard error code
        static ERRCODE read ( const char * filePath, MemHandle & buffer );

        /// @brief Reads data from the gzip file and appends it to this buffer
        /// @param [in] filePath The path of the file to read from
        /// @param [out] buffer The buffer to read the data into
        /// @return Standard error code
        static ERRCODE readGzip ( const char * filePath, MemHandle & buffer );

        /// @brief Writes content of the buffer to a file
        /// @param [in] buffer Buffer to write
        /// @param [in] filePath The path of the file to write to
        /// @param [in] appendToFile If set to true, the data will be appended to the file. Otherwise the content
        ///                     of the file (if any) is overwritten (this is the default).
        /// @param [in] createDirs If this flag is set to true (it's false by default), when the output file
        ///                         cannot be created, the directories in the path will be created first,
        ///                         and file opening will be retried. If it still fails, the directories that
        ///                         may have been created are NOT cleaned up.
        /// @return Standard error code
        static ERRCODE write (
            const Buffer & buffer, const String & filePath,
            bool appendToFile = false, bool createDirs = false );

        /// @brief Writes content of the buffer to a file
        /// @param [in] handle Data to write
        /// @param [in] filePath The path of the file to write to
        /// @param [in] appendToFile If set to true, the data will be appended to the file. Otherwise the content
        ///                     of the file (if any) is overwritten (this is the default).
        /// @param [in] createDirs If this flag is set to true (it's false by default), when the output file
        ///                         cannot be created, the directories in the path will be created first,
        ///                         and file opening will be retried. If it still fails, the directories that
        ///                         may have been created are NOT cleaned up.
        /// @return Standard error code
        static ERRCODE write (
            const MemHandle & handle, const String & filePath,
            bool appendToFile = false, bool createDirs = false );

        /// @brief Writes a string to a file
        /// @param [in] str The string to write
        /// @param [in] filePath The path of the file to write to
        /// @param [in] appendToFile If set to true, the data will be appended to the file. Otherwise the content
        ///                     of the file (if any) is overwritten (this is the default).
        /// @param [in] createDirs If this flag is set to true (it's false by default), when the output file
        ///                         cannot be created, the directories in the path will be created first,
        ///                         and file opening will be retried. If it still fails, the directories that
        ///                         may have been created are NOT cleaned up.
        /// @return Standard error code
        static ERRCODE write (
            const String & str, const String & filePath,
            bool appendToFile = false, bool createDirs = false );

        /// @brief Writes data to a file
        /// @param [in] data Data to write
        /// @param [in] dataSize The size of the data to write
        /// @param [in] filePath The path of the file to write to
        /// @param [in] appendToFile If set to true, the data will be appended to the file. Otherwise the content
        ///                     of the file (if any) is overwritten (this is the default).
        /// @param [in] createDirs If this flag is set to true (it's false by default), when the output file
        ///                         cannot be created, the directories in the path will be created first,
        ///                         and file opening will be retried. If it still fails, the directories that
        ///                         may have been created are NOT cleaned up.
        /// @return Standard error code
        static ERRCODE write (
            const char * data, size_t dataSize, const char * filePath,
            bool appendToFile = false, bool createDirs = false );

        /// @brief Tests whether a given file exists
        /// @param [in] filePath The path to the file
        /// @return True if this file exists (and can be opened); False otherwise
        static bool exists ( const char * filePath );

        /// @brief Tests whether a given file exists
        /// @param [in] filePath The path to the file
        /// @return True if this file exists (and can be opened); False otherwise
        static inline bool exists ( const String & filePath )
        {
            return exists ( filePath.c_str() );
        }

        /// @brief Tests whether a given file can be opened in append mode
        /// @param [in] filePath The path to the file
        /// @param [in] removeAfter True to remove the file after testing, false to leave the file
        /// @return True if this file can be opened appendable; False otherwise
        static bool isWritable ( const char * filePath, bool removeAfter );

        /// @brief Tests whether a given file can be opened in append mode
        /// @param [in] filePath The path to the file
        /// @param [in] removeAfter True to remove the file after testing, false to leave the file
        /// @return True if this file can be opened appendable; False otherwise
        static inline bool isWritable ( const String & filePath, bool removeAfter )
        {
            return isWritable ( filePath.c_str(), removeAfter );
        }

        /// @brief Try to remove a file
        /// @param [in] filePath The path to the file
        /// @return True if this file was removed successfully
        static bool remove ( const char * filePath );

        /// @brief Try to remove a file
        /// @param [in] filePath The path to the file
        /// @return True if this file was removed successfully
        static inline bool remove ( const String & filePath )
        {
            return remove ( filePath.c_str() );
        }
};
}
