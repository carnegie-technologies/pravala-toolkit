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

#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include <jni.h>
#include <android/log.h>

#include "SocketUtils.h"

/**
 * Logging tag.
 */
#define TAG    "SocketUtilsImpl"

/// @brief Creates a local socket listening on the given socket file path.
/// This method always removes the socket path first before binding to the socket file path.
/// @param [in] env The JNI environment pointer
/// @param [in] obj The calling Java object
/// @param [in] socketPath The socket file path to listen on
/// @return The new fd on success, -1 on failure
JNIEXPORT jint JNICALL Java_com_pravala_socket_SocketUtils_createLocalListeningSocketFd (
        JNIEnv * env, jclass obj,
        jstring sockPath )
{
    assert ( sockPath != 0 );

    const char * sockPathStr;

    if ( ( sockPathStr = ( *env )->GetStringUTFChars ( env, sockPath, 0 ) ) == 0 )
    {
        __android_log_print ( ANDROID_LOG_DEBUG, TAG, "null sockPath" );
        return -1;
    }

    // We always need to remove the socket path first, in case it was left behind from a previous run
    unlink ( sockPathStr );

    const int sockPathLen = strlen ( sockPathStr );

    struct sockaddr_un addr;

    if ( sockPathLen + 1 > sizeof ( addr.sun_path ) )
    {
        __android_log_print ( ANDROID_LOG_DEBUG, TAG, "'%s' is too long to be a local socket path", sockPathStr );

        ( *env )->ReleaseStringUTFChars ( env, sockPath, sockPathStr );
        return -1;
    }

    memset ( &addr, 0, sizeof ( addr ) );

    addr.sun_family = AF_LOCAL;
    strncpy ( addr.sun_path, sockPathStr, sizeof ( addr.sun_path ) );

    ( *env )->ReleaseStringUTFChars ( env, sockPath, sockPathStr );
    sockPathStr = 0;

    int fd = socket ( AF_LOCAL, SOCK_STREAM, 0 );

    if ( fd < 0 )
    {
        __android_log_print ( ANDROID_LOG_DEBUG, TAG,
                              "Failed to create a local socket: [%d] %s", errno, strerror ( errno ) );

        return -1;
    }

    if ( bind ( fd, ( struct sockaddr * ) &addr, sizeof ( addr.sun_family ) + sockPathLen ) < 0 )
    {
        __android_log_print ( ANDROID_LOG_DEBUG, TAG,
                              "Failed to bind local socket to '%s': [%d] %s",
                              sockPathStr, errno, strerror ( errno ) );

        close ( fd );
        return -1;
    }

    if ( listen ( fd, 1 ) < 0 )
    {
        __android_log_print ( ANDROID_LOG_DEBUG, TAG,
                              "Failed to listen to local socket at '%s': [%d] %s",
                              sockPathStr, errno, strerror ( errno ) );

        close ( fd );
        return -1;
    }

    return fd;
}
