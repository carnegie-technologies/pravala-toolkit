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

#include "NetManager.hpp"

namespace Pravala
{
/// @brief The base network manager functionality.
/// To be used by specific implementations.
/// It contains the functionality that is not needed by the public API.
/// Methods of this object actually manage NetManager's state.
class NetManagerBase: public NetManager
{
    protected:
        /// @brief Sets the new set of routes.
        /// @param [in,out] routes The new set of routes. It will be modified to include only the routes that were
        ///                         not active before, but now are.
        void setRoutes ( HashSet<NetManagerTypes::Route> & routes );

        /// @brief Modify the set of routes.
        /// @param [in,out] add Routes to be added. It will be modified to include only the routes that were
        ///                         not active before, but now are.
        /// @param [in,out] remove Routes to be removed. It will be modified to include only the routes that were
        ///                         active before, but now are not.
        void modifyRoutes (
            HashSet<NetManagerTypes::Route> & add,
            HashSet<NetManagerTypes::Route> & remove );

        /// @brief Sets the new set of addresses.
        /// @param [in,out] addresses The new set of addresses. It will be modified to include only
        ///                            the addresses that were not active before, but now are.
        void setAddresses ( HashSet<NetManagerTypes::Address> & addresses );

        /// @brief Modify the set of addresses.
        /// @param [in,out] add Addresses to be added. It will be modified to include only
        ///                      the addresses that were not active before, but now are.
        /// @param [in,out] remove Addresses to be removed. It will be modified to include only
        ///                         the addresses that were active before, but now are not.
        void modifyAddresses (
            HashSet<NetManagerTypes::Address> & add,
            HashSet<NetManagerTypes::Address> & remove );

        /// @brief Sets the new set of interfaces.
        /// @param [in,out] ifaces The new map of interfaces (iface_id: iface_data).
        ///                         For performance reasons this set may be modified. If the caller needs to keep
        ///                         the original data, they should pass a copy.
        /// @param [in,out] addresses The new set of addresses. It will be modified to include only
        ///                            the addresses that were not active before, but now are.
        /// @param [in,out] routes The new set of routes. It will be modified to include only the routes that were
        ///                         not active before, but now are.
        void setIfaces (
            HashMap<int, NetManagerTypes::Interface> & ifaces,
            HashSet<NetManagerTypes::Address> & addresses,
            HashSet<NetManagerTypes::Route> & routes );

        /// @brief Removes an interface.
        /// It will also remove routes and addresses related to this interface.
        /// @param [in] ifaceId The ID of the interface to remove.
        void removeIface ( const int ifaceId );

        /// @brief Updates the list of interfaces.
        /// It will also update routes and addresses related to interfaces changed
        /// (remove them, activate, or deactivate - as appropriate).
        /// @param [in,out] updateData The updated list of interface data objects. It can contain new data
        ///                             for existing interfaces, or data for new interfaces.
        ///                             For performance reasons this set may be modified. If the caller needs to keep
        ///                             the original data, they should pass a copy.
        /// @param [in,out] removeIfaces The list of IDs of interfaces to remove. It will be modified to include only
        ///                               the interfaces that were actually removed.
        void updateIfaces (
            HashMap<int, NetManagerTypes::Interface> & updateData,
            HashSet<int> & removeIfaces );

    private:
        /// @brief A helper function that removes this route from internal set of routes of each of its interfaces.
        /// It does not generate any callbacks.
        /// @param [in] route The route to remove.
        void removeIfaceRouteNoCb ( const NetManagerTypes::Route & route );

        /// @brief A helper function that removes this route from _defaultRoutes and/or _hostRoutes (if needed).
        /// If this is not host or default route, nothing will be done.
        /// It does not remove it from _routes or any other sets.
        /// @param [in] route The route to remove.
        void removeHelperRouteNoCb ( const NetManagerTypes::Route & route );

        /// @brief A helper function that tries to 'activate' given route.
        /// It will check if all interfaces associated with this route are active.
        /// If not, it will return 'false'.
        /// Otherwise the route is considered 'active' and will be added to _routes, _defaultRoutes and _hostRoutes
        /// (depending on the route type) and this function will return true.
        /// It does not generate any callbacks.
        /// @param [in] route The route to activate.
        /// @return True if this route is an active route (even if it was active before); False otherwise.
        bool activateRouteNoCb ( const NetManagerTypes::Route & route );

        /// @brief A helper function that deactivates and optionally removes given routes.
        /// It does not generate any callbacks.
        /// @param [in,out] routes The set of routes to deactivate and optionally remove.
        ///                         It is modified - after calling this function this set will only contain
        ///                         routes that were active before (but now are not).
        /// @param [in] alsoIfaceRemove If false, this function will remove routes from active routes sets
        ///                              (_routes, _defaultRoutes, _hostRoutes); If true, it will ALSO remove
        ///                              them from all interfaces associated with them.
        void deactivateRoutesNoCb ( HashSet<NetManagerTypes::Route> & routes, bool alsoIfaceRemove );

        /// @brief A helper function that tries to 'activate' given address.
        /// It will check if the interface associated with this address is active.
        /// If not, it will return 'false'.
        /// Otherwise the address is considered 'active' and will be added to _addresses
        /// and this function will return true.
        /// It does not generate any callbacks.
        /// @param [in] address The address to activate.
        /// @return True if this address is an active address (even if it was active before); False otherwise.
        bool activateAddressNoCb ( const NetManagerTypes::Address & address );

        /// @brief A helper function that deactivates and optionally removes given addresses.
        /// It does not generate any callbacks.
        /// @param [in,out] addresses The set of addresses to deactivate and optionally remove.
        ///                            It is modified - after calling this function this set will only contain
        ///                            addresses that were active before (but now are not).
        /// @param [in] alsoIfaceRemove If false, this function will remove addresses from _addresses;
        ///                             If true, it will ALSO remove them from interfaces associated with them.
        void deactivateAddressesNoCb ( HashSet<NetManagerTypes::Address> & addresses, bool alsoIfaceRemove );

        /// @brief A helper function that updates the state of the manager.
        /// @param [in,out] updateData The map of updated interface data (iface_id:data).
        ///                             All interfaces in this map will be added/updated, except the interfaces
        ///                             whose IDs are also in removeIfaces set - those will be removed.
        ///                             For performance reasons it may be modified. If the caller needs to keep
        ///                             the original data, they should pass a copy.
        /// @param [in,out] removeIfaces The set of interface IDs to remove. It will be modified to include only
        ///                               the interfaces that were actually removed.
        /// @param [in,out] setAddressesTo Addresses to set. If not 0, all addresses not in this set
        ///                                 will be removed, and new addresses will be added. It will be modified
        ///                                 to include only the addresses that were not active before, but now are.
        /// @param [in,out] setRoutesTo Routes to set. If not 0, all routes not in this set
        ///                              will be removed, and new routes will be added. It will be modified
        ///                                 to include only the routes that were not active before, but now are.
        void intUpdateIfaces (
            HashMap<int, NetManagerTypes::Interface> & updateData,
            HashSet<int> & removeIfaces,
            HashSet<NetManagerTypes::Address> * setAddressesTo,
            HashSet<NetManagerTypes::Route> * setRoutesTo );

        /// @brief A helper function that removes or deactivates routes based on interface state changes.
        /// It generates an update with all route removals.
        /// @param [in,out] updateData The map of updated interface data (iface_id:data).
        ///                             It is used to determine which interfaces are being deactivated.
        ///                             It will be modified - all interfaces that are also in 'removeIfaces'
        ///                             set will be removed from 'updateData'.
        /// @param [in,out] removeIfaces The set of interface IDs to remove. It will be modified to include only
        ///                               the interfaces that actually exist.
        /// @param [in] removeRoutesExcept If not 0, all routes NOT in this set will be removed - in addition to
        ///                                 removals caused by interface changes.
        void intDeactivateIfaceRoutes (
            HashMap<int, NetManagerTypes::Interface> & updateData,
            HashSet<int> & removeIfaces,
            const HashSet<NetManagerTypes::Route> * removeRoutesExcept );

        /// @brief A helper function that removes or deactivates addresses based on interface state changes.
        /// It generates an update with all address removals.
        /// @param [in,out] updateData The map of updated interface data (iface_id:data).
        ///                             It is used to determine which interfaces are being deactivated.
        ///                             It will be modified - all interfaces that are also in 'removeIfaces'
        ///                             set will be removed from 'updateData'.
        /// @param [in,out] removeIfaces The set of interface IDs to remove. It will be modified to include only
        ///                               the interfaces that actually exist.
        /// @param [in] removeAddressesExcept If not 0, all addresses NOT in this set will be removed - in addition to
        ///                                    removals caused by interface changes.
        void intDeactivateIfaceAddresses (
            HashMap<int, NetManagerTypes::Interface> & updateData,
            HashSet<int> & removeIfaces,
            const HashSet<NetManagerTypes::Address> * removeAddressesExcept );
};
}
