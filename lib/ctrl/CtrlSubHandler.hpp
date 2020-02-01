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

#include "basic/HashSet.hpp"
#include "error/Error.hpp"
#include "log/TextLog.hpp"
#include "CtrlLink.hpp"

namespace Pravala
{
namespace Ctrl
{
class Update;
class SubscriptionRequest;
class SimpleSubscriptionResponse;
}

/// @brief Control Subscription Handler.
/// To be inherited by classes that want to handle control subscription requests.
class CtrlSubHandler
{
    public:
        const uint32_t SubId; ///< The ID of the subscription request this handler is responsible for.
        const bool AutoDelete; ///< Whether this object is set to be auto-deleted.

    protected:
        static TextLog _log; ///< Log stream.

        CtrlLink::Owner & _ctrlOwner; ///< The CtrlLink's owner this handler is operating for.

        /// @brief Constructor.
        /// This will register this handler with a specified CtrlLink's owner.
        /// @param [in] ctrlOwner The CtrlLink's owner this handler will be operating for.
        /// @param [in] subId The ID of the subscription request this handler is responsible for.
        ///                   If there already is another handler that uses the same ID, it will be removed.
        /// @param [in] autoDelete If set to true (default), the owner will be responsible for removing this object
        ///                         when it's no longer needed - in that case the caller should not use it directly.
        ///                        If set to false, it's the callers responsibility to remove this object.
        CtrlSubHandler ( CtrlLink::Owner & ctrlOwner, uint32_t subId, bool autoDelete = true );

        /// @brief Destructor.
        virtual ~CtrlSubHandler();

        /// @brief Sends a control response.
        /// @param [in] link The CtrlLink over which the response should be sent.
        /// @param [in] resp The response message.
        /// @param [in] reqMsg The request message.
        /// @return Standard error code. Unless something is very wrong, it is a 'ResponseSent' code.
        ERRCODE ctrlSendResponse ( CtrlLink * link, Ctrl::Update & resp, const Ctrl::SubscriptionRequest & reqMsg );

        /// @brief Called when the CtrlLink receives a SubscriptionRequest message for this handler.
        /// @param [in] link The CtrlLink over which the request was received.
        /// @param [in] msg The message received.
        /// @return Standard error code.
        virtual ERRCODE ctrlProcessSubRequest ( CtrlLink * link, const Ctrl::SubscriptionRequest & msg ) = 0;

        /// @brief Called when the link is going away.
        /// It should unsubscribe this link from updates (if it is subscribed).
        /// @param [in] link The link which is not valid anymore.
        virtual void ctrlLinkRemoved ( CtrlLink * link ) = 0;

        friend class CtrlLink;
        friend class CtrlLink::Owner;
};

/// @brief The base of the Simple Control Subscription Handler.
/// It most likely should not be inherited directly, but through CtrlSubSimpleHandler - which provides
/// better type control.
/// It handles simple control subscriptions that don't need additional parameters - links are either subscribed or not.
/// This class manages the list of those subscribers and provides convenience functions.
class CtrlSubSimpleHandlerBase: protected CtrlSubHandler
{
    public:
        /// @brief Returns true if this handler has any subscribers.
        /// @return True if this handler has any subscribers; False otherwise.
        inline bool hasSubscribers() const
        {
            return ( !_subscribers.isEmpty() );
        }

        using CtrlSubHandler::SubId;
        using CtrlSubHandler::AutoDelete;

    protected:
        /// @brief Constructor.
        /// This will register this handler with a specified server.
        /// @param [in] ctrlOwner The CtrlLink's owner this handler will be operating for.
        /// @param [in] subId The ID of the subscription request this handler is responsible for.
        ///                   If there already is another handler that uses the same ID, it will be removed.
        /// @param [in] autoDelete If set to true (default), the server will be responsible for removing this object
        ///                         when it's no longer needed - in that case the caller should not use it directly.
        ///                        If set to false, it's the callers responsibility to remove this object.
        CtrlSubSimpleHandlerBase ( CtrlLink::Owner & ctrlOwner, uint32_t subId, bool autoDelete = true );

        /// @brief Publishes an update to all the subscribers.
        /// @param [in] updateMsg The message to send to all subscribers.
        void ctrlSubBasePublish ( Ctrl::Update & updateMsg );

        /// @brief Called whenever the state of subscriptions changes.
        /// @param [in] active If true, it means that the first subscriber has been added;
        ///                    If false, it means that the last subscriber went away.
        virtual void ctrlSubActive ( bool active ) = 0;

        /// @brief Called when a link tries to subscribe.
        /// @param [in] link The CtrlLink that subscribed.
        /// @param [in] reqMsg The message received.
        /// @param [out] respMsg The response message that will be sent as a response to the request.
        ///                       If this function returns 'ResponseSent' code, the response will not be sent.
        ///                       Otherwise, this message will be sent with the error code returned by this function.
        ///                       This includes 'Success' code, but also any error code. This code will be set
        ///                       in respMsg after this function returns.
        /// @return Standard error code.
        ///         If it returns 'ResponseSent', then it means it succeeded and no other response will be sent.
        ///         Otherwise, the 'respMsg' with the code returned (the code will be set after calling this function)
        ///         will be sent.
        virtual ERRCODE ctrlSubAdd (
            CtrlLink * link,
            const Ctrl::SubscriptionRequest & reqMsg,
            Ctrl::SimpleSubscriptionResponse & respMsg ) = 0;

        virtual void ctrlLinkRemoved ( CtrlLink * link );
        virtual ERRCODE ctrlProcessSubRequest ( CtrlLink * link, const Ctrl::SubscriptionRequest & msg );

    private:
        HashSet<CtrlLink * > _subscribers; ///< The set of links that are subscribed.
};

/// @brief Simple Control Subscription Handler.
/// It is a wrapper around CtrlSubSimpleHandlerBase and adds better type safety.
/// @tparam T The type of the update messages. It has to inherit Ctrl::Update.
template<typename T> class CtrlSubSimpleHandler: public CtrlSubSimpleHandlerBase
{
    public:
        /// @brief Publishes an update to all the subscribers.
        /// @param [in] updateMsg The message to send to all subscribers.
        inline void ctrlSubPublish ( T & updateMsg )
        {
            ctrlSubBasePublish ( updateMsg );
        }

    protected:
        /// @brief Constructor.
        /// This will register this handler with a specified server.
        /// @param [in] ctrlOwner The CtrlLink's owner this handler will be operating for.
        /// @param [in] subId The ID of the subscription request this handler is responsible for.
        ///                   If there already is another handler that uses the same ID, it will be removed.
        /// @param [in] autoDelete If set to true (default), the server will be responsible for removing this object
        ///                         when it's no longer needed - in that case the caller should not use it directly.
        ///                        If set to false, it's the callers responsibility to remove this object.
        inline CtrlSubSimpleHandler ( CtrlLink::Owner & ctrlOwner, uint32_t subId, bool autoDelete = true ):
            CtrlSubSimpleHandlerBase ( ctrlOwner, subId, autoDelete )
        {
        }
};
}
