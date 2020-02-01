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

extern "C"
{
#include <pthread.h>
#include <semaphore.h>
}

#include "basic/Mutex.hpp"
#include "event/EventManager.hpp"
#include "CorePacketWriter.hpp"
#include "PosixPacketWriterData.hpp"

struct mmsghdr;

namespace Pravala
{
/// @brief A class for writing data packets (UDP packets, IP packets) in a more efficient way.
/// It buffers packets being written, and writes more data at a time on, optionally, a separate thread.
class PosixPacketWriter:
    public CorePacketWriter,
    protected PosixPacketWriterData,
    protected EventManager::LoopEndEventHandler
{
    protected:
        /// @brief Internal flag set when we subscribe to end-of-loop events; Cleared when that event runs.
        static const uint16_t FlagEolSubscribed = ( 1 << 8 );

        /// @brief Internal flag set when the thread is started.
        static const uint16_t FlagThreadRunning = ( 1 << 9 );

        /// @brief Speed limit in Mbps.
        /// Only enforced when threading is enabled. 0 means "unlimited".
        /// Its precision depends on used bucket size, low limit used with small bucket size may not be enforceable.
        const uint16_t SpeedLimit;

        Mutex _mutex; ///< The mutex used for synchronizing threads.

        pthread_t _thread; ///< The thread handle.
        sem_t * _semaphore; ///< Semaphore that the thread waits for.

        size_t _maxQueuedPackets; ///< The highest seen number of queued packets in the queue.
        size_t _writesSinceEol; ///< The number of packet writes received since end-of-loop event.

        uint16_t _flags;      ///< Additional flags.

        /// @brief An index to the next entry in the queue that will be sent.
        /// In threaded mode, it is set by the writing thread, and read by the main thread.
        volatile uint16_t _sendIndex;

        /// @brief An index to the next entry in the queue where the next request should be placed.
        /// In threaded mode, it is set by the main thread, and read by the writing thread.
        volatile uint16_t _appendIndex;

        /// @brief An index to the next entry in the queue that should be cleared.
        uint16_t _clearIndex;

        /// @brief Constructor.
        /// @param [in] wType The type of the writer.
        /// @param [in] flags Additional flags.
        /// @param [in] queueSize The size of the queue.
        ///                       This is the max number of individual packets that can be buffered.
        ///                       Ignored in 'BasicWriter' mode.
        /// @param [in] speedLimit The speed limit (in Mbps).
        ///                        It is only enforced when threading is enabled. 0 means "unlimited".
        ///                        Its precision depends on used bucket size, low limit used with small bucket
        ///                        size may not be enforceable.
        PosixPacketWriter ( WriterType wType, uint16_t flags, uint16_t queueSize, uint16_t speedLimit );

        /// @brief Destructor.
        /// It stops the internal thread (if it's being used).
        virtual ~PosixPacketWriter();

        /// @brief Internal function that replaces currently used FD with the new value.
        /// It will first stop the thread if it is running.
        /// It should also be used to clear the descriptor (by setting it to -1).
        /// If the writer is threaded and a valid file descriptor is set, it will start the thread.
        /// @note This function blocks waiting for the background thread to finish.
        ///       It posts to the semaphore, so it should happen right away.
        /// @param [in] fDesc The file descriptor to set.
        void configureFd ( int fDesc );

        /// @brief An internal function that either writes the packet,
        ///        or appends it to write queue (depending on writer's configuration).
        /// @param [in] addr The address to send the data to. Doesn't need to be valid. Ignored in 'basic' mode.
        /// @param [in] data The data to send/write.
        /// @return Standard error code.
        ERRCODE writePacket ( const SockAddr & addr, MemVector & data );

        /// @brief Tries to flush the queue by writing its content.
        /// It will call writePackets() and clear all possible entries from the queue.
        /// On error it unsets the file descriptor and clears the queue.
        /// It may also re-subscribe this object to end-of-loop events.
        /// @note It should only be used in the non-threaded mode!
        void flushQueue();

        /// @brief The main function that the thread is running
        /// @return 0
        void * threadFunc();

        /// @brief The static function used by pthread_create
        /// @param [in] arg Used to pass the pointer to the writer object
        /// @return 0
        static void * staticThreadFunc ( void * arg );

        virtual void receiveLoopEndEvent();
};
}
