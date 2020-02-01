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

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>

extern "C"
{
#include <pthread.h>
}

#include "basic/Random.hpp"
#include "basic/Mutex.hpp"
#include "socket/PacketDataStore.hpp"

using namespace Pravala;

// mutex controlling access to the test array
static Mutex testMutex ( "DataStoreTest" );

// test array of MemHandle objects
static MemHandle * testArray = 0;

// current index in the array
static int testIndex = 0;

// number of threads to run
static int numThreads = 0;

// number of allocations by each thread
static int numAllocs = 0;

// the size of the test array
static int arraySize = 0;

// the sleep delay between allocations (in ms)
static int sleepDelay = 0;

static void * run ( void * ptr )
{
    const int threadNum = reinterpret_cast<unsigned long> ( ptr );

    printf ( "Thread %d is initializing the data store...\n", threadNum );
    PacketDataStore::init();
    printf ( "Thread %d initialized the data store\n", threadNum );

    for ( int idx = 0; idx < numAllocs; ++idx )
    {
        testMutex.lock();

        printf ( "Thread %d is allocating packet at index %d; Previous size: %d\n",
                 threadNum, testIndex, ( int ) testArray[ testIndex ].size() );

        testArray[ testIndex ] = PacketDataStore::getPacket();

        printf ( "Thread %d allocated packet at index %d; New size: %d\n",
                 threadNum, testIndex, ( int ) testArray[ testIndex ].size() );

        testIndex = ( testIndex + 1 ) % arraySize;

        testMutex.unlock();

        if ( sleepDelay > 0 )
        {
            // Let's sleep between 0.5 and 1.5 of sleep delay:

            int delay = ( sleepDelay / 2 ) + Random::rand ( sleepDelay + 1 );

            if ( delay < 1 )
                delay = 1;

            printf ( "Thread %d sleeping for %d ms\n", threadNum, delay );

            usleep ( delay * 1000 );

            printf ( "Thread %d is done sleeping\n", threadNum );
        }
    }

    PacketDataStore::shutdown();
    return 0;
}

int main ( int argc, char * argv[] )
{
    if ( argc != 5
         || !String ( argv[ 1 ] ).toNumber ( numThreads )
         || numThreads < 1
         || !String ( argv[ 2 ] ).toNumber ( numAllocs )
         || numAllocs < 1
         || !String ( argv[ 3 ] ).toNumber ( arraySize )
         || arraySize < 1
         || !String ( argv[ 4 ] ).toNumber ( sleepDelay )
         || sleepDelay < 0 )
    {
        fprintf ( stderr, "Usage: %s number_of_threads number_of_allocs_per_thread test_array_size sleep_delay_ms\n",
                  argv[ 0 ] );
        fprintf ( stderr, "If sleep delay is not 0, the actual sleep time will be random, "
                  "between 0.5 and 1.5 of delay used.\n" );

        return EXIT_FAILURE;
    }

    testArray = new MemHandle[ arraySize ];

    if ( !testArray )
    {
        fprintf ( stderr, "Error allocating test array of size %d\n", arraySize );
        return EXIT_FAILURE;
    }

    pthread_t * threads = new pthread_t[ numThreads ];

    if ( !threads )
    {
        fprintf ( stderr, "Error allocating array of pthread_t values of size %d\n", numThreads );
        return EXIT_FAILURE;
    }

    memset ( threads, 0, sizeof ( pthread_t ) * numThreads );

    // We want to lock it before we start creating threads!
    testMutex.lock();

    for ( int i = 0; i < numThreads; ++i )
    {
        size_t s = i;

        if ( pthread_create ( threads + i, 0, run, ( void * ) s ) != 0 )
        {
            fprintf ( stderr, "Error creating thread %d: %s\n", i, strerror ( errno ) );
            return EXIT_FAILURE;
        }

        printf ( "Thread %d created\n", i );
    }

    printf ( "Running the test!\n" );

    testMutex.unlock();

    for ( int i = 0; i < numThreads; ++i )
    {
        void * ret;
        pthread_join ( threads[ i ], &ret );

        printf ( "Thread %d joined\n", i );
    }

    printf ( "All threads completed - cleaning up the memory\n" );

    for ( int i = 0; i < arraySize; ++i )
    {
        printf ( "Cleaning entry %d; Previous size: %d\n", i, ( int ) testArray[ i ].size() );
        testArray[ i ].clear();
    }

    delete[] testArray;
    testArray = 0;

    delete[] threads;
    threads = 0;

    printf ( "All done\n" );

    return EXIT_SUCCESS;
}
