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

#include "basic/SubHelpers.hpp"
#include "CtrlSubHandler.hpp"

/// @def SUB_FIELD_CTRL_HANDLER(field_type, field_name, ctrl_req_type, ctrl_msg_type)
/// Generates a CtrlSubHandler that can be used to allow subscriptions to a specific field over the control channel.
/// @note This declares a class that uses CtrlLink owner's 'auto delete' feature (see comments of CtrlSubHandler
/// constructor). This means that a new object of this type can be created (using 'new', not on the stack!!),
/// and it will be automatically deleted by the CtrlLink owner once it's no longer needed.
/// @note This does not declare the field itself, just a control handler for a field already declared using one
///        of the SUB_FIELD_* macros.
/// @note This handler takes a reference to the field it should work with. This field has to exist during
///        the entire lifetime of any handler created this way!
/// field_type is the internal data type, it could be a simple number/string, or a collection.
/// field_name is the name of the field, which should use CamelCase.
/// ctrl_req_type is the type of the subscription request message
/// ctrl_msg_type is the type of the update message
#define SUB_FIELD_CTRL_HANDLER( field_type, field_name, ctrl_req_type, ctrl_msg_type ) \
    public: \
        class field_name ## CtrlSubHandler: \
            protected CtrlSubSimpleHandler<ctrl_msg_type>, protected field_name ## Receiver \
        { \
            public: \
                field_name ## CtrlSubHandler ( field_name ## Field & myField, \
                                               CtrlLink::Owner & ctrlOwner, \
                                               bool autoDelete = true ): \
                    CtrlSubSimpleHandler ( ctrlOwner, ctrl_req_type::DEF_TYPE, autoDelete ), _myField ( myField ) \
                { \
                    _myField.subscribe ( this ); \
                } \
            protected: \
                field_name ## Field & _myField; \
                virtual void updated ## field_name ( const field_type &value ) \
                { \
                    ctrl_msg_type msg; \
                    msg.setValue ( value ); \
                    ctrlSubPublish ( msg ); \
                } \
                virtual void ctrlSubActive ( bool active ) \
                { \
                    if ( active ) { _myField.subscribe ( this ); } \
                    else { _myField.unsubscribe ( this ); } \
                } \
                virtual ERRCODE ctrlSubAdd ( CtrlLink *, const Ctrl::SubscriptionRequest &, \
                                             Ctrl::SimpleSubscriptionResponse & respMsg ) \
                { \
                    ctrl_msg_type msg; \
                    respMsg.modUpdates().append ( msg.setValue ( _myField.get() ) ); \
                    return Error::Success; \
                } \
        }; \
    private:
