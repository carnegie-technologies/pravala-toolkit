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

#include "HashSet.hpp"

namespace Pravala
{
/// @brief A template class that simplifies creation of objects that publish notifications for their subscribers.
/// @tparam S The type of the subscriber.
template<typename S> class Publisher
{
    public:
        /// @brief Should be inherited by all subscribing classes.
        class Subscriber
        {
            protected:
                /// @brief Called when something subscribes with this object as a subscriber.
                /// It adds the publisher to this object's list of publishers.
                /// @param [in] publisher Publisher that this object is subscribed to.
                inline void addPublisher ( Publisher<S> * publisher )
                {
                    assert ( publisher != 0 );
                    assert ( !_publishers.contains ( publisher ) );

                    _publishers.insert ( publisher );
                }

                /// @brief Called when something unsubscribes with this object as a subscriber,
                /// removes the publisher from this object's list of publishers.
                /// @param [in] publisher Publisher that this object has unsubscribed from
                inline void removePublisher ( Publisher<S> * publisher )
                {
                    assert ( publisher != 0 );
                    assert ( _publishers.contains ( publisher ) );

                    _publishers.remove ( publisher );
                }

                /// @brief Virtual destructor
                /// Unsubscribes this subscriber from all publishers that it has subscribed to
                virtual ~Subscriber()
                {
                    // We REALLY need to create a copy, since _publishers will be modified when subscribers unsubscribe!

                    const HashSet<Publisher<S> *> pubs ( _publishers );

                    for ( typename HashSet<Publisher<S> *>::Iterator it ( pubs ); it.isValid(); it.next() )
                    {
                        assert ( it.value() != 0 );

                        it.value()->Publisher<S>::unsubscribe ( static_cast<S *> ( this ) );
                    }

                    assert ( _publishers.isEmpty() );

                    _publishers.clear();
                }

            private:
                HashSet<Publisher<S> *> _publishers; ///< All objects this subscriber is subscribed to.

                friend class Publisher;
        };

        /// @brief Subscribes to updates.
        /// @param [in] subscriber Subscriber that wishes to receive updates.
        /// @return True if it succeeded, false if the subscriber was 0 or it was already subscribed.
        bool subscribe ( S * subscriber )
        {
            if ( !subscriber )
                return false;

            const size_t prevSize = _subscribers.size();

            _subscribers.insert ( subscriber );

            if ( _subscribers.size() == prevSize )
                return false;

            ( static_cast<Publisher<S>::Subscriber *> ( subscriber ) )->addPublisher ( this );

            if ( prevSize == 0 )
                subscriptionsActive ( this, true );

            return true;
        }

        /// @brief Unsubscribes from updates.
        /// @param [in] subscriber Subscriber that does not wish to receive updates anymore.
        void unsubscribe ( S * subscriber )
        {
            if ( !subscriber )
                return;

            const size_t prevSize = _subscribers.size();

            _subscribers.remove ( subscriber );

            if ( _subscribers.size() == prevSize )
                return;

            ( static_cast<Publisher<S>::Subscriber *> ( subscriber ) )->removePublisher ( this );

            if ( _subscribers.size() == 0 )
                subscriptionsActive ( this, false );
        }

        /// @brief Checks whether this publisher has subscribers.
        /// @return True if this publisher has subscribers; False otherwise.
        inline bool hasSubscribers() const
        {
            return ( _subscribers.size() > 0 );
        }

    protected:

        /// @brief Virtual destructor
        virtual ~Publisher()
        {
            // We need a copy in case something modifies it:
            const HashSet<S *> subs ( _subscribers );

            for ( typename HashSet<S *>::Iterator it ( subs ); it.isValid(); it.next() )
            {
                assert ( it.value() != 0 );

                ( static_cast<Publisher<S>::Subscriber *> ( it.value() ) )->removePublisher ( this );
            }

            _subscribers.clear();
        }

        /// @brief Checks whether given subscriber is subscribed.
        /// This function operates on the original set of subscribers.
        /// It can be used while processing updates, to make sure that each subscriber is still subscribed.
        /// Some of them may stop being subscribed if they get removed inside one of the earlier callbacks
        /// during the same processing loop.
        /// @param [in] subscriber Subscriber to check
        /// @return True if given subscriber is subscribed; False otherwise
        inline bool isSubscribed ( S * subscriber ) const
        {
            return _subscribers.contains ( subscriber );
        }

        /// @brief Returns a copy of the subscriber's set.
        /// HashSet uses implicit sharing, so this is cheap, unless someone modifies it during processing.
        /// @return a copy of the subscriber's set.
        inline HashSet<S *> getSubscribers() const
        {
            return _subscribers;
        }

        /// @brief Called whenever the set of subscribers stops or starts being empty.
        /// @param [in] publisher The publisher that changed activity status.
        /// @param [in] active True if there are some subscribers; False if the last subscriber was removed.
        virtual void subscriptionsActive ( Publisher<S> * publisher, bool active )
        {
            ( void ) publisher;
            ( void ) active;
        }

    private:
        HashSet<S *> _subscribers; ///< All objects that subscribed to receive updates
};
}
