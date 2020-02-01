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

#include "CtrlSubHandler.hpp"

#include "auto/ctrl/Ctrl/Update.hpp"
#include "auto/ctrl/Ctrl/SubscriptionRequest.hpp"
#include "auto/ctrl/Ctrl/SimpleSubscriptionResponse.hpp"

using namespace Pravala;

TextLog CtrlSubHandler::_log ( "ctrl_sub_handler" );

CtrlSubHandler::CtrlSubHandler ( CtrlLink::Owner & ctrlOwner, uint32_t subId, bool autoDelete ):
    SubId ( subId ), AutoDelete ( autoDelete ), _ctrlOwner ( ctrlOwner )
{
    _ctrlOwner.ctrlAddSubHandler ( subId, this );
}

CtrlSubHandler::~CtrlSubHandler()
{
    _ctrlOwner.ctrlRemoveSubHandler ( SubId, this );
}

ERRCODE CtrlSubHandler::ctrlSendResponse (
        CtrlLink * link, Ctrl::Update & resp, const Ctrl::SubscriptionRequest & reqMsg )
{
    assert ( link != 0 );

    resp.setRequestType ( reqMsg.getType() );

    LOG ( L_DEBUG, "Sending response to request type " << reqMsg.getType() );

    if ( reqMsg.hasRequestId() )
    {
        resp.setRequestId ( reqMsg.getRequestId() );
    }

    const ERRCODE eCode = link->sendPacket ( resp );

    LOG_ERR ( IS_OK ( eCode ) ? L_DEBUG : L_ERROR, eCode, "Sending response to request type " << reqMsg.getType()
              << "; Response type: " << resp.getType() );

    return eCode;
}

CtrlSubSimpleHandlerBase::CtrlSubSimpleHandlerBase ( CtrlLink::Owner & ctrlOwner, uint32_t subId, bool autoDelete ):
    CtrlSubHandler ( ctrlOwner, subId, autoDelete )
{
}

void CtrlSubSimpleHandlerBase::ctrlLinkRemoved ( CtrlLink * link )
{
    assert ( link != 0 );

    if ( _subscribers.remove ( link ) > 0 && _subscribers.isEmpty() )
    {
        ctrlSubActive ( false );
    }
}

ERRCODE CtrlSubSimpleHandlerBase::ctrlProcessSubRequest ( CtrlLink * link, const Ctrl::SubscriptionRequest & msg )
{
    if ( !msg.hasSubRequestType() )
    {
        return Error::RequiredFieldNotSet;
    }

    if ( msg.getSubRequestType() == Ctrl::SubscriptionRequest::ReqType::Subscribe )
    {
        if ( _subscribers.contains ( link ) )
            return Error::Success;

        Ctrl::SimpleSubscriptionResponse respMsg;

        const ERRCODE handlerCode = ctrlSubAdd ( link, msg, respMsg );

        // Let's set the code received from the handler:
        respMsg.setCode ( handlerCode );

        ERRCODE retCode = handlerCode;

        if ( handlerCode != Error::ResponseSent && handlerCode != Error::ResponsePending )
        {
            // Now let's send the response
            retCode = ctrlSendResponse ( link, respMsg, msg );
        }

        // If the handler was successful, let's add the subscriber (and potentially call 'subs active')
        if ( IS_OK ( handlerCode ) || handlerCode == Error::ResponseSent || handlerCode == Error::ResponsePending )
        {
            _subscribers.insert ( link );

            if ( _subscribers.size() == 1 )
            {
                ctrlSubActive ( true );
            }
        }

        return retCode;
    }

    if ( _subscribers.remove ( link ) > 0 && _subscribers.isEmpty() )
    {
        ctrlSubActive ( false );
    }

    return Error::Success;
}

void CtrlSubSimpleHandlerBase::ctrlSubBasePublish ( Ctrl::Update & updateMsg )
{
    if ( _subscribers.isEmpty() )
        return;

    for ( HashSet<CtrlLink *>::Iterator it ( _subscribers ); it.isValid(); it.next() )
    {
        assert ( it.value() != 0 );

        const ERRCODE ret = it.value()->sendPacket ( updateMsg );

        if ( NOT_OK ( ret ) )
        {
            LOG_ERR ( L_ERROR, ret, "Error sending update message to link with ID " << it.value()->LinkId );
        }
    }
}
