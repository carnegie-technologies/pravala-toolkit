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

package com.pravala.protocol.ctrl;

import android.net.LocalSocket;
import android.net.LocalSocketAddress;

import com.pravala.protocol.CodecException;
import com.pravala.protocol.auto.ctrl.Request;
import com.pravala.protocol.auto.ctrl.Update;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;

/**
 * Class that contains static method(s) for making simple blocking ctrl requests.
 */
public class SimpleBlockingRequest
{
    /**
     * Class name, for logging.
     */
    private static final String TAG = SimpleBlockingRequest.class.getName();

    /**
     * The next request ID that is used by makeRequest().
     * It is incremented after being used.
     * This is only used to listen for responses with the matching ID.
     */
    private static int nextRequestId = 1;

    /**
     * Send a ctrl request then wait for a matching response.
     * This always blocks until the response is received (or until the timeout).
     *
     * @param request The ctrl request to make.
     * @param sockPath The local socket path to connect to.
     * @param sockMode Whether the local socket path is a filesystem path or an abstract path.
     * @param timeoutMs The max time to wait for the response, 0 or less means infinite time.
     * @return The response on success, this should be decoded into the specific response msg.
     *         Otherwise null on error or timeout.
     */
    public static Update makeRequest (
            Request request, String sockPath, LocalSocketAddress.Namespace sockMode, int timeoutMs )
    {
        // This means the response to the request will always be generated, even if the operation is successful.
        request.setRequestResponse ( true );

        // We use the request ID to listen for responses with the matching ID; any other responses are discarded.
        int requestId;

        synchronized ( SimpleBlockingRequest.class )
        {
            requestId = nextRequestId++;
        }

        request.setRequestId ( requestId );

        LocalSocket sock = new LocalSocket();

        final long startTime = System.currentTimeMillis();
        final long endTime = ( timeoutMs > 0 ) ? ( startTime + timeoutMs ) : Long.MAX_VALUE;

        try
        {
            // We call getInputStream() first to force the internal socket impl to get created, otherwise
            // the setSoTimeout() call below will fail because there is no impl. This is to work around the
            // weird lazy behaviour of Java socket implementations.
            InputStream is = sock.getInputStream();

            if ( timeoutMs > 0 )
            {
                sock.setSoTimeout ( timeoutMs );
            }

            sock.connect ( new LocalSocketAddress ( sockPath, sockMode ) );

            request.serializeWithLength ( sock.getOutputStream() );

            Update resp = new Update();

            while ( resp.deserializeWithLength ( is ) > 0 )
            {
                if ( resp.getRequestId() == requestId )
                {
                    return resp;
                }

                if ( ( timeoutMs > 0 ) && ( System.currentTimeMillis() >= endTime ) )
                {
                    Log.d ( TAG, "Timeout (" + timeoutMs
                               + " ms) reached waiting for response to Request msg with type "
                               + request.getType() + "; Returning null" );

                    return null;
                }

                resp = new Update();
            }

            Log.d ( TAG, "End of stream reached without decoding a valid response; Returning null" );

            return null;
        }
        catch ( IOException | CodecException e )
        {
            e.printStackTrace();

            Log.e ( TAG, "Exception (" + e.toString() + ") while handling Request msg with type "
                       + request.getType() + "; Returning null" );

            return null;
        }
        finally
        {
            try
            {
                sock.close();
            }
            catch ( Exception e )
            {
            }
        }
    }
}
