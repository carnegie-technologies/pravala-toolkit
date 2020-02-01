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

#include "basic/String.hpp"
#include "ProtoSpec.hpp"
#include "FileObject.hpp"
#include "Symbol.hpp"

namespace Pravala
{
/// @brief A class representing a single language generator.
///
/// Contains some basic things, common to other generators.
/// To be inherited by specific language generators
class LanguageGenerator
{
    public:
        /// @brief The result of setting an option.
        enum SetOptResult
        {
            OptOkValueIgnored,  ///< Option set, the value was not used.
            OptOkValueConsumed, ///< Option set, the value was consumed.
            OptErrMissingValue, ///< Error, missing value.
            OptErrUnknownOption ///< Error, unknown option.
        };

        /// @brief Creates a new LanguageGenerator
        /// @param [in] proto The protocol specification object, it includes all the data read from
        ///                   the protocol description file (as a tree structure of symbols plus some other settings)
        LanguageGenerator ( ProtocolSpec & proto );

        /// @brief Checks if the generator should be run at all.
        /// The base version checks if the flag files (if configured) already exist.
        /// If either of them does, and --overwrite option is not used, an exception will be thrown.
        /// @note Instead of returning a code, an exception should be thrown to prevent the generator from running.
        virtual void canRun();

        /// @brief Runs the language generator
        ///
        /// This function calls procSymbol on the root of the ProtocolSpec object,
        /// and then it writes all the files generated (that were added to _fileObjects map) to the disk.
        virtual void run();

        /// @brief Configures the language generator after it has been created.
        /// It usually configures all reserved keywords and options.
        virtual void init();

        /// @brief Sets a specific command-line option.
        /// @param [in] shortName The short name of the option (if use, otherwise it's 0).
        /// @param [in] longName The long name of the option, or empty if not used.
        /// @param [in] value The value of the option, or empty if not provided.
        /// @return The status of the operation. On other errors an exception should be thrown.
        virtual SetOptResult setOption ( char shortName, const String & longName, const String & value );

        /// @brief Generates generator-specific help text.
        /// @return Generator-specific help text.
        virtual String getHelpText();

    protected:
        ProtocolSpec & _proto; ///< The protocol specification object to use

        String _baseOutDir; ///< The 'base' directory in which the output files are generated
        String _singleIndent; ///< The string representing a single indent level
        String _namespacePrefix; ///< Namespace prefix to use for all generated symbols

        String _flagPath; ///< The path to the flag file which will be created after generating the output.

        /// @brief The path to the flag file where current timestamp will be stored after generating the output.
        String _timeFlagPath;

        StringList _skipLeadingDirs; ///< The list of directories to skip at the beginning of generated paths.

        bool _overwriteFiles; ///< If true, output files will be overwritten.

        /// @brief Contains mapping file_path:file_object
        /// Contains all file objects generated so far.
        HashMap<String, FileObject *> _fileObjects;

        /// @brief Sets a specific command-line option.
        /// @note This version is used by the main setOption, and only deals with basic options,
        ///       common to all generators. Right now those are flag and overwrite options.
        /// @param [in] shortName The short name of the option (if use, otherwise it's 0).
        /// @param [in] longName The long name of the option, or empty if not used.
        /// @param [in] value The value of the option, or empty if not provided.
        /// @return The status of the operation. On other errors an exception should be thrown.
        SetOptResult setBasicOption ( char shortName, const String & longName, const String & value );

        /// @brief Generates generator-specific help text.
        /// @note This version is used by the main getHelpText, and only deals with basic options,
        ///       common to all generators. Right now those are flag and overwrite options.
        /// @return Basic help text.
        String getBasicHelpText();

        /// @brief To be called after successful code generation, to create flag files (if enabled).
        /// It will only generate those flags once, so running it multiple times is safe.
        void generateFlagFiles();

        /// @brief Adds a FileObject with a given path
        /// It throws an exception if the path is already used.
        /// @param [in] fileObject File object to use
        void addFile ( FileObject * fileObject );

        /// @brief Helper function that calls procSymbol() for every internal symbol of the given symbol.
        /// Internal symbols are processed in the order they were declared.
        /// @param [in] symbol Symbol whose internal symbols should be processed.
        void procInternalSymbols ( Symbol * symbol );

        /// @brief Called with an element to process
        ///
        /// Default implementation calls different proc*Symbol() functions,
        /// depending on the symbol's type.
        ///
        /// @param [in] symbol The symbol to process
        virtual void procSymbol ( Symbol * symbol );

        /// @brief Called for each namespace symbol to be processed
        ///
        /// Default implementation calls procInternalSymbols().
        ///
        /// @param [in] symbol The namespace symbol to be processed
        virtual void procNamespaceSymbol ( Symbol * symbol );

        /// @brief Called for each 'basic' type
        ///
        /// Basic types are primitive types that don't usually need any output generated,
        /// and don't contain any other symbols inside.
        ///
        /// The default implementation doesn't do anything
        ///
        /// @param [in] symbol The symbol to process
        virtual void procBasicSymbol ( Symbol * symbol );

        /// @brief Called for each regular type
        ///
        /// 'Regular' types are all messages (including base messages) and enumerators.
        /// This is not called for primitive types and namespaces.
        ///
        /// @param [in] symbol The symbol to process
        virtual void procRegularSymbol ( Symbol * symbol ) = 0;

        /// @brief Writes a file to a disk
        /// It refuses to overwrite existing files, unless _overwriteFiles is set to true.
        /// @param [in] path The path to write to
        /// @param [in] file The file object to write
        virtual void writeFile ( const String & path, FileObject * file );

        /// @brief Generates the output path for a file.
        /// The default implementation returns "_baseOutDir/file->getPath()".
        /// @param [in] file The file object to get the path for.
        /// @return The path to write the file object to.
        virtual String getOutputFilePath ( FileObject * file );

        /// @brief Creates a directory and all missing directories in the path.
        /// @param [in] path The path to create
        /// @param [in] skipLast If true, the last element in the path will be skipped.
        ///                      It should be true, if 'path' describes a file.
        static void createDirs ( const String & path, bool skipLast );
};
}
