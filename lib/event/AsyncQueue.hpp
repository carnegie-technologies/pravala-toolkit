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

#include "basic/NoCopy.hpp"
#include "basic/HashSet.hpp"
#include "basic/Mutex.hpp"
#include "error/Error.hpp"

#include "SocketPair.hpp"
#include "EventManager.hpp"

namespace Pravala
{
/// @brief Class that allows to run tasks on the main thread
class AsyncQueue: public EventManager::FdEventHandler
{
    public:
        /// @brief Policy for deleting tasks that are passed to the queue.
        enum DeletePolicy
        {
            /// @brief Delete the task even if it fails to run.
            DeleteOnError = 0,

            /// @brief Don't delete the task if it fails to run.
            /// Only tasks that are successfully passed to the main thread are eventually deleted after being run.
            DontDeleteOnError = 1
        };

        /// @brief A class that should be inherited by all tasks
        class Task
        {
            protected:
                /// @brief Called to run the task.
                virtual void runTask() = 0;

                /// @brief Returns a pointer to an object that this task will talk to.
                /// It can be overloaded by the specific task and return a pointer to the object
                /// that this task will want to talk to on the main thread.
                /// The AsyncQueue knows all registered receivers, and if this function returns a non-zero
                /// value, the AsyncQueue will verify if that receiver is still registered.
                /// Receivers should unregister in the destructor.
                /// The use of this feature is optional.
                /// By default this function returns whatever was passed to the constructor.
                /// If this function returns zero, no check will be performed and the task will always run.
                /// @return A pointer to the object this task will be talking to,
                ///         or 0 if the AsyncQueue shouldn't care.
                virtual NoCopy * getReceiver() const;

                /// @brief Constructor
                /// @note Although this class does not care about the actual object types,
                ///       objects registered with it should typically not be copyable.
                ///       Therefore this method takes a NoCopy pointer, forcing objects to inherit NoCopy class.
                /// @param [in] receiver The receiver to return using getReceiver().
                Task ( NoCopy * receiver );

                /// @brief Virtual destructor.
                virtual ~Task();

                /// @brief Deletes this task on error, if the policy is DeleteOnError.
                /// @param [in] deletePolicy The deletion policy.
                void deleteOnError ( DeletePolicy deletePolicy );

            private:
                NoCopy * const _receiver; ///< Pointer to the receiver

                friend class AsyncQueue;
        };

        /// @brief Returns a reference to the global AsyncQueue.
        /// @warning The first time it is called it initializes the AsyncQueue and EventManager.
        /// It should happen on the main thread!
        /// @return A reference to the global AsyncQueue.
        static AsyncQueue & get();

        /// @brief Registers an object in the list of valid receivers.
        /// @note Although this class does not care about the actual object types,
        ///       objects registered with it should typically not be copyable.
        ///       Therefore this method takes a NoCopy pointer, forcing objects to inherit NoCopy class.
        /// @warning This should only be called on the main thread!
        /// @param [in] receiver Pointer to the object to register.
        void registerReceiver ( NoCopy * receiver );

        /// @brief Unregisters an object from the list of valid receivers.
        /// @note Although this class does not care about the actual object types,
        ///       objects registered with it should typically not be copyable.
        ///       Therefore this method takes a NoCopy pointer, forcing objects to inherit NoCopy class.
        /// @warning This should only be called on the main thread!
        /// @param [in] receiver Pointer to the object to unregister.
        void unregisterReceiver ( NoCopy * receiver );

        /// @brief Schedules the task to be run on main thread without blocking.
        /// It should be called from the other thread.
        /// The task passed should be allocated on the heap.
        /// No copy is created, and that pointer is passed to the main thread.
        /// The main thread, when it receives this pointer, will first check if the receiver
        /// (obtained using getReceiver) is still valid (if getReceiver returns non-zero value),
        /// and if it does, it will call runTask().
        /// Once it completes, this task object will be deleted on the main thread.
        /// @param [in] task A pointer to the task to run on the main thread.
        /// @param [in] deletePolicy Policy for deleting tasks passed to the queue.
        ///                          Tasks that are passed to the main thread successfully are always deleted.
        /// @return Standard error code:
        ///          - Success if the task was passed to the main thread. The task will be deleted on main thread.
        ///          - InvalidParameter if the task pointer is 0, or if deletePolicy is invalid, in which case the task
        ///            will not be deleted.
        ///          - Closed if the queue is broken and can no longer be used.
        ///          - SoftFail if the queue is blocked and should be tried later.
        ERRCODE runTask ( Task * task, DeletePolicy deletePolicy = DeleteOnError );

        /// @brief Schedules the task to be run on main thread, blocking up to a given amount of time.
        /// It should be called from the other thread.
        /// The task passed should be allocated on the heap.
        /// No copy is created, and that pointer is passed to the main thread.
        /// The main thread, when it receives this pointer, will first check if the receiver
        /// (obtained using getReceiver) is still valid (if getReceiver returns non-zero value),
        /// and if it does, it will call runTask().
        /// Once it completes, this task object will be deleted on the main thread.
        /// @param [in] task A pointer to the task to run on the main thread.
        /// @param [in] timeoutMs The max time to block in milliseconds.
        ///                       0 means this can block forever.
        ///                       This timeout value may only be accurate to within 100ms.
        /// @param [in] deletePolicy Policy for deleting tasks passed to the queue.
        ///                          Tasks that are passed to the main thread successfully are always deleted.
        /// @return Standard error code:
        ///          - Success if the task was passed to the main thread. The task will be deleted on main thread.
        ///          - InvalidParameter if the task pointer is 0, or if deletePolicy is invalid, in which case the task
        ///            will not be deleted.
        ///          - Closed if the queue is broken and can no longer be used.
        ///          - Timeout if we reached the timeout.
        ERRCODE blockingRunTask ( Task * task, uint32_t timeoutMs = 0, DeletePolicy deletePolicy = DeleteOnError );

    protected:
        /// @brief Function called when an event occurs on specific descriptor.
        /// @param [in] fd File descriptor that generated this event.
        /// @param [in] events Is a bit sum of Event* values and describes what kind of events were detected.
        virtual void receiveFdEvent ( int fd, short int events );

    private:
        Mutex _mutex; ///< A mutex used for synchronizing write operations
        HashSet<NoCopy *> _receivers; ///< Registered receivers
        SocketPair _socks; ///< SocketPair used for passing data between threads
        char _readBuf[ sizeof ( Task * ) ]; ///< Buffer for reading the data on the main thread
        size_t _offset; ///< Offset in the buffer
        bool _isBroken; ///< Set when a partial write or write error occurs, to prevent sending invalid data.

        /// @brief Default constructor
        /// This actually initializes the SocketPair
        AsyncQueue();
};
}
