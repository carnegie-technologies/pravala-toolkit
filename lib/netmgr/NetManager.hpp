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

#include "NetManagerTypes.hpp"
#include "error/Error.hpp"
#include "log/TextLog.hpp"
#include "event/EventManager.hpp"

namespace Pravala
{
/// @brief Network manager.
class NetManager: public EventManager::LoopEndEventHandler, public EventManager::ShutdownHandler
{
    public:
        /// @brief Interface for route change notifications.
        class RouteMonitor
        {
            public:
                /// @brief Called when there is a change in routes.
                /// @param [in] added Set of routes that have been added.
                /// @param [in] removed Set of routes that have been removed.
                virtual void netRoutesChanged (
                    const HashSet<NetManagerTypes::Route> & added,
                    const HashSet<NetManagerTypes::Route> & removed ) = 0;

                /// @brief Destructor.
                virtual ~RouteMonitor();
        };

        /// @brief Subscribes for receiving route updates.
        /// Updates to interfaces addresses and routes are generated in unspecified order.
        /// @param [in] monitor The object that wishes to receive those updates.
        /// @param [in] fullUpdate If set to true (default), a "fake" full update callback will be generated
        ///                         for this monitor at the end of the next event loop.
        void subscribeRoutes ( RouteMonitor * monitor, bool fullUpdate = true );

        /// @brief Unsubscribes from receiving ALL route updates.
        /// @param [in] monitor The object that wishes to stop receiving updates.
        void unsubscribeRoutes ( RouteMonitor * monitor );

        /// @brief Interface for interface address change notifications.
        class AddressMonitor
        {
            public:
                /// @brief Called when there is a change in interface addresses.
                /// @param [in] added Set of addresses that have been added.
                /// @param [in] removed Set of addresses that have been removed.
                virtual void netIfaceAddressesChanged (
                    const HashSet<NetManagerTypes::Address> & added,
                    const HashSet<NetManagerTypes::Address> & removed ) = 0;

                /// @brief Destructor.
                virtual ~AddressMonitor();
        };

        /// @brief Subscribes for receiving address updates.
        /// Updates to interfaces addresses and routes are generated in unspecified order.
        /// @param [in] monitor The object that wishes to receive those updates.
        /// @param [in] fullUpdate If set to true (default) , a "fake" full update callback will be generated
        ///                         for this monitor at the end of the next event loop.
        void subscribeAddresses ( AddressMonitor * monitor, bool fullUpdate = true );

        /// @brief Unsubscribes from receiving address updates.
        /// @param [in] monitor The object that wishes to stop receiving updates.
        void unsubscribeAddresses ( AddressMonitor * monitor );

        /// @brief Interface for interface change notifications.
        class IfaceMonitor
        {
            public:
                /// @brief Called when there is a change in interfaces.
                /// @param [in] activated Set of IDs of interfaces that became active.
                /// @param [in] deactivated The set of IDs of interfaces that are present but not active.
                /// @param [in] removed Set of IDs of interfaces that have been removed from the system.
                virtual void netIfacesChanged (
                    const HashSet<int> & activated,
                    const HashSet<int> & deactivated,
                    const HashSet<int> & removed ) = 0;

                /// @brief Destructor.
                virtual ~IfaceMonitor();
        };

        /// @brief Subscribes for receiving interface updates.
        /// Updates to interfaces addresses and routes are generated in unspecified order.
        /// @note Interface that goes from active to inactive and then to removed will generate the "removed"
        /// callback twice - once it's being deactivated and once when it's actually removed.
        /// If it is being removed while it's still active it will only generate a single "removed" callback.
        /// @param [in] monitor The object that wishes to receive those updates.
        /// @param [in] fullUpdate If set to true (default), a "fake" full update callback will be generated
        ///                         for this monitor at the end of the next event loop.
        void subscribeIfaces ( IfaceMonitor * monitor, bool fullUpdate = true );

        /// @brief Unsubscribes from receiving interface updates.
        /// @param [in] monitor The object that wishes to stop receiving updates.
        void unsubscribeIfaces ( IfaceMonitor * monitor );

        /// @brief Returns global instance of the NetManager
        static NetManager & get();

        /// @brief Checks whether an interface is marked as 'active'.
        /// @note This returns a cached version.
        /// @param [in] ifaceId The ID of the interface to check.
        /// @return True if the interface is active (up and running); False otherwise.
        inline bool isIfaceActive ( int ifaceId ) const
        {
            const NetManagerTypes::InterfaceObject * const iface = _ifaces.value ( ifaceId );

            return ( iface != 0 && iface->isActive() );
        }

        /// @brief Exposes all the interfaces.
        /// @note This returns a cached version. Creating a new interface and calling this function will
        ///        not return the interface that just has been created.
        /// @return The map with all the interfaces (interface ID : interface pointer)
        inline const HashMap<int, NetManagerTypes::InterfaceObject *> & getIfaces()
        {
            return _ifaces;
        }

        /// @brief Returns the interfaces object.
        /// @note This returns a cached version. Creating a new interface and calling this function will
        ///        not return the interface that just has been created.
        /// @param [in] ifaceId The ID of the interface to return.
        /// @return The interface object, or 0 if the interface does not exist.
        inline NetManagerTypes::InterfaceObject * getIface ( int ifaceId )
        {
            return _ifaces.value ( ifaceId );
        }

        /// @brief Reads the current, uncached interface information from the OS.
        /// @note This will not update the internal cache, which means that calling getIfaces()
        ///        after this function may return different data.
        /// @param [in] ifaceId The ID of the interface to query.
        /// @param [out] iface Interface data read.
        /// @return Standard error code.
        virtual ERRCODE getUncachedIface ( int ifaceId, NetManagerTypes::Interface & iface ) = 0;

        /// @brief Reads the current, uncached interface information from the OS.
        /// @note This will not update the internal cache, which means that calling getIfaces()
        ///        after this function may return different data.
        /// @param [in] ifaceName The name of the interface to query.
        /// @param [out] iface Interface data read.
        /// @return Standard error code.
        virtual ERRCODE getUncachedIface ( const String & ifaceName, NetManagerTypes::Interface & iface ) = 0;

        /// @brief Reads the data usage of an interface.
        /// @param [in] ifaceName The name of the interface.
        /// @param [out] rxBytes The number of bytes received over this interface.
        /// @param [out] txBytes The number of bytes sent over this interface.
        /// @return Standard error code.
        virtual ERRCODE readIfaceUsage ( const String & ifaceName, uint64_t & rxBytes, uint64_t & txBytes ) = 0;

        /// @brief Exposes all active routes.
        /// @note This returns a cached version.
        /// @return The set of all active routes.
        inline const HashSet<NetManagerTypes::Route> & getRoutes() const
        {
            return _routes;
        }

        /// @brief Exposes all the host routes.
        /// @note This returns a cached version.
        /// @return The map with all the host routes (host address : set of routes)
        inline const HashMap<IpAddress, HashSet<NetManagerTypes::Route> > & getHostRoutes() const
        {
            return _hostRoutes;
        }

        /// @brief Exposes all the default routes.
        /// @note This returns a cached version.
        /// @return The map with all the default routes (interface ID : set of routes)
        inline const HashMap<int, HashSet<NetManagerTypes::Route> > & getDefaultRoutes() const
        {
            return _defaultRoutes;
        }

        /// @brief Synchronously sets the state of an interface.
        /// @param [in] ifaceId Interface ID.
        /// @param [in] isUp Whether the interface should be "UP" (true), or "DOWN" (false).
        /// @return Standard error code.
        virtual ERRCODE setIfaceState ( int ifaceId, bool isUp ) = 0;

        /// @brief Asynchronously sets the state of an interface.
        /// Depending on the specific NetworkManager implementation, it will either use native
        /// features of the OS to perform the operation asynchronously, or it will perform these operations
        /// Synchronously at the end of the next event loop.
        /// @param [in] ifaceId Interface ID.
        /// @param [in] isUp Whether the interface should be "UP" (true), or "DOWN" (false).
        virtual void setIfaceStateAsync ( int ifaceId, bool isUp );

        /// @brief Synchronously sets the MTU value of an interface.
        /// @param [in] ifaceId Interface ID.
        /// @param [in] mtu MTU value to use.
        /// @return Standard error code.
        virtual ERRCODE setIfaceMtu ( int ifaceId, int mtu ) = 0;

        /// @brief Asynchronously sets the MTU value of an interface.
        /// Depending on the specific NetworkManager implementation, it will either use native
        /// features of the OS to perform the operation asynchronously, or it will perform these operations
        /// Synchronously at the end of the next event loop.
        /// @param [in] ifaceId Interface ID.
        /// @param [in] mtu MTU value to use.
        virtual void setIfaceMtuAsync ( int ifaceId, int mtu );

        /// @brief Synchronously adds an address to an interface.
        /// @param [in] ifaceId The ID of the interface to add the address to.
        /// @param [in] address The address to add.
        /// @return Standard error code.
        virtual ERRCODE addIfaceAddress ( int ifaceId, const IpAddress & address ) = 0;

        /// @brief Asynchronously adds an address to an interface.
        /// Depending on the specific NetworkManager implementation, it will either use native
        /// features of the OS to perform the operation asynchronously, or it will perform these operations
        /// Synchronously at the end of the next event loop.
        /// @param [in] ifaceId The ID of the interface to add the address to.
        /// @param [in] address The address to add.
        virtual void addIfaceAddressAsync ( int ifaceId, const IpAddress & address );

        /// @brief Synchronously removes an address from an interface.
        /// @param [in] ifaceId The ID of the interface to remove the address from.
        /// @param [in] address The address to remove.
        /// @return Standard error code.
        virtual ERRCODE removeIfaceAddress ( int ifaceId, const IpAddress & address ) = 0;

        /// @brief Asynchronously removes an address from an interface.
        /// Depending on the specific NetworkManager implementation, it will either use native
        /// features of the OS to perform the operation asynchronously, or it will perform these operations
        /// Synchronously at the end of the next event loop.
        /// @param [in] ifaceId The ID of the interface to remove the address from.
        /// @param [in] address The address to remove.
        virtual void removeIfaceAddressAsync ( int ifaceId, const IpAddress & address );

        /// @brief Synchronously adds a system route.
        /// @param [in] dst The destination address.
        /// @param [in] mask The netmask to use.
        /// @param [in] gw The gateway to use; It doesn't need to be valid if ifaceId is used.
        /// @param [in] ifaceId Interface ID to use; It won't be used if < 0.
        /// @param [in] metric The metric to use.
        /// @param [in] tableId The ID of the routing table to use. May not be supported on all platforms.
        ///                      If < 0 (or unsupported), the default table will be used.
        /// @return Standard error code.
        virtual ERRCODE addRoute (
            const IpAddress & dst, uint8_t mask, const IpAddress & gw,
            int ifaceId = -1, int metric = 0, int tableId = -1 ) = 0;

        /// @brief Asynchronously adds a system route.
        /// Depending on the specific NetworkManager implementation, it will either use native
        /// features of the OS to perform the operation asynchronously, or it will perform these operations
        /// Synchronously at the end of the next event loop.
        /// @param [in] dst The destination address.
        /// @param [in] mask The netmask to use.
        /// @param [in] gw The gateway to use; It doesn't need to be valid if ifaceId is used.
        /// @param [in] ifaceId Interface ID to use; It won't be used if < 0.
        /// @param [in] metric The metric to use.
        /// @param [in] tableId The ID of the routing table to use. May not be supported on all platforms.
        ///                      If < 0 (or unsupported), the default table will be used.
        virtual void addRouteAsync (
            const IpAddress & dst, uint8_t mask, const IpAddress & gw,
            int ifaceId = -1, int metric = 0, int tableId = -1 );

        /// @brief Synchronously removes a system route.
        /// @param [in] dst The destination address.
        /// @param [in] mask The netmask to use.
        /// @param [in] gw The gateway to use; It doesn't need to be valid if ifaceId is used.
        /// @param [in] ifaceId Interface ID to use; It won't be used if < 0.
        /// @param [in] metric The metric to use.
        /// @param [in] tableId The ID of the routing table to use. May not be supported on all platforms.
        ///                      If < 0 (or unsupported), the default table will be used.
        /// @return Standard error code.
        virtual ERRCODE removeRoute (
            const IpAddress & dst, uint8_t mask, const IpAddress & gw,
            int ifaceId = -1, int metric = 0, int tableId = -1 ) = 0;

        /// @brief Asynchronously removes a system route.
        /// Depending on the specific NetworkManager implementation, it will either use native
        /// features of the OS to perform the operation asynchronously, or it will perform these operations
        /// Synchronously at the end of the next event loop.
        /// @param [in] dst The destination address.
        /// @param [in] mask The netmask to use.
        /// @param [in] gw The gateway to use; It doesn't need to be valid if ifaceId is used.
        /// @param [in] ifaceId Interface ID to use; It won't be used if < 0.
        /// @param [in] metric The metric to use.
        /// @param [in] tableId The ID of the routing table to use. May not be supported on all platforms.
        ///                      If < 0 (or unsupported), the default table will be used.
        virtual void removeRouteAsync (
            const IpAddress & dst, uint8_t mask, const IpAddress & gw,
            int ifaceId = -1, int metric = 0, int tableId = -1 );

    protected:
        static TextLog _log; ///< Log stream

        /// @brief Default constructor.
        NetManager();

        /// @brief Destructor.
        virtual ~NetManager();

        /// @brief Performs a route update callback.
        /// @param [in] added The set of routes that were added.
        /// @param [in] removed The set of routes that were removed.
        void doNotify (
            const HashSet<NetManagerTypes::Route> added,
            const HashSet<NetManagerTypes::Route> removed );

        /// @brief Performs a address update callback.
        /// @param [in] added The set of addresses that were added.
        /// @param [in] removed The set of addresses that were removed.
        void doNotify (
            const HashSet<NetManagerTypes::Address> added,
            const HashSet<NetManagerTypes::Address> removed );

        /// @brief Performs an interface update callback.
        /// @param [in] added The set of IDs of interfaces that were activated.
        /// @param [in] deactivated The set of IDs of interfaces that are present but not active.
        /// @param [in] removed The set of IDs of interfaces that were removed.
        void doNotify (
            const HashSet<int> added,
            const HashSet<int> deactivated,
            const HashSet<int> removed );

        virtual void receiveLoopEndEvent();
        virtual void receiveShutdownEvent();

        /// @brief Runs all tasks stored in _asyncTasks list.
        void runTasks();

    private:
        /// @brief Different monitors.
        struct MonitorGroup
        {
            HashSet<RouteMonitor *> route; ///< The set of route monitors.
            HashSet<AddressMonitor *> address; ///< The set of address monitors.
            HashSet<IfaceMonitor *> iface; ///< The set of interface monitors.
        };

        /// @brief A class for scheduling asynchronous tasks
        class AsyncTask
        {
            public:
                /// @brief Runs the task.
                /// @param [in] The NetManager that runs it.
                virtual void runTask ( NetManager & netManager ) = 0;

                /// @brief Destructor.
                virtual ~AsyncTask();
        };

        /// @brief A class that performs all initial updates for monitors in _scheduledMonitors group.
        class DoScheduledUpdatesTask: public AsyncTask
        {
            public:
                /// @brief Runs the task.
                /// @param [in] The NetManager that runs it.
                virtual void runTask ( NetManager & netManager );

                /// @brief Destructor.
                virtual ~DoScheduledUpdatesTask();
        };

        /// @brief A task that performs delayed route operations.
        class RouteOpTask: public AsyncTask
        {
            public:
                /// @brief Type of the task.
                enum Type
                {
                    AddRoute, ///< Add route
                    RemoveRoute ///< Remove route
                };

                /// @brief Constructor.
                /// @param [in] type The type of the route operation to perform.
                /// @param [in] dst Route's destination.
                /// @param [in] mask Route's netmask.
                /// @param [in] gw Route's gateway; It doesn't need to be valid if ifaceId is used.
                /// @param [in] ifaceId Route's interface ID; It won't be used if < 0.
                /// @param [in] metric Route's metric.
                /// @param [in] tableId The ID of the routing table to use.
                ///                      If < 0 (or unsupported), the default table will be used.
                RouteOpTask (
                    Type type, const IpAddress & dst, uint8_t mask, const IpAddress & gw,
                    int ifaceId, int metric, int tableId );

                virtual void runTask ( NetManager & netManager );

            private:
                const Type _type; ///< The type of the route operation to perform.
                const IpAddress _dst; ///< Route's destination.
                const IpAddress _gw; ///< Route's gateway.
                const int _ifaceId; ///< Route's interface ID.
                const int _metric; ///< Route's metric.
                const int _tableId; ///< Route's table ID.
                const uint8_t _mask; ///< Route's netmask.
        };

        /// @brief A task that performs delayed address operations.
        class IfaceAddressOpTask: public AsyncTask
        {
            public:
                /// @brief Type of the task.
                enum Type
                {
                    AddAddress, ///< Add address
                    RemoveAddress ///< Remove address
                };

                /// @brief Constructor.
                /// @param [in] type The type of the address operation to perform.
                /// @param [in] ifaceId The ID of the interface.
                /// @param [in] address The address to add/remove.
                IfaceAddressOpTask ( Type type, int ifaceId, const IpAddress & address );

                virtual void runTask ( NetManager & netManager );

            private:
                const Type _type; ///< The type of the address operation to perform.
                const int _ifaceId; ///< The ID of the interface.
                const IpAddress _addr; ///< The address to add/remove.
        };

        /// @brief A task that performs delayed iface operations.
        class IfaceOpTask: public AsyncTask
        {
            public:
                /// @brief Type of the task.
                enum Type
                {
                    SetState, ///< Set iface state (value != 0 means 'up', value == 0 means 'down')
                    SetMtu ///< Set iface MTU
                };

                /// @brief Constructor.
                /// @param [in] type The type of the interface operation to perform.
                /// @param [in] ifaceId The ID of the interface.
                /// @param [in] value The value for the operation (see IfaceOpTask::Type enum).
                IfaceOpTask ( Type type, int ifaceId, int value );

                virtual void runTask ( NetManager & netManager );

            private:
                const Type _type; ///< The type of the interface operation to perform.
                const int _ifaceId; ///< The ID of the interface.
                const int _value; ///< The value for the operation.
        };

        MonitorGroup _activeMonitors; ///< All active monitors. It does NOT include scheduled monitors.
        MonitorGroup _scheduledMonitors; ///< All monitors scheduled for the first, full update.

        List<AsyncTask *> _asyncTasks; ///< The list of async tasks to run at the end of the event loop.

        /// @brief All known interfaces; Maps their ID to the interface object.
        HashMap<int, NetManagerTypes::InterfaceObject *> _ifaces;

        HashSet<NetManagerTypes::Address> _addresses; ///< All active addresses.

        HashSet<NetManagerTypes::Route> _routes; ///< All active routes.

        /// @brief All (active) host routes.
        /// Maps host's IP address to the set with all the routes to that host.
        HashMap<IpAddress, HashSet<NetManagerTypes::Route> > _hostRoutes;

        /// @brief All (active) default routes.
        /// Maps the 'out' interface index to the set with all default routes over that interface.
        HashMap<int, HashSet<NetManagerTypes::Route> > _defaultRoutes;

        bool _scheduledUpdatesTaskAdded; ///< Set to true when the DoScheduledUpdatesTask is scheduled.

        /// @brief Schedules a task for asynchronous execution at the end of the next event loop.
        /// @param [in] task Pointer to the task to schedule.
        void scheduleTask ( AsyncTask * task );

        /// @brief A helper function that schedules a new DoScheduledUpdatesTask task (unless it's already scheduled)
        void scheduleFullUpdates();

        friend class NetManagerBase;
};
}
