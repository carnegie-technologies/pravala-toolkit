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

import java.io.File;
import java.io.IOException;

/**
 * A class for creating a listening UNIX-domain socket with a filesystem path.
 */
public class LocalPathServerSocket extends android.net.LocalServerSocket
{
    /**
     * The listening socket path
     */
    private final String sockPath;

    /**
     * Creates a new server socket listening at specified socket path.
     * This method always removes the socket path first before creating the socket.
     * And the socket path will be removed after the socket is closed.
     * @param sockPath The listening socket path
     * @throws IOException if the socket cannot be created
     * @throws NoSuchFieldException if the FileDescriptor.descriptor field cannot be found
     * @throws IllegalAccessException if the FileDescriptor.descriptor field cannot be assigned
     */
    public LocalPathServerSocket ( String sockPath )
    throws IOException, NoSuchFieldException, IllegalAccessException
    {
        super ( SocketUtils.createLocalListeningSocket ( sockPath ) );

        this.sockPath = sockPath;
    }

    @Override
    public void close() throws IOException
    {
        super.close();

        new File ( sockPath ).delete();
    }
}
