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
#include "basic/List.hpp"
#include "basic/Buffer.hpp"
#include "basic/MemHandle.hpp"
#include "error/Error.hpp"
#include "object/PooledOwnedObject.hpp"
#include "EventManager.hpp"

namespace Pravala
{
class Process;

/// @brief Interface to be inherited by all the objects that wish to start and receive
/// updates about child processes.
class ProcessOwner
{
    protected:
        /// @brief Callback called when the status of the child process chanes.
        /// @param [in] proc Pointer to the Process object responsible.
        /// @param [in] newStatus New status of the child process. One of "enum Status"
        virtual void processStatusChanged ( Process * proc, int newStatus ) = 0;

        /// @brief Callback called when some data has been read from the child process' standard output.
        /// @param [in] proc Pointer to the Process object responsible.
        /// @param [in] stdOut Buffer containing data received from the child process. If the owner doesn't clear
        ///                     it, the next read will append to the data already in the buffer.
        ///                     To consume part of the data and keep some to be appended to during the next read,
        ///                     a copies (or MemHandles) of the data to be consumed and the data to be kept for later
        ///                     should be created, this buffer should be cleared, and then appended to the required
        ///                     part of the data to.
        virtual void processReadStd ( Process * proc, RwBuffer & stdOut, size_t readBytes ) = 0;

        /// @brief Callback called when some data has been read from the child process' error output.
        /// @param [in] proc Pointer to the Process object responsible.
        /// @param [in] errOut Buffer containing data received from the child process. If the owner doesn't clear
        ///                     it, the next read will append to the data already in the buffer.
        ///                     To consume part of the data and keep some to be appended to during the next read,
        ///                     a copies (or MemHandles) of the data to be consumed and the data to be kept for later
        ///                     should be created, this buffer should be cleared, and then appended to the required
        ///                     part of the data to.
        virtual void processReadErr ( Process * proc, RwBuffer & errOut, size_t readBytes ) = 0;

        /// @brief Destructor
        virtual ~ProcessOwner()
        {
        }

        friend class Process;
};

/// @brief Process wrapper.
///
/// Makes using external processes easy. Handles all the logic behind connecting
/// input/outputs of the child process, subscribing to events on them, buffering,
/// and child exit monitoring.
class Process:
    public PooledOwnedObject<Process, ProcessOwner>,
    public EventManager::FdEventHandler,
    public EventManager::ChildEventHandler
{
    public:
        /// @brief Child process' status
        enum Status
        {
            NotStarted = 0, ///< Process has not been started yet
            Running = 1, ///< Process is running

            /// @brief Process has been stopped.
            ///
            /// getSignal() can be used to retrieve a value of the signal that caused that.
            Stopped = 2,
            Succeeded = 3, ///< Process has successfully finished.

            /// @brief Process has been killed by a signal.
            ///
            /// getSignal() can be used to retrieve a value of the signal that caused that.
            Interrupted = 4,
            Crashed = 5, ///< Process has crashed.

            /// @brief Read error has been detected
            ///
            /// This means that the process has crashed/finished - this status is temporary,
            /// it should almost immediately change to one of the other values (as soon as
            /// SIGCHLD is received).
            /// This also means that all the pipes to the process had been closed.
            ReadError = 6,

            /// @brief Write error has been detected
            ///
            /// This means that the process has crashed/finished - this status is temporary,
            /// it should almost immediately change to one of the other values (as soon as
            /// SIGCHLD is received).
            /// This also means that all the pipes to the process had been closed.
            WriteError = 7
        };

        /// @brief Returns the environment value associated with the key given
        /// @param [in] key The key to look for
        /// @return The value associated with that key, empty if not found
        static String getEnvValue ( const String & envKey );

        /// @brief Searches for the program name in the system's path
        /// Takes a name of the program and returns the full path to it
        /// @param [in] programName The name of the program
        /// @return Full path to the program, or an empty string if not found
        static String findProgramPath ( const String & programName );

        /// @brief Generates new Process object (or takes one from the pool)
        ///
        /// This function does not actually run the process, run() has to be called for that.
        /// @param [in] path Path to the program to run
        /// @param [in] owner Pointer to the owner of this Process. It will be receiving all status
        /// updates and read notifications.
        /// @param [in] copyEnvironment If set to true, a copy of the current environment is made
        ///  (those values can be modified later). If set to false, the initial environment
        ///  for the new process is empty.
        /// @return Process pointer.
        static Process * generate ( ProcessOwner * owner, const String & path, bool copyEnvironment );

        /// @brief Sets the size of the next read operation from std-out descriptor
        /// @param [in] maxRead The size of the next read operation
        void setupNextOutReadSize ( size_t maxRead );

        /// @brief Sets the size of the next read operation from err-out descriptor
        /// @param [in] maxRead The size of the next read operation
        void setupNextErrReadSize ( size_t maxRead );

        /// @brief Sets the buffer to be appended to upon the next std-output read
        /// Note that if there are some other writable references to this buffer's data,
        /// a copy will be created just before the read operation.
        /// @param [in] buffer The buffer to use
        void setupOutputBuffer ( const Buffer & buffer );

        /// @brief Sets the buffer to be appended to upon the next error-output read
        /// Note that if there are some other writable references to this buffer's data,
        /// a copy will be created just before the read operation.
        /// @param [in] buffer The buffer to use
        void setupErrorBuffer ( const Buffer & buffer );

        /// @brief Sets working directory
        /// When the child process is started, it changes its directory to the given value
        /// @param [in] workingDir The directory to run the process inside
        inline void setWorkingDir ( const String & workingDir )
        {
            _workingDir = workingDir;
        }

        /// @brief Returns reference to the path of the actual program.
        /// It can be used to modify program's path before it's started.
        /// After the program has been started, changing it has no effect.
        /// @return Program's path
        inline String & path()
        {
            return _path;
        }

        /// @brief Returns the reference to the list of program's arguments.
        /// It can be modified to append/change program's arguments before it is started.
        /// After the program has been started, changing it has no effect.
        /// @return Program's arguments
        inline StringList & arguments()
        {
            return _args;
        }

        /// @brief Returns the reference to the map of program's environment.
        ///
        /// It can be modified to add/change program's environment before it is started.
        /// for example environment().insert("PATH", "/usr/local/bin") sets the PATH
        /// variable.
        /// After the program has been started, changing it has no effect.
        /// @return Program's environment
        HashMap<String, String> & environment()
        {
            return _env;
        }

        /// @brief Returns program's status (one of Status enums)
        /// @return Program's status
        inline int getStatus() const
        {
            return _status;
        }

        /// @brief Returns program's exit status.
        /// If program finished properly, it could be the value returned by main() function,
        /// or passed to exit() function.
        /// @return Program's exit status
        inline int getExitStatus() const
        {
            return _valExitStatus;
        }

        /// @brief Returns the value of the signal that has been used to stop/kill the program.
        /// @return Value of the signal sent to the program to stop/kill it.
        inline int getSignal() const
        {
            return _valSignal;
        }

        /// @brief Returns program's PID
        /// @return Program's PID
        inline int getPid() const
        {
            return _pid;
        }

        /// @brief Sends a specified signal to the process.
        /// @param [in] signum Value of the signal to be sent to the process.
        /// @return Error code of the operation (Error::Success == 0 if no error)
        ERRCODE sendProcSig ( int signum );

        /// @brief Sends a kill signal to the process.
        /// @return Error code of the operation (Error::Success == 0 if no error)
        ERRCODE killProc();

        /// @brief Closes the pipe connected to process' standard input
        /// @return Error code of the operation (Error::Success == 0 if no error)
        ERRCODE closeProcInput();

        /// @brief Writes given data to process' standard input
        /// @return Error code of the operation (Error::Success == 0 if no error)
        ERRCODE writeToInput ( const MemHandle & mem );

        /// @brief Starts the program.
        ///
        /// This function rarely reports eny errors (except for starting program
        /// that has already been started or even exited). It reports problems with
        /// pipe or fork system calls, but not with exec call - this happens in the
        /// separate thread, and is reported as a child process' status update
        /// after it is updated.
        /// @return Error code of the operation (Error::Success == 0 if no error)
        ERRCODE run();

    protected:
        /// @brief Callback for receiving FD events from the child process.
        /// @param [in] fd File descriptor
        /// @param [in] events Types of events received.
        void receiveFdEvent ( int fd, short events );

        /// @brief Receives updates about child process' execution.
        /// @param [in] childPid PID of the child process updated
        /// @param [in] childStatus Status of the process
        /// @param [in] statusValue Either exit status or the value of the signal.
        void receiveChildEvent ( int childPid, int childStatus, int statusValue );

        /// @brief Creates a new Process object
        /// @return A new Process object, 0 if new failed
        inline static Process * generateNew()
        {
            return new Process();
        }

    private:
        /// @brief If set to something else than NotStarted it means we're in "waiting to finish" mode
        ///
        /// In this mode, when the last read descriptor is closed, we should set _status to this
        /// value, and notify the owner that we are done.
        int _waitingToFinishStatus;

        int _status; ///< Stores the status of the process (one of Status enum)
        int _valExitStatus; ///< Process' exit value
        int _valSignal; ///< Value of the signal that caused the process to change status.

        String _workingDir; ///< The directory to run the process in (just the child)
        String _path; ///< Path of the program
        StringList _args; ///< Commandline arguments of the program
        HashMap<String, String> _env; ///< Environment of the program.

        RwBuffer _outputBuf; ///< buffer with the data received from program's standard output.
        RwBuffer _errorBuf; ///< buffer with the data received from program's error output.

        size_t _nextOutReadSize; ///< The size of the next read operation from std-out descriptor
        size_t _nextErrReadSize; ///< The size of the next read operation from err-out descriptor

        List<MemHandle> _inputQueue;

        int _inFd; ///< File descriptor of the pipe connected to program's std input.
        int _outFd; ///< File descriptor of the pipe connected to program's std output.
        int _errFd; ///< File descriptor of the pipe connected to program's error output.
        int _pid; ///< Program's PID

        bool _readyToWrite; ///< Are we ready to write (set after EventWrite when there is nothing to write)

        /// @brief Default constructor.
        ///
        /// Creates empty buffer, which doesn't have any memory allocated.
        Process();

        /// @brief Destructor.
        ///
        /// Deallocates the memory.
        ~Process();

        /// @brief Called when this object is about to be returned to the pool.
        /// Does some cleanup.
        void returnsToPool();

        /// @brief Unregisters this object from both file descriptors and child monitoring (in the EventManager)
        void doUnregisterAll();

        /// @brief Unregisters this object from file descriptor monitoring in the EventManager
        void doUnregisterFds();

        /// @brief Unregisters this object from child status monitoring in the EventManager
        void doUnregisterChild();

        /// @brief Helper function, called when one of the file descriptors is closed.
        ///
        /// Checks whether we should notify the owner about the end of the process (if the child is already dead
        /// and we were just waiting for all read streams to end). It also calls EventManager::closeFd()
        /// @param [in] fd The file descriptor that just was closed.
        void fdClosed ( int fd );

        /// @brief Puts a copy of the current environment in the hash map provided
        /// @param [out] env The hash map to put the values from the process' current environment in.
        ///                  It only deals with 'name=value' entries in the environment,
        ///                  and uses the name as the key of the map.
        static void copyCurrentEnvironment ( HashMap<String, String> & env );

        /// @brief ObjectPool-related declarations
        POOLED_OWNED_OBJECT_HDR ( Process, ProcessOwner );
};
}
