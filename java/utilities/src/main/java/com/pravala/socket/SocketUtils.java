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

package com.pravala.socket;

import android.content.Context;
import android.net.LocalServerSocket;

import com.getkeepsafe.relinker.ReLinker;
import com.getkeepsafe.relinker.ReLinkerInstance;
import com.pravala.utilities.ObjectUtils;
import android.util.Log;

import java.io.FileDescriptor;
import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

/**
 * A class that contains some convenience methods for creating sockets
 *
 * The native libraries must be loaded before any of their methods can be used.
 * @see SocketUtils#loadLibraries(Context)
 */
public class SocketUtils
{
    /**
     * Class name, for logging.
     */
    private static final String TAG = SocketUtils.class.getName();

    /**
     * The ReLinker instance used for loading the native libraries.
     * Using a static ReLinkerInstance lets ReLinker skip libraries that have already been loaded.
     */
    private static ReLinkerInstance reLinker = ReLinker.log ( new ReLinker.Logger()
    {
        public void log ( String message )
        {
            Log.d ( TAG, message );
        }
    } );

    /**
     * Loads the native libraries required by this class. This must be called in a process before any of the
     * native libraries' functions can be called in that process. It is safe to call this function multiple
     * times within the same process.
     *
     * @note Typically, these would be loaded via System.loadLibrary() in this class's static initializer block.
     * However, a bug in older versions of the Android application updater sometimes prevents native libraries from
     * being properly extracted. Subsequent calls to System.loadLibrary() throw an UnsatisfiedLinkError exception.
     *
     * Using ReLinker allows us to load these libraries even if they were not properly extracted, but it requires
     * a Context to do so. This means the libraries must be loaded explicitly, rather than loaded automatically
     * alongside this class.
     *
     * @param context A context from the process in which we should load the shared libraries.
     */
    public synchronized static void loadLibraries ( Context context )
    {
        reLinker.loadLibrary ( context, "SocketUtils" );
    }

    /**
     * The errno value for "Function not implemented".
     */
    public static final int ENOSYS = 38;

    /**
     * Creates a local socket listening on the given socket file path.
     * This method always removes the socket path first before binding to the socket file path.
     * @param socketPath The socket file path to listen on
     * @return The new fd on success, -1 on failure
     */
    public static native int createLocalListeningSocketFd ( String socketPath );

    /**
     * Creates a local socket listening on the given socket file path.
     * This method always removes the socket path first before binding to the socket file path.
     * @param socketPath The socket file path to listen on
     * @return The new FileDescriptor on success, null on failure
     * @throws IOException if the socket cannot be created
     * @throws NoSuchFieldException if the FileDescriptor.descriptor field cannot be found
     * @throws IllegalAccessException if the FileDescriptor.descriptor field cannot be assigned
     */
    public static FileDescriptor createLocalListeningSocket ( String socketPath )
    throws IOException, NoSuchFieldException, IllegalAccessException
    {
        int fd = createLocalListeningSocketFd ( socketPath );

        if ( fd < 0 )
        {
            throw new IOException ( "Cannot create local listening socket on file path: " + socketPath );
        }

        FileDescriptor fileDescriptor = new FileDescriptor();
        Field field = FileDescriptor.class.getDeclaredField ( "descriptor" );
        field.setAccessible ( true );
        field.setInt ( fileDescriptor, fd );
        return fileDescriptor;
    }

    /**
     * Create a new LocalServerSocket listening at the specified name
     * @param name The socket name
     * @param abstractPath True if the name is in the Linux abstract namespace, false if on the filesystem
     * @return A new LocalServerSocket listening at the specified name
     * @throws IOException if the socket cannot be created
     * @throws NoSuchFieldException if the FileDescriptor.descriptor field cannot be found
     * @throws IllegalAccessException if the FileDescriptor.descriptor field cannot be assigned
     */
    public static LocalServerSocket createLocalServerSocket ( String name, boolean abstractPath )
    throws IOException, NoSuchFieldException, IllegalAccessException
    {
        if ( abstractPath )
        {
            return new LocalServerSocket ( name );
        }
        else
        {
            return new LocalPathServerSocket ( name );
        }
    }

    /**
     * Get the file descriptor integer value from a FileDescriptor object
     * @param fileDescriptor The FileDescriptor
     * @return The file descriptor integer on success, -1 on failure
     */
    public static int getFileDescriptor ( FileDescriptor fileDescriptor )
    {
        if ( fileDescriptor == null )
        {
            return -1;
        }

        try
        {
            Field fdField = fileDescriptor.getClass().getDeclaredField ( "descriptor" );
            fdField.setAccessible ( true );
            return ( Integer ) fdField.get ( fileDescriptor );
        }
        catch ( Exception e )
        {
        }

        return -1;
    }

    /**
     * Get the native file descriptor from a Java socket object.
     * This supports anything that inherits from Socket or DatagramSocket.
     * @param socket The socket object
     * @return The native file descriptor on success, -1 on failure
     */
    public static int getNativeFd ( Object socket )
    {
        if ( socket == null )
        {
            return -1;
        }

        try
        {
            Field implField = ObjectUtils.getFirstDeclaredField ( socket, "impl" );
            implField.setAccessible ( true );
            Object socketImpl = implField.get ( socket );

            Field fdField = ObjectUtils.getFirstDeclaredField ( socketImpl, "fd" );
            fdField.setAccessible ( true );
            return getFileDescriptor ( ( FileDescriptor ) fdField.get ( socketImpl ) );
        }
        catch ( Exception e )
        {
        }

        return -1;
    }

    /**
     * Call Android's bindSocketToNetwork() function via reflection.
     *
     * On Android 7.0+ attempting to use a private native library from the wrong namespace will
     * cause a runtime error (i.e. using System.loadLibrary or dlopen). See:
     * https://developer.android.com/about/versions/nougat/android-7.0-changes.html#ndk
     *
     * This Java function can call bindSocketToNetwork() with no problems because it simply reflects out
     * the android.net.NetworkUtils.bindSocketToNetwork() method which internally calls bindSocketToNetwork()
     * and is allowed to do so.
     *
     * Then native code can call this static Java method through JNI whenever we need bindSocketToNetwork().
     *
     * @deprecated This function is deprecated. We now use official
     *             Network.bindSocket(FileDescriptor) API, introduced in API-23.
     *             We still need this version, to maintain compatibility with API-21 and API-22.
     *             Earlier versions do things using routing rules.
     *
     * @param socketFd The socket FD to bind to the network.
     * @param netId The Android netId that identifies a network.
     * @return 0 on success; otherwise a standard errno code indicating the failure reason.
     */
    @Deprecated
    public static int bindSocketFd ( int socketFd, int netId )
    {
        try
        {
            Class< ? > networkUtilsClass
                = Class.forName ( "android.net.NetworkUtils", true, ClassLoader.getSystemClassLoader() );

            Method bindSocketToNetworkMethod = networkUtilsClass.getMethod (
                "bindSocketToNetwork", new Class[] { int.class, int.class } );

            // This directly calls bindSocketToNetwork. The first arg is the object with the method,
            // which is null in this case because it is a static method.
            int ret = ( Integer ) bindSocketToNetworkMethod.invoke ( null, socketFd, netId );

            // When bindSocketToNetwork fails (returns <0), the return code is a negative errno value.
            if ( ret < 0 )
                ret = -ret;

            return ret;
        }
        catch ( Exception e )
        {
            e.printStackTrace();

            return ENOSYS;
        }
    }
}
