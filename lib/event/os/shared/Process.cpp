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

#include <cassert>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <csignal>

extern "C"
{
#include <unistd.h>
#include <sys/stat.h>
}

#include "../../Process.hpp"

#define DEFAULT_PROC_READ    256

using namespace Pravala;

/// @brief Creates a new memory array with the environment entry in 'name = value' form
/// @param [in] name The name of the entry
/// @param [in] value The value of the entry
/// @return The memory (that should be deleted[] by the caller) that contains the 'name=value'
///          null-terminated string for the given parameters.
static char * makeEnvEntry ( const char * name, const char * value )
{
    // +1 for '='
    int len = strlen ( name ) + strlen ( value ) + 1;

    char * ret = new char[ len + 1 ];

    snprintf ( ret, len + 1, "%s=%s", name, value );

    ret[ len ] = 0;

    return ret;
}

/// @brief Creates a new memory array with the copy of the value provided.
/// @param [in] value The value to copy
/// @return The memory (that should be deleted[] by the caller) that contains the copy of 'value'
static char * makeCopy ( const char * value )
{
    int len = strlen ( value );

    char * ret = new char[ len + 1 ];

#ifdef SYSTEM_UNIX
    strlcpy ( ret, value, len + 1 );
#else
    strcpy ( ret, value );
#endif

    ret[ len ] = 0;

    return ret;
}

Process::Process():
    _waitingToFinishStatus ( NotStarted ),
    _status ( NotStarted ),
    _valExitStatus ( 0 ),
    _valSignal ( 0 ),
    _nextOutReadSize ( DEFAULT_PROC_READ ),
    _nextErrReadSize ( DEFAULT_PROC_READ ),
    _inFd ( -1 ),
    _outFd ( -1 ),
    _errFd ( -1 ),
    _pid ( 0 ),
    _readyToWrite ( false )
{
}

Process * Process::generate ( ProcessOwner * owner, const String & path, bool copyEnvironment )
{
    Process * ptr = getFromPool ( owner );

    assert ( ptr->_waitingToFinishStatus == NotStarted );
    assert ( ptr->_status == NotStarted );
    assert ( ptr->_valExitStatus == 0 );
    assert ( ptr->_valSignal == 0 );

    assert ( ptr->_outputBuf.isEmpty() );
    assert ( ptr->_errorBuf.isEmpty() );
    assert ( ptr->_inputQueue.isEmpty() );

    assert ( ptr->_inFd == -1 );
    assert ( ptr->_outFd == -1 );
    assert ( ptr->_errFd == -1 );
    assert ( ptr->_pid == 0 );
    assert ( ptr->_readyToWrite == false );

    ptr->_waitingToFinishStatus = NotStarted;
    ptr->_status = NotStarted;
    ptr->_workingDir.clear();
    ptr->_path = path;
    ptr->_args.clear();
    ptr->_env.clear();

    ptr->_nextOutReadSize = DEFAULT_PROC_READ;
    ptr->_nextErrReadSize = DEFAULT_PROC_READ;

    if ( copyEnvironment )
        copyCurrentEnvironment ( ptr->_env );

    return ptr;
}

Process::~Process()
{
    // Just in case...
    returnsToPool();
}

void Process::returnsToPool()
{
    doUnregisterAll();
    _pid = 0;

    _inputQueue.clear();
    _outputBuf.clear();
    _errorBuf.clear();

    _waitingToFinishStatus = NotStarted;
    _status = NotStarted;
    _valExitStatus = 0;
    _valSignal = 0;
    _readyToWrite = false;
}

void Process::doUnregisterFds()
{
    if ( _inFd >= 0 )
        EventManager::closeFd ( _inFd );

    if ( _outFd >= 0 )
        EventManager::closeFd ( _outFd );

    if ( _errFd >= 0 )
        EventManager::closeFd ( _errFd );

    _inFd = -1;
    _outFd = -1;
    _errFd = -1;
}

void Process::doUnregisterAll()
{
    doUnregisterFds();
    doUnregisterChild();
}

void Process::doUnregisterChild()
{
    if ( _pid > 0 )
        EventManager::removeChildHandler ( _pid );
}

String Process::findProgramPath ( const String & programName )
{
    if ( programName.isEmpty() )
        return "";

    StringList paths = getEnvValue ( "PATH" ).split ( ":" );

    for ( size_t i = 0; i < paths.size(); ++i )
    {
        String testPath = paths[ i ];
        testPath.append ( "/" );
        testPath.append ( programName );

        struct stat fStat;

        if ( stat ( testPath.c_str(), &fStat ) == 0
             && S_ISREG ( fStat.st_mode )
             && ( fStat.st_mode & ( S_IXOTH | S_IXGRP | S_IXUSR ) ) != 0 )
        {
            return testPath;
        }
    }

    return "";
}

ERRCODE Process::run()
{
    if ( _status != NotStarted )
        return Error::WrongState;

    if ( !getOwner() )
        return Error::NoOwner;

    assert ( _inFd == -1 );
    assert ( _outFd == -1 );
    assert ( _errFd == -1 );

    if ( !_workingDir.isEmpty() )
    {
        struct stat fStat;

        if ( stat ( _workingDir.c_str(), &fStat ) != 0
             || !S_ISDIR ( fStat.st_mode ) )
        {
            return Error::ConfigError;
        }
    }

    int inputPipe[ 2 ];
    int outputPipe[ 2 ];
    int errorPipe[ 2 ];

    if ( pipe ( inputPipe ) )
    {
        return Error::PipeFailed;
    }

    if ( pipe ( outputPipe ) )
    {
        close ( inputPipe[ 0 ] );
        close ( inputPipe[ 1 ] );

        return Error::PipeFailed;
    }

    if ( pipe ( errorPipe ) )
    {
        close ( inputPipe[ 0 ] );
        close ( inputPipe[ 1 ] );
        close ( outputPipe[ 0 ] );
        close ( outputPipe[ 1 ] );

        return Error::PipeFailed;
    }

    _pid = fork();

    if ( _pid < 0 )
        return Error::ForkFailed;

    if ( _pid > 0 )
    {
        // Parent

        // close the read end of Parent->Child (std input) pipe
        close ( inputPipe[ 0 ] );
        _inFd = inputPipe[ 1 ];

        // close the write end of Child->Parent pipes
        // (std error and output)
        close ( outputPipe[ 1 ] );
        _outFd = outputPipe[ 0 ];

        assert ( _outFd > 0 );

        close ( errorPipe[ 1 ] );
        _errFd = errorPipe[ 0 ];

        EventManager::setFdHandler ( _inFd, this, EventManager::EventWrite );
        EventManager::setFdHandler ( _outFd, this, EventManager::EventRead );
        EventManager::setFdHandler ( _errFd, this, EventManager::EventRead );

        EventManager::setChildHandler ( _pid, this );

        _status = Running;

        return Error::Success;
    }

    assert ( _pid == 0 );
    // Child

    // close the write end of Parent->Child (std input) pipe
    close ( inputPipe[ 1 ] );

    // close the read end of Child->Parent pipes
    // (std error and output)
    close ( outputPipe[ 0 ] );
    close ( errorPipe[ 0 ] );

    if ( dup2 ( inputPipe[ 0 ], STDIN_FILENO ) != STDIN_FILENO )
    {
        perror ( "dup2(STDIN)" );
        abort();
    }

    if ( dup2 ( outputPipe[ 1 ], STDOUT_FILENO ) != STDOUT_FILENO )
    {
        perror ( "dup2(STDOUT)" );
        abort();
    }

    if ( dup2 ( errorPipe[ 1 ], STDERR_FILENO ) != STDERR_FILENO )
    {
        perror ( "dup2(STDERR)" );
        abort();
    }

    if ( !_workingDir.isEmpty() )
    {
        if ( chdir ( _workingDir.c_str() ) != 0 )
        {
            perror ( "chdir()" );
            abort();
        }
    }

    char ** childArgv = new char *[ _args.size() + 2 ];

    // We allocate new memory here, but we pass it to the new process.
    // it will by deallocated by the OS once that other process ends.
    childArgv[ 0 ] = makeCopy ( _path.c_str() );

    for ( size_t i = 0; i < _args.size(); ++i )
    {
        // We allocate new memory here, but we pass it to the new process.
        // it will by deallocated by the OS once that other process ends.
        childArgv[ i + 1 ] = makeCopy ( _args[ i ].c_str() );
    }

    childArgv[ _args.size() + 1 ] = 0;

    char ** childEnv = new char *[ _env.size() + 1 ];

    size_t idx = 0;

    for ( HashMap<String, String>::Iterator it ( _env ); it.isValid(); it.next() )
    {
        assert ( idx < _env.size() );

        // We allocate new memory here, but we pass it to the new process.
        // it will by deallocated by the OS once that other process ends.
        childEnv[ idx ] = makeEnvEntry ( it.key().c_str(), it.value().c_str() );

        ++idx;
    }

    childEnv[ _env.size() ] = 0;

    execve ( _path.c_str(), childArgv, childEnv );

    perror ( "execve()" );
    abort();

    assert ( false );
}

void Process::receiveFdEvent ( int fd, short events )
{
    assert ( fd == _inFd || fd == _outFd || fd == _errFd );

    assert ( getOwner() != 0 );

    if ( fd == _inFd && ( events & EventManager::EventWrite ) )
    {
        if ( !_inputQueue.isEmpty() )
        {
            MemHandle mem = _inputQueue.first();

            _inputQueue.removeFirst();

            assert ( mem.size() > 0 );

            ssize_t ret = write ( _inFd, mem.get(), mem.size() );

            if ( ret > 0 )
            {
                if ( ( size_t ) ret < mem.size() )
                {
                    _inputQueue.prepend ( mem.getHandle ( ( size_t ) ret ) );
                }
            }
            else if ( errno != EAGAIN && errno != EWOULDBLOCK )
            {
                // Something is wrong...
                /// @todo Check if we are doing what should be done.
                EventManager::disableWriteEvents ( _inFd );

                _status = WriteError;
                getOwner()->processStatusChanged ( this, _status );
            }
        }
        else
        {
            EventManager::disableWriteEvents ( _inFd );
            _readyToWrite = true;
        }
    }
    else if ( fd == _outFd && ( events & EventManager::EventRead ) )
    {
        assert ( _nextOutReadSize > 0 );

        char * const w = _outputBuf.getAppendable ( _nextOutReadSize );

        ssize_t ret = -1;

        if ( !w )
        {
            errno = ENOMEM;
        }
        else
        {
            ret = read ( _outFd, w, _nextOutReadSize );
        }

        if ( ret > 0 )
        {
            _outputBuf.markAppended ( ( size_t ) ret );
            getOwner()->processReadStd ( this, _outputBuf, ( size_t ) ret );

            // We don't clear the buffer
        }
        else if ( ret == 0 )
        {
            // We may be unreferenced inside the callback:
            fdClosed ( fd );
            return;
        }
        else if ( errno != EAGAIN && errno != EWOULDBLOCK )
        {
            EventManager::disableReadEvents ( _outFd );

            _status = ReadError;
            getOwner()->processStatusChanged ( this, _status );
        }
    }
    else if ( fd == _errFd && ( events & EventManager::EventRead ) )
    {
        assert ( _nextErrReadSize > 0 );

        char * const w = _errorBuf.getAppendable ( _nextErrReadSize );

        ssize_t ret = -1;

        if ( !w )
        {
            errno = ENOMEM;
        }
        else
        {
            ret = read ( _errFd, w, _nextErrReadSize );
        }

        if ( ret > 0 )
        {
            _errorBuf.markAppended ( ( size_t ) ret );
            getOwner()->processReadErr ( this, _errorBuf, ( size_t ) ret );

            // We don't clear the buffer
        }
        else if ( ret == 0 )
        {
            // We may be unreferenced inside the callback:
            fdClosed ( fd );
            return;
        }
        else if ( errno != EAGAIN && errno != EWOULDBLOCK )
        {
            EventManager::disableReadEvents ( _errFd );

            _status = ReadError;
            getOwner()->processStatusChanged ( this, _status );
        }
    }
    else
    {
        fprintf ( stderr, "Unknown Event Received: %d\n", events );
        assert ( false );
    }
}

// 'fd' should not be passed by a reference, if it is one of the descriptors
// we modify inside, it wouldn't work properly.
void Process::fdClosed ( int fd )
{
    assert ( fd >= 0 );
    assert ( fd == _inFd || fd == _outFd || fd == _errFd );

    if ( fd == _inFd )
    {
        _inFd = -1;
    }
    else if ( fd == _outFd )
    {
        _outFd = -1;
    }
    else if ( fd == _errFd )
    {
        _errFd = -1;
    }

    EventManager::closeFd ( fd );

    // If we're in 'waiting to finish' mode, we check whether we're still reading
    // from one of the read descriptors.
    // We ignore the STDIN descriptor, which, if the process is dead, doesn't really matter.
    // But even after the process died there still could be some data in STDOUT or STDERR
    // for us to read.
    if ( _waitingToFinishStatus != NotStarted && _outFd == -1 && _errFd == -1 )
    {
        // We are in 'waiting to finish' mode and there
        // is nothing else to read (both read descriptors are closed)
        // We can now tell our owner that we're done.

        doUnregisterFds();

        // Set the '_status' to the status at child exit
        // We want to do this in case there were some temporary values of _status
        // due to read/write errors. The owner would have been notified about them already.
        // Here we want to say that the actual process ended (and how).
        _status = _waitingToFinishStatus;

        getOwner()->processStatusChanged ( this, _status );
    }
}

void Process::receiveChildEvent ( int childPid, int childStatus, int statusValue )
{
    bool ended = false;

    ( void ) childPid;
    assert ( childPid == _pid );

    _valExitStatus = _valSignal = 0;

    switch ( childStatus )
    {
        case EventManager::ChildExited:
            _status = Succeeded;
            _valExitStatus = statusValue;
            ended = true;
            break;

        case EventManager::ChildSignal:
            _status = Interrupted;
            _valSignal = statusValue;
            ended = true;
            break;

        case EventManager::ChildStopped:
            _status = Stopped;
            _valSignal = statusValue;
            break;

        case EventManager::ChildContinued:
            _status = Running;
            break;

        default:
            assert ( false );
            abort();
            break;
    }

    assert ( getOwner() != 0 );

    if ( ended )
    {
        // We ended. We need to unregister the child process
        doUnregisterChild();

        // Are we still reading from the sockets?
        // We ignore the STDIN descriptor, which, if the process is dead, doesn't really matter.
        // But even after the process died there still could be some data in STDOUT or STDERR
        // for us to read.
        if ( _outFd == -1 && _errFd == -1 )
        {
            // Nope. Tell the owner!

            doUnregisterFds();

            getOwner()->processStatusChanged ( this, _status );
        }
        else
        {
            // We can't tell our owner that we're done yet.
            // We may still be reading data from the descriptors!
            // But let's just mark that we should exit once we're done reading.
            _waitingToFinishStatus = _status;

            assert ( _waitingToFinishStatus != NotStarted );
        }
    }
    else
    {
        // We didn't end yet - tell the owner
        getOwner()->processStatusChanged ( this, _status );
    }
}

void Process::setupNextOutReadSize ( size_t maxRead )
{
    if ( maxRead < 1 )
        return;

    _nextOutReadSize = maxRead;
}

void Process::setupOutputBuffer ( const Buffer & buffer )
{
    _outputBuf = buffer;
}

void Process::setupNextErrReadSize ( size_t maxRead )
{
    if ( maxRead < 1 )
        return;

    _nextErrReadSize = maxRead;
}

void Process::setupErrorBuffer ( const Buffer & buffer )
{
    _errorBuf = buffer;
}

ERRCODE Process::writeToInput ( const MemHandle & mem )
{
    if ( _status != Running )
        return Error::WrongState;

    if ( mem.isEmpty() )
        return Error::InvalidParameter;

    _inputQueue.append ( mem );

    if ( _readyToWrite )
    {
        _readyToWrite = false;
        EventManager::enableWriteEvents ( _inFd );
    }

    return Error::Success;
}

ERRCODE Process::closeProcInput()
{
    if ( _status != Running )
        return Error::WrongState;

    if ( _inFd < 0 )
        return Error::NothingToDo;

    EventManager::closeFd ( _inFd );
    _inFd = -1;

    return Error::Success;
}

ERRCODE Process::sendProcSig ( int signum )
{
    if ( _status != Running )
        return Error::WrongState;

    if ( kill ( _pid, signum ) != 0 )
        return Error::KillFailed;

    return Error::Success;
}

ERRCODE Process::killProc()
{
    return sendProcSig ( SIGKILL );
}

extern char ** environ;

void Process::copyCurrentEnvironment ( HashMap<String, String> & env )
{
    char ** envPtr = environ;

    assert ( envPtr != 0 );

    if ( !envPtr ) return;

    for ( ; *envPtr != 0; ++envPtr )
    {
        char * entry = *envPtr;

        assert ( entry != 0 );

        for ( int idx = 0; entry[ idx ] != 0; ++idx )
        {
            if ( entry[ idx ] == '=' )
            {
                if ( idx > 0 )
                {
                    env.insert ( String ( entry, idx ), String ( entry + idx + 1 ) );
                }

                break;
            }
        }
    }
}
