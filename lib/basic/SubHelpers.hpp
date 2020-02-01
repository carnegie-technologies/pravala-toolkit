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

#include "Publisher.hpp"

/// @def SUB_FIELD_CLASSES(field_type, field_name)
/// Generates receiver and publisher classes for a field with get, set and (un)subscribe functions.
/// field_type is the internal data type, it could be a simple number/string, or a collection.
/// field_name is the name of the field, which should use CamelCase.
#define SUB_FIELD_CLASSES( field_type, field_name ) \
    public: \
        class field_name ## Receiver: public Publisher<field_name ## Receiver>::Subscriber \
        { \
            public: \
                virtual void updated ## field_name ( const field_type &value ) = 0; \
        }; \
    private: \
        class field_name ## Publisher: public Publisher<field_name ## Receiver> \
        { \
            public: \
                field_name ## Publisher(): _value ( field_type() ) {} \
                inline const field_type & get() const { return _value; } \
            protected: \
                field_type _value; \
                void set ( const field_type &value ) \
                { \
                    if ( value == _value ) return; \
                    _value = value; \
                    const HashSet<field_name ## Receiver *> subs \
                        = Publisher<field_name ## Receiver>::getSubscribers(); \
                    for ( HashSet<field_name ## Receiver *>::Iterator it ( subs ); it.isValid(); it.next() ) \
                    { \
                        assert ( it.value() != 0 ); \
                        if ( Publisher<field_name ## Receiver>::isSubscribed ( it.value() ) ) \
                            it.value()->updated ## field_name ( get() ); \
                    } \
                } \
        };

/// @def SUB_ARG_FIELD_CLASSES(field_type, field_name)
/// Generates receiver and publisher classes for a field with get, set and (un)subscribe functions.
/// This version also passes an extra argument when the value changes.
/// field_type is the internal data type, it could be a simple number/string, or a collection.
/// field_name is the name of the field, which should use CamelCase.
/// arg_type is the type of the extra argument to be passed in the callback.
#define SUB_ARG_FIELD_CLASSES( field_type, field_name, arg_type ) \
    public: \
        class field_name ## Receiver: public Publisher<field_name ## Receiver>::Subscriber \
        { \
            public: \
                virtual void updated ## field_name ( const arg_type &arg, const field_type &value ) = 0; \
        }; \
    private: \
        class field_name ## Publisher: public Publisher<field_name ## Receiver> \
        { \
            public: \
                field_name ## Publisher(): _value ( field_type() ) {} \
                inline const field_type & get() const { return _value; } \
            protected: \
                field_type _value; \
                void set ( const arg_type &arg, const field_type &value ) \
                { \
                    if ( value == _value ) return; \
                    _value = value; \
                    const HashSet<field_name ## Receiver *> subs \
                        = Publisher<field_name ## Receiver>::getSubscribers(); \
                    for ( HashSet<field_name ## Receiver *>::Iterator it ( subs ); it.isValid(); it.next() ) \
                    { \
                        assert ( it.value() != 0 ); \
                        if ( Publisher<field_name ## Receiver>::isSubscribed ( it.value() ) ) \
                            it.value()->updated ## field_name ( arg, get() ); \
                    } \
                } \
        };

/// @def SUB_PUB_FIELD(field_type, field_name)
/// It generates SUB_FIELD_CLASSES and actually adds the field of field_name ## Publisher type.
/// This version will expose this field's set() method publicly.
/// field_type is the internal data type, it could be a simple number/string, or a collection.
/// field_name is the name of the field, which should use CamelCase.
#define SUB_PUB_FIELD( field_type, field_name ) \
    SUB_FIELD_CLASSES ( field_type, field_name ) \
    public: \
        class field_name ## Field: public field_name ## Publisher { \
            public: using field_name ## Publisher::set; \
        } \
        f ## field_name; \
    private:

/// @def SUB_FRIEND_FIELD(friend_class, field_type, field_name)
/// It generates SUB_FIELD_CLASSES and actually adds the field of field_name ## Publisher type.
/// This version will declare a friend class.
/// friend_class is the name of the class that should be befriended (to have access to 'set' method)
/// field_type is the internal data type, it could be a simple number/string, or a collection.
/// field_name is the name of the field, which should use CamelCase.
#define SUB_FRIEND_FIELD( friend_class, field_type, field_name ) \
    SUB_FIELD_CLASSES ( field_type, field_name ) \
    public: \
        class field_name ## Field: public field_name ## Publisher { \
            friend class friend_class; \
        } \
        f ## field_name; \
    private:

/// @def SUB_FRIEND_FIELD_AND_SET(friend_class, field_type, field_name)
/// It generates SUB_FRIEND_FIELD and adds a protected set method in the current object.
/// friend_class is the name of the class that should be befriended - it should be the external class' name!
/// field_type is the internal data type, it could be a simple number/string, or a collection.
/// field_name is the name of the field, which should use CamelCase.
#define SUB_FRIEND_FIELD_AND_SET( friend_class, field_type, field_name ) \
    SUB_FRIEND_FIELD ( friend_class, field_type, field_name ) \
    protected: virtual void set ## field_name ( const field_type &value ) { f ## field_name.set ( value ); } \
    private:

/// @def SUB_PUB_EXT_FIELD(field_type, field_name)
/// It generates SUB_FIELD_CLASSES and actually adds the field of field_name ## Publisher type.
/// This version will expose this field's set() method publicly.
/// It will also declare a subscriptionsActive() method that should be implemented.
/// field_type is the internal data type, it could be a simple number/string, or a collection.
/// field_name is the name of the field, which should use CamelCase.
#define SUB_PUB_EXT_FIELD( field_type, field_name ) \
    SUB_FIELD_CLASSES ( field_type, field_name ) \
    public: \
        class field_name ## Field: public field_name ## Publisher { \
            protected: virtual void subscriptionsActive ( Publisher<field_name ## Receiver> *, bool ); \
            public: using field_name ## Publisher::set; \
        } \
        f ## field_name; \
    private:

/// @def SUB_FRIEND_EXT_FIELD(friend_class, field_type, field_name)
/// It generates SUB_FIELD_CLASSES and actually adds the field of field_name ## Publisher type.
/// This version will declare a friend class.
/// It will also declare a subscriptionsActive() method that should be implemented.
/// friend_class is the name of the class that should be befriended (to have access to 'set' method)
/// field_type is the internal data type, it could be a simple number/string, or a collection.
/// field_name is the name of the field, which should use CamelCase.
#define SUB_FRIEND_EXT_FIELD( friend_class, field_type, field_name ) \
    SUB_FIELD_CLASSES ( field_type, field_name ) \
    public: \
        class field_name ## Field: public field_name ## Publisher { \
            protected: virtual void subscriptionsActive ( Publisher<field_name ## Receiver> *, bool ); \
                friend class friend_class; \
        } \
        f ## field_name; \
    private:

/// @def SUB_FRIEND_EXT_FIELD_AND_SET(friend_class, field_type, field_name)
/// It generates SUB_FRIEND_EXT_FIELD and adds a protected set method in the current object.
/// friend_class is the name of the class that should be befriended - it should be the external class' name!
/// field_type is the internal data type, it could be a simple number/string, or a collection.
/// field_name is the name of the field, which should use CamelCase.
#define SUB_FRIEND_EXT_FIELD_AND_SET( friend_class, field_type, field_name ) \
    SUB_FRIEND_EXT_FIELD ( friend_class, field_type, field_name ) \
    protected: virtual void set ## field_name ( const field_type &value ) { f ## field_name.set ( value ); } \
    private:

/// @def SUB_PUB_ARG_FIELD(field_type, field_name, arg_type)
/// It generates SUB_ARG_FIELD_CLASSES and actually adds the field of field_name ## Publisher type.
/// This version will expose this field's set() method publicly.
/// field_type is the internal data type, it could be a simple number/string, or a collection.
/// field_name is the name of the field, which should use CamelCase.
/// arg_type is the type of the extra argument to be passed in the callback.
#define SUB_PUB_ARG_FIELD( field_type, field_name, arg_type ) \
    SUB_ARG_FIELD_CLASSES ( field_type, field_name, arg_type ) \
    public: \
        class field_name ## Field: public field_name ## Publisher { \
            public: using field_name ## Publisher::set; \
        } \
        f ## field_name; \
    private:

/// @def SUB_FRIEND_ARG_FIELD(friend_class, field_type, field_name, arg_type)
/// It generates SUB_ARG_FIELD_CLASSES and actually adds the field of field_name ## Publisher type.
/// This version will declare a friend class.
/// friend_class is the name of the class that should be befriended (to have access to 'set' method)
/// field_type is the internal data type, it could be a simple number/string, or a collection.
/// field_name is the name of the field, which should use CamelCase.
/// arg_type is the type of the extra argument to be passed in the callback.
#define SUB_FRIEND_ARG_FIELD( friend_class, field_type, field_name, arg_type ) \
    SUB_ARG_FIELD_CLASSES ( field_type, field_name, arg_type ) \
    public: \
        class field_name ## Field: public field_name ## Publisher { \
            friend class friend_class; \
        } \
        f ## field_name; \
    private:

/// @def SUB_FRIEND_ARG_FIELD_AND_SET(friend_class, field_type, field_name, arg_type)
/// It generates SUB_FRIEND_ARG_FIELD and adds a protected set method in the current object.
/// friend_class is the name of the class that should be befriended - it should be the external class' name!
/// field_type is the internal data type, it could be a simple number/string, or a collection.
/// field_name is the name of the field, which should use CamelCase.
/// arg_type is the type of the extra argument to be passed in the callback.
#define SUB_FRIEND_ARG_FIELD_AND_SET( friend_class, field_type, field_name, arg_type ) \
    SUB_FRIEND_ARG_FIELD ( friend_class, field_type, field_name, arg_type ) \
    protected: \
        virtual void set ## field_name ( const arg_type &arg, const field_type &value ) \
        { f ## field_name.set ( arg, value ); } \
    private:

/// @def SUB_PUB_ARG_EXT_FIELD(field_type, field_name, arg_type)
/// It generates SUB_ARG_FIELD_CLASSES and actually adds the field of field_name ## Publisher type.
/// This version will expose this field's set() method publicly.
/// It will also declare a subscriptionsActive() method that should be implemented.
/// field_type is the internal data type, it could be a simple number/string, or a collection.
/// field_name is the name of the field, which should use CamelCase.
/// arg_type is the type of the extra argument to be passed in the callback.
#define SUB_PUB_EXT_ARG_FIELD( field_type, field_name, arg_type ) \
    SUB_ARG_FIELD_CLASSES ( field_type, field_name, arg_type ) \
    public: \
        class field_name ## Field: public field_name ## Publisher { \
            protected: virtual void subscriptionsActive ( Publisher<field_name ## Receiver> *, bool ); \
            public: using field_name ## Publisher::set; \
        } \
        f ## field_name; \
    private:

/// @def SUB_FRIEND_ARG_EXT_FIELD(friend_class, field_type, field_name, arg_type)
/// It generates SUB_ARG_FIELD_CLASSES and actually adds the field of field_name ## Publisher type.
/// This version will declare a friend class.
/// It will also declare a subscriptionsActive() method that should be implemented.
/// friend_class is the name of the class that should be befriended (to have access to 'set' method)
/// field_type is the internal data type, it could be a simple number/string, or a collection.
/// field_name is the name of the field, which should use CamelCase.
/// arg_type is the type of the extra argument to be passed in the callback.
#define SUB_FRIEND_EXT_ARG_FIELD( friend_class, field_type, field_name, arg_type ) \
    SUB_ARG_FIELD_CLASSES ( field_type, field_name, arg_type ) \
    public: \
        class field_name ## Field: public field_name ## Publisher { \
            protected: virtual void subscriptionsActive ( Publisher<field_name ## Receiver> *, bool ); \
                friend class friend_class; \
        } \
        f ## field_name; \
    private:

/// @def SUB_FRIEND_ARG_EXT_FIELD_AND_SET(friend_class, field_type, field_name, arg_type)
/// It generates SUB_FRIEND_ARG_EXT_FIELD and adds a protected set method in the current object.
/// friend_class is the name of the class that should be befriended - it should be the external class' name!
/// field_type is the internal data type, it could be a simple number/string, or a collection.
/// field_name is the name of the field, which should use CamelCase.
/// arg_type is the type of the extra argument to be passed in the callback.
#define SUB_FRIEND_ARG_EXT_FIELD_AND_SET( friend_class, field_type, field_name, arg_type ) \
    SUB_FRIEND_ARG_EXT_FIELD ( friend_class, field_type, field_name, arg_type ) \
    protected: \
        virtual void set ## field_name ( const arg_type &arg, const field_type &value ) \
        { f ## field_name.set ( arg, value ); } \
    private:
