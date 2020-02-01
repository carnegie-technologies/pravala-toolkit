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

#include <cstring>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <ctime>
#include <csignal>

#include "PosixEventManager.hpp"

extern "C"
{
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
}

#ifdef USE_SIGNALFD
extern "C"
{
// signalfd from uClibc 0.9.32 refuses to build otherwise
#if defined( __UCLIBC__ ) && __UCLIBC_MAJOR__ == 0 && __UCLIBC_MINOR__ <= 9 && __UCLIBC_SUBLEVEL__ <= 32
#undef __THROW
#define __THROW
#endif

#include <sys/signalfd.h>
}
#endif

using namespace Pravala;

const int EventManager::SignalHUP ( SIGHUP );
const int EventManager::SignalUsr1 ( SIGUSR1 );
const int EventManager::SignalUsr2 ( SIGUSR2 );

/// @brief Set to true when we receive SIGINT/SIGTERM
bool PosixEventManager::_globalExit ( false );

#ifndef USE_SIGNALFD

/// @brief Number of SIGCHLDs to process
THREAD_LOCAL volatile uint32_t PosixEventManager::_globalSigChld ( 0 );

/// @brief Number of SIGHUPs to process
THREAD_LOCAL volatile uint32_t PosixEventManager::_globalSigHup ( 0 );

/// @brief Number of SIGUSR1s to process
THREAD_LOCAL volatile uint32_t PosixEventManager::_globalSigUsr1 ( 0 );

/// @brief Number of SIGUSR2s to process
THREAD_LOCAL volatile uint32_t PosixEventManager::_globalSigUsr2 ( 0 );

void PosixEventManager::posixSignalHandler ( int sigNum )
{
    if ( sigNum == SIGINT || sigNum == SIGTERM )
    {
        _globalExit = true;
    }
    else if ( sigNum == SIGCHLD )
    {
        // This should be an atomic operation (http://www.informit.com/guides/content.aspx?g=cplusplus&seqNum=469)
        ++_globalSigChld;
    }
    else if ( sigNum == SIGHUP )
    {
        ++_globalSigHup;
    }
    else if ( sigNum == SIGUSR1 )
    {
        ++_globalSigUsr1;
    }
    else if ( sigNum == SIGUSR2 )
    {
        ++_globalSigUsr2;
    }
}
#endif

PosixEventManager::PosixEventManager()
#ifdef USE_SIGNALFD
    : _signalFd ( -1 )
#endif
{
    if ( _isPrimaryManager )
    {
        // This is the first EventManager, block all signals, and have it enable signals on run()

        sigset_t sigMask;
        sigfillset ( &sigMask );

        // We don't want to block SIGPROF signal. If we did, profiling would not work.
        sigdelset ( &sigMask, SIGPROF );

        if ( sigprocmask ( SIG_BLOCK, &sigMask, 0 ) < 0 )
        {
            perror ( "PosixEventManager::PosixEventManager(): Error calling sigprocmask\n" );
        }
    }
}

PosixEventManager::~PosixEventManager()
{
#ifdef USE_SIGNALFD
    if ( _signalFd >= 0 )
    {
        ::close ( _signalFd );
        _signalFd = -1;
    }
#endif
}

bool PosixEventManager::initSignals()
{
    if ( !_isPrimaryManager )
        return false;

    sigset_t sigMask;

    // We will handle following signals:
    // SIGCHLD SIGINT SIGTERM SIGPIPE SIGHUP SIGUSR1 SIGUSR2
    sigemptyset ( &sigMask );
    sigaddset ( &sigMask, SIGCHLD );
    sigaddset ( &sigMask, SIGINT );
    sigaddset ( &sigMask, SIGTERM );
    sigaddset ( &sigMask, SIGPIPE );
    sigaddset ( &sigMask, SIGHUP );
    sigaddset ( &sigMask, SIGUSR1 );
    sigaddset ( &sigMask, SIGUSR2 );

#ifdef USE_SIGNALFD
    if ( _signalFd >= 0 )
    {
        ::close ( _signalFd );
        _signalFd = -1;
    }

    _signalFd = signalfd ( -1, &sigMask, 0 );

    if ( _signalFd < 0 )
    {
        perror ( "PosixEventManager:run(): Error calling signalfd\n" );

        return false;
    }

    int flags = fcntl ( _signalFd, F_GETFD, 0 );

    if ( flags < 0 )
        flags = 0;

    if ( fcntl ( _signalFd, F_SETFD, flags | FD_CLOEXEC ) < 0 )
    {
        perror ( "PosixEventManager:run(): Unable to fcntl signalfd with FD_CLOEXEC\n" );
    }
#else
    struct sigaction sa;

    sa.sa_handler = posixSignalHandler;

    sa.sa_flags = 0;

    sigemptyset ( &sa.sa_mask );

    sigaction ( SIGCHLD, &sa, NULL );
    sigaction ( SIGINT, &sa, NULL );
    sigaction ( SIGTERM, &sa, NULL );
    sigaction ( SIGPIPE, &sa, NULL );
    sigaction ( SIGHUP, &sa, NULL );
    sigaction ( SIGUSR1, &sa, NULL );
    sigaction ( SIGUSR2, &sa, NULL );

    if ( sigprocmask ( SIG_UNBLOCK, &sigMask, 0 ) < 0 )
    {
        perror ( "PosixEventManager::run(): Error calling sigprocmask\n" );
        return false;
    }
#endif

    return true;
}

bool PosixEventManager::runProcessSignals()
{
    if ( !_isPrimaryManager )
        return false;

    bool ret = false;
    bool gotSigChld = false;

#ifdef USE_SIGNALFD
    struct signalfd_siginfo sigInfo;

    ssize_t res = read ( _signalFd, &sigInfo, sizeof ( sigInfo ) );

    if ( res < 0 )
    {
        perror ( "PosixEventManager: read(_signalFd) failed\n" );
    }
    else if ( res != sizeof ( sigInfo ) )
    {
        fprintf ( stderr, "PosixEventManager: read(_signalFd) returned wrong size (%d) "
                  "instead of %d expected\n", ( int ) res, ( int ) sizeof ( sigInfo ) );
    }
    else if ( sigInfo.ssi_signo == SIGTERM )
    {
        printf ( "SIGTERM received. Exiting Event Manager\n" );

        _working = false;
        _globalExit = true;

        return true;
    }
    else if ( sigInfo.ssi_signo == SIGINT )
    {
        printf ( "SIGINT received. Exiting Event Manager\n" );

        _working = false;
        _globalExit = true;

        return true;
    }
    else if ( sigInfo.ssi_signo == SIGCHLD )
    {
        ret = true;
        gotSigChld = true;
    }
    else if ( sigInfo.ssi_signo == SIGPIPE )
    {
        // Ignoring...
        /// @todo Do we want to handle this somehow?

        ret = true;
    }
    else if ( sigInfo.ssi_signo == SIGHUP )
    {
        ret = true;
        notifySignalHandlers ( SignalHUP );
    }
    else if ( sigInfo.ssi_signo == SIGUSR1 )
    {
        ret = true;
        notifySignalHandlers ( SignalUsr1 );
    }
    else if ( sigInfo.ssi_signo == SIGUSR2 )
    {
        ret = true;
        notifySignalHandlers ( SignalUsr2 );
    }
    else
    {
        fprintf ( stderr, "PosixEventManager: Unexpected signal (%d) read from _signalFd\n",
                  sigInfo.ssi_signo );
    }
#else
    if ( _globalExit )
    {
        printf ( "Signal received. Exiting Event Manager.\n" );

        _working = false;

        return true;
    }

    if ( _globalSigChld > 0 )
    {
        ret = true;
        _globalSigChld = 0;
        gotSigChld = true;
    }

    if ( _globalSigHup > 0 )
    {
        ret = true;
        _globalSigHup = 0;
        notifySignalHandlers ( SignalHUP );
    }

    if ( _globalSigUsr1 > 0 )
    {
        ret = true;
        _globalSigUsr1 = 0;
        notifySignalHandlers ( SignalUsr1 );
    }

    if ( _globalSigUsr2 > 0 )
    {
        ret = true;
        _globalSigUsr2 = 0;
        notifySignalHandlers ( SignalUsr2 );
    }
#endif

    if ( gotSigChld )
    {
        runChildWait();
    }

    return ret;
}

int PosixEventManager::getSafeTimeout()
{
    const int baseTimeout = getTimeout();

    // There is already something in the end-of-loop queue, we should timeout right away!
    if ( !_loopEndQueue.isEmpty() )
        return 0;

    return baseTimeout;
}

void PosixEventManager::initFd ( int fd )
{
    assert ( fd >= 0 );

    if ( fd < 0 ) return;

    // Set close-on-exec, so fork() will close all the descriptors
    // that has been previously used by the EventManager (all of them should)
    int flags = fcntl ( fd, F_GETFD, 0 );

    if ( flags < 0 )
        flags = 0;

    if ( fcntl ( fd, F_SETFD, flags | FD_CLOEXEC ) < 0 )
    {
        fprintf ( stderr, "initFd: fcntl() failed: %s\n", strerror ( errno ) );
    }

    // Also set socket to non-blocking mode
    flags = fcntl ( fd, F_GETFL, 0 );

    if ( flags < 0 )
        flags = 0;

    if ( fcntl ( fd, F_SETFL, flags | O_NONBLOCK ) < 0 )
    {
        fprintf ( stderr, "initFd: fcntl() failed: %s\n", strerror ( errno ) );
    }
}

void PosixEventManager::runChildWait()
{
    int statVal = 0;
    pid_t pid = 0;

    while ( ( pid = waitpid ( ( pid_t ) -1, &statVal, WNOHANG ) ) > 0 )
    {
        assert ( pid > 0 );

        int childStatus = 0;
        int statusValue = 0;

        if ( WIFEXITED ( statVal ) )
        {
            childStatus = ChildExited;
            statusValue = WEXITSTATUS ( statVal );
        }
        else if ( WIFSIGNALED ( statVal ) )
        {
            childStatus = ChildSignal;
            statusValue = WTERMSIG ( statVal );
        }
        else if ( WIFSTOPPED ( statVal ) )
        {
            childStatus = ChildStopped;
            statusValue = WSTOPSIG ( statVal );
        }

#ifdef WIFCONTINUED
        else if ( WIFCONTINUED ( statVal ) )
        {
            childStatus = ChildContinued;
        }
#endif

        ChildEventHandler * cHandler = _childHandlers.value ( pid );

        if ( cHandler != 0 )
        {
            if ( childStatus != ChildStopped
                 && childStatus != ChildContinued )
            {
                _childHandlers.remove ( pid );
            }

            cHandler->receiveChildEvent ( pid, childStatus, statusValue );
        }
    }
}
