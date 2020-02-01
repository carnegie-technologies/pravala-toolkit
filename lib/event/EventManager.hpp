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

#include "basic/Thread.hpp"
#include "basic/MsvcSupport.hpp"
#include "basic/SimpleArray.hpp"
#include "basic/Mutex.hpp"
#include "TimerManager.hpp"

// Debugging of file descriptor operations (setting handlers, etc.)
// #define EVENT_MANAGER_DEBUG_FD_OPS 1

#ifdef USE_LIBEVENT
struct event;
#endif

namespace Pravala
{
/// @brief Event Manager.
class EventManager: public TimerManager
{
    public:
        /// @brief To be inherited by classes that wish to receive information about file descriptor events
        class FdEventHandler
        {
            public:
                /// @brief Function called when an event occurs on specific descriptor.
                ///
                /// This method should be defined in classes to be able to subscribe to events.
                /// @param [in] fd File descriptor that generated this event.
                /// @param [in] events Is a bit sum of Event* values and describes what kind of events were detected.
                virtual void receiveFdEvent ( int fd, short events ) = 0;

                /// @brief Virtual destructor
                virtual ~FdEventHandler()
                {
                }
        };

        /// @brief To be inherited by classes that wish to receive information about file descriptor events
        class ChildEventHandler
        {
            public:
                /// @brief Function called when the child process exits.
                ///
                /// This method should be defined in classes to be able to subscribe to child exit events.
                /// @param [in] childPid PID of the child that exited.
                /// @param [in] childStatus exit status of the child process. It's one of Child* enums.
                /// @param [in] statusValue Depending on the exit status, this can be either
                ///  child's return code (for ChildExited), or the value of the signal that terminated
                ///  the child (ChildSignal), or the value of the signal that stopped the child (ChildStopped).
                virtual void receiveChildEvent ( int childPid, int childStatus, int statusValue ) = 0;

                /// @brief Virtual destructor
                virtual ~ChildEventHandler()
                {
                }
        };

        /// @brief To be inherited by classes that wish to subscribe to end-of-loop events
        class LoopEndEventHandler
        {
            public:
                /// @brief Default constructor
                LoopEndEventHandler(): _endOfLoopId ( 0 )
                {
                }

                /// @brief Callback in the object that happens at the end of event loop
                virtual void receiveLoopEndEvent() = 0;

                /// @brief Virtual destructor
                /// Unsubscribes this object from the end-of-loop events
                virtual ~LoopEndEventHandler()
                {
                    EventManager::loopEndUnsubscribe ( this );
                }

            private:
                /// @brief An identifier of the end-of-loop queue
                uint8_t _endOfLoopId;

                friend class EventManager;
        };

        /// @brief To be inherited by classes that wish to receive signal notifications
        class SignalHandler
        {
            public:
                /// @brief Default constructor
                SignalHandler()
                {
                }

                /// @brief Callback in the object called when a signal is received
                /// @param [in] sigRcvd A Signal* value, which contains the signal received
                virtual void receiveSignalEvent ( int sigRcvd ) = 0;

                /// @brief Virtual destructor
                /// Unsubscribes this object from signal notifications.
                virtual ~SignalHandler()
                {
                    EventManager::signalUnsubscribe ( this );
                }
        };

        /// @brief To be inherited by classes that wish to receive notification when the EventManager is shutting down.
        class ShutdownHandler
        {
            public:
                /// @brief Default constructor
                ShutdownHandler()
                {
                }

                /// @brief Callback in the object called when the EventManager is shutting down.
                virtual void receiveShutdownEvent() = 0;

                /// @brief Virtual destructor
                /// Unsubscribes this object from shutdown notifications.
                virtual ~ShutdownHandler()
                {
                    EventManager::shutdownUnsubscribe ( this );
                }
        };

        /// @brief Child exit codes.
        enum
        {
            ChildExited, ///< Child process exited normally
            ChildSignal, ///< Child process was killed by a signal
            ChildStopped, ///< Child process was stopped (by a signal)
            ChildContinued ///< Child process has been resumed
        };

        static const int EventRead; ///< "Read" event on the file descriptor
        static const int EventWrite; ///< "Write" event on the file descriptor

        static const int SignalHUP; ///< SIGHUP signal
        static const int SignalUsr1; ///< SIGUSR1 signal
        static const int SignalUsr2; ///< SIGUSR2 signal

        /// @brief Static method that returns the number of existing EventManagers.
        /// It does NOT create the EventManager if it doesn't exist.
        /// This function returning >0 does NOT mean that current thread has an EventManager.
        /// @note It is safe to use this function without an existing EventManager.
        /// @see _numManagers
        /// @return the number of existing EventManagers
        static size_t getNumManagers();

        /// @brief Static method that returns true if this thread's EventManager has already been initialized.
        /// @return True if this thread's EventManager has already been initialized; False otherwise.
        static inline bool isInitialized()
        {
            return ( _instance != 0 );
        }

        /// @brief Static method that checks if this thread's EventManager acts as a "primary" manager.
        /// At the moment the only difference is that the "primary" manager handles signals.
        /// @return True if this thread's EventManager acts as a primary manager.
        static inline bool isPrimaryManager()
        {
            return ( _instance != 0 && _instance->_isPrimaryManager );
        }

        /// @brief Creates and initializes the EventManager.
        ///
        /// It should be called before using most of the other methods in this class.
        /// @return Standard error code.
        static ERRCODE init();

        /// @brief Shuts down this thread's EventManager.
        ///
        /// The caller should make sure that the EventManager is not running, otherwise this function will fail.
        /// @param [in] force If set to true, some checks will be skipped. If it is false, this function will check
        ///                    if anything is still using the manager (timers, FD handlers, etc.) and it will fail
        ///                    if that's the case.
        /// @return Standard error code.
        static ERRCODE shutdown ( bool force = false );

        /// @brief Start monitoring for events.
        ///
        /// This function doesn't exit until stop() is called, or an interrupt signal is received.
        /// @note This function can only be used after this thread's EventManager has been initialized using init().
        static void run();

        /// @brief Stops monitoring for events.
        /// This functions doesn't actually stop anything, it just marks,
        /// that run() function should exit after the next internal loop.
        /// @note It is safe to use this function without an existing EventManager.
        static void stop();

        /// @brief Returns current time used by this thread's EventManager.
        /// @note This function can only be used after this thread's EventManager has been initialized using init().
        /// @param [in] refresh If true, current time will be refreshed before it's returned.
        ///                     This is somewhat heavier operation, so the use should be limited to cases that
        ///                     require most up-to-date time.
        /// @return Current time from the moment it was last refreshed.
        static const Time & getCurrentTime ( bool refresh = false );

        /// @brief Sets the object to receive events detected on specific file descriptor.
        /// @note This function can only be used after this thread's EventManager has been initialized using init().
        /// @param [in] fd File descriptor for which events need to be monitored. It has to be valid (>=0)!
        /// @param [in] handler Object that should receive event notifications. It has to be valid (!=0)!
        /// @param [in] events Bit sum of Event* values that we want to receive notifications for.
        static void setFdHandler ( int fd, FdEventHandler * handler, int events );

        /// @brief Disables monitoring of specific file descriptor and removes handler for it.
        ///
        /// It does not delete the handler's object, just removes its pointers from internal
        /// data structures.
        /// @note It is safe to use this function without an existing EventManager.
        /// @param [in] fd File descriptor for which events should not be monitored anymore.
        ///                Invalid FDs (<0) are ignored.
        static void removeFdHandler ( int fd );

        /// @brief Returns events for which given file descriptors is monitored.
        /// @note It is safe to use this function without an existing EventManager.
        /// @param [in] fd File descriptor to be checked. If it is invalid, no events (0) will be returned.
        /// @return Bit sum of Event* values describing which events this file descriptor is monitored for.
        static int getFdEvents ( int fd );

        /// @brief Sets list of events to monitor file descriptor for.
        /// @note This function can only be used after this thread's EventManager has been initialized using init().
        /// @note This function can only be used if there already is a registered FD handler for this FD.
        /// @param [in] fd File descriptor which settings we are modifying. It has to be valid (>=0)!
        /// @param [in] events Bit sum of Event* values that we want to receive notifications for.
        static void setFdEvents ( int fd, int events );

        /// @brief Enables EventWrite events on file descriptor.
        ///
        /// It doesn't modify EventRead settings for that descriptor, just adds
        /// (if needed) EventWrite monitoring.
        /// @note This function can only be used after this thread's EventManager has been initialized using init().
        /// @note This function can only be used if there already is a registered FD handler for this FD.
        /// @param [in] fd File descriptor for which write events need to be monitored. It has to be valid (>=0)!
        static void enableWriteEvents ( int fd );

        /// @brief Disables EventWrite events on file descriptor.
        ///
        /// It doesn't modify EventRead settings for that descriptor, just removes
        /// (if needed) EventWrite monitoring.
        /// @note It is safe to use this function without an existing EventManager.
        /// @note This function doesn't do anything if the handler for this FD is not registered.
        /// @param [in] fd File descriptor for which write events shouldn't be monitored anymore.
        ///                 It has to be valid (>=0)!
        static void disableWriteEvents ( int fd );

        /// @brief Enables EventRead events on file descriptor.
        ///
        /// It doesn't modify EventRead settings for that descriptor, just adds
        /// (if needed) EventRead monitoring.
        /// @note This function can only be used after this thread's EventManager has been initialized using init().
        /// @note This function can only be used if there already is a registered FD handler for this FD.
        /// @param [in] fd File descriptor for which Read events need to be monitored. It has to be valid (>=0)!
        static void enableReadEvents ( int fd );

        /// @brief Disables EventRead events on file descriptor.
        ///
        /// It doesn't modify EventRead settings for that descriptor, just removes
        /// (if needed) EventRead monitoring.
        /// @note It is safe to use this function without an existing EventManager.
        /// @note This function doesn't do anything if the handler for this FD is not registered.
        /// @param [in] fd File descriptor for which Read events shouldn't be monitored anymore.
        ///                 It has to be valid (>=0)!
        static void disableReadEvents ( int fd );

        /// @brief Closes file descriptor and removes event monitoring for it.
        ///
        /// First it uninstalls handler for this file descriptor, disables all events
        /// on this file descriptor, and calls close() on it.
        /// @note It is safe to use this function without an existing EventManager.
        /// @param [in] fd File descriptor to be closed. If < 0 is passed, it is ignored and nothing happens.
        /// @return True if the close was successful; False otherwise.
        ///          It fails if the FD was invalid, or if there was system error.
        static bool closeFd ( int fd );

        /// @brief Adds the object at the end of "End-of-loop" callback list
        ///
        /// If this object already has existing end-of-loop subscription it is not be added again.
        /// After performing the callback, event is removed from the list. If, inside this callback,
        /// an object tries to subscribe again, it will be subscribed for the next end of loop.
        /// @note This function can only be used after this thread's EventManager has been initialized using init().
        /// @param [in] handler A pointer to the object that wants to receive this notification.
        static void loopEndSubscribe ( LoopEndEventHandler * handler );

        /// @brief Removes end-of-loop subscription for the specified object.
        /// @note It is safe to use this function without an existing EventManager.
        /// @param [in] handler A pointer to the object that should be removed from the end-of-loop event list.
        static void loopEndUnsubscribe ( LoopEndEventHandler * handler );

        /// @brief Adds the object to the list of objects to receive notifications about the signals
        /// @note Those notifications are only generated for SIGHUP, SIGUSR1 and SIGUSR2
        /// @note This function can only be used after this thread's EventManager has been initialized using init().
        /// @param [in] handler A pointer to the object that wants to receive this notification.
        static void signalSubscribe ( SignalHandler * handler );

        /// @brief Removes signal event subscription
        /// @note It is safe to use this function without an existing EventManager.
        /// @param [in] handler A pointer to the object that should be removed from the signal subscription list.
        static void signalUnsubscribe ( SignalHandler * handler );

        /// @brief Adds the object to the list of objects to receive 'shutdown' notification
        /// @note This function can only be used after this thread's EventManager has been initialized using init().
        /// @param [in] handler A pointer to the object that wants to receive this notification.
        static void shutdownSubscribe ( ShutdownHandler * handler );

        /// @brief Removes shutdown event subscription
        /// @note It is safe to use this function without an existing EventManager.
        /// @param [in] handler A pointer to the object that should be removed from the shutdown subscription list.
        static void shutdownUnsubscribe ( ShutdownHandler * handler );

        /// @brief Sets the object to receive information when specified child finishes.
        /// @note This function can only be used after this thread's EventManager has been initialized using init().
        /// @param [in] pid PID of the child.
        /// @param [in] handler Object that should receive the notification.
        static void setChildHandler ( int pid, ChildEventHandler * handler );

        /// @brief Disables monitoring of specific child pid and removes handler for it.
        ///
        /// It does not delete the handler's object, just removes its pointers from internal
        /// data structures.
        /// @note It is safe to use this function without an existing EventManager.
        /// @param [in] pid PID of the child process for which the handler should be removed.
        static void removeChildHandler ( int pid );

    protected:
        /// @brief Internal structure for describing events and their receivers
        struct FdEventInfo
        {
            FdEventHandler * handler; ///< Object that will be notified about events

#ifdef USE_LIBEVENT
            struct event * libEventState;  ///< libevent event struct (only used by libevent)
#endif

            int events; ///< Currently set events - bit sum of Event*
        };

        /// @brief Array with description of events.
        SimpleArray<FdEventInfo> _events;

        /// @brief The list of subscriptions to the End-Of-Loop event
        List<LoopEndEventHandler *> _loopEndQueue;

        /// @brief The list of subscriptions to the End-Of-Loop event while they are being processed
        List<LoopEndEventHandler *> _processedLoopEndQueue;

        /// @brief The list of subscriptions to signal event
        List<SignalHandler *> _signalHandlers;

        /// @brief The list of subscriptions to shutdown event
        List<ShutdownHandler *> _shutdownHandlers;

        /// @brief HashMap storing children pids and associated objects that
        /// should receive information about specific child process.
        HashMap<int, ChildEventHandler *> _childHandlers;

        /// @brief Set to true in the primary EventManager.
        /// @see _primaryManagerExists
        const bool _isPrimaryManager;

        /// @brief Marks if this EventManager is working.
        ///
        /// Makes run() function break its loop if it is set to false.
        bool _working;

        uint8_t _currentEndOfLoopId; ///< The ID of the current end-of-loop queue

        /// @brief Constructor
        EventManager();

        /// @brief Destructor
        virtual ~EventManager();

        /// @brief Exposes pointer to the instance of the EventManager.
        /// @return Current pointer to the instance of the EventManager.
        static inline EventManager * getInstance()
        {
            return _instance;
        }

        /// @brief Handles timers and end-of-loop events
        /// Should be called at the end of every event loop
        void runEndOfLoop();

        /// @brief Notifies signal handlers
        /// @param [in] sigRcvd The signal received
        void notifySignalHandlers ( int sigRcvd );

        /// @brief Shuts down this thread's EventManager.
        /// This function can be overloaded by the specific implementation.
        /// The base version only checks if this EventManager is running and, if 'force' is set to false,
        /// whether anything is still using it.
        /// @note If this function returns code that's considered 'OK', this manager's destructor will be called.
        /// @param [in] force If true, it will try harder and potentially skip some checks.
        /// @return Standard error code.
        virtual ERRCODE implShutdown ( bool force );

        /// @brief Start monitoring for events.
        ///
        /// This function doesn't exit until stop() is called, or an interrupt signal is received.
        virtual void implRun() = 0;

        /// @brief Sets the object to receive events detected on specific file descriptor.
        ///
        /// @param [in] fd File descriptor for which events need to be monitored. It should be valid (>=0).
        /// @param [in] handler Object that should receive event notifications. It should be valid (!=0).
        /// @param [in] events Bit sum of Event* values that we want to receive notifications for.
        virtual void implSetFdHandler ( int fd, FdEventHandler * handler, int events ) = 0;

        /// @brief Disables monitoring of specific file descriptor and removes handler for it.
        ///
        /// It does not delete the handler's object, just removes its pointers from internal
        /// data structures.
        /// @param [in] fd File descriptor for which events should not be monitored anymore. It should be valid (>=0).
        virtual void implRemoveFdHandler ( int fd ) = 0;

        /// @brief Sets list of events to monitor file descriptor for.
        ///
        /// @note The handler for this FD should be already subscribed.
        /// @param [in] fd File descriptor which settings we are modifying. It should be valid (>=0).
        /// @param [in] events Bit sum of Event* values that we want to receive notifications for.
        virtual void implSetFdEvents ( int fd, int events ) = 0;

        friend class Timer;

    private:
        /// @brief A mutex controlling access to _numManagers and _primaryManagerExists.
        static Mutex _mutex;

        /// @brief The number of created EventManagers (across all threads).
        /// When it's 0, it means that EventManager has never been created (or it has been created and later destroyed)
        /// and it's safe to do anything.
        ///
        /// When this is >0, at least one EventManager exists, and you should not create any child processes
        /// (e.g. with Utils::forkChild) that use EventManager as bad things will happen!
        ///
        /// You can however, use EventManager after a pthread_create, as that will return a new and different
        /// instance of EventManager.
        static size_t _numManagers;

        /// @brief Set when a primary EventManager exists.
        /// The first EventManager created will be the "primary" one.
        /// However, if that EventManager is destroyed, this flag will be unset.
        /// The next EventManager created will become the new primary manager,
        /// even if at that point there are other EventManagers.
        /// The difference between primary and non-primary managers is, at the moment, signal handling.
        /// Only the primary manager handles signals.
        static bool _primaryManagerExists;

        /// @brief A pointer to this thread's instance of the EventManager.
        static THREAD_LOCAL EventManager * _instance;

        /// @brief This function should be called whenever an EventManager is created.
        /// It always increments the _numCreated counter.
        /// It also inspects _primaryManagerExists. If that flag is set to 'false',
        /// this function sets it to 'true' and returns 'true'.
        /// Otherwise it returns false (the counter is still incremented).
        /// @return True if the calling EventManager should become a new primary manager.
        static bool newManagerCreated();
};
}
