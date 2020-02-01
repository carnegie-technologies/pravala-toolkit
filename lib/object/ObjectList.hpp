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

#include <cassert>
#include <cstring>

#include "Object.hpp"

namespace Pravala
{
template< class ObjectType, typename ArgType > class ObjectListEntry:
    public Object<ObjectListEntry<ObjectType, ArgType> >
{
    public:
        ArgType arg;
        ObjectType * obj;
        ObjectListEntry * next;

        static ArgType & getInvalidArg()
        {
            static ArgType invalidArg;

            return invalidArg;
        }

        static ObjectListEntry * generate ( ObjectType * nObj )
        {
            ObjectListEntry<ObjectType, ArgType> * ptr
                = Object<ObjectListEntry<ObjectType, ArgType> >::getFromPool();

            assert ( ptr != 0 );

            assert ( !ptr->obj );
            assert ( !ptr->next );

            ptr->next = 0;
            ptr->obj = nObj;
            memset ( &( ptr->arg ), 0, sizeof ( ArgType ) );

            nObj->simpleRef();

            return ptr;
        }

        ObjectListEntry(): obj ( 0 ), next ( 0 )
        {
        }

        ~ObjectListEntry()
        {
            assert ( !next );
            assert ( !obj );
        }

        void returnsToPool()
        {
            assert ( !next );
            assert ( obj != 0 );

            obj->simpleUnref();
            obj = 0;
        }

        typedef ObjectListEntry<ObjectType, ArgType> TypedObjectListEntry;

        OBJECT_HDR ( TypedObjectListEntry );
};

/// @brief Template class that provides a list of objects inheriting Object<>.
/// It uses internal _next pointer field of all objects using ObjectPool.

template< class ObjectType, typename ArgType = char > class ObjectList
{
    public:
        class Iterator: public Object<Iterator>
        {
            public:
                bool isValid() const
                {
                    return ( _entry != 0 && _entry->obj != 0 );
                }

                ObjectType * getObject()
                {
                    if ( _entry != 0 )
                        return _entry->obj;

                    return 0;
                }

                const ArgType & getArg() const
                {
                    if ( _entry != 0 )
                        return _entry->arg;

                    return ObjectListEntry<ObjectType, ArgType>::getInvalidArg();
                }

                ArgType & getArg()
                {
                    if ( _entry != 0 )
                        return _entry->arg;

                    return ObjectListEntry<ObjectType, ArgType>::getInvalidArg();
                }

                void advance()
                {
                    if ( _entry != 0 )
                    {
                        ObjectListEntry<ObjectType, ArgType> * oldEntry = _entry;
                        _entry = _entry->next;

                        if ( _entry != 0 )
                        {
                            _entry->simpleRef();
                        }

                        oldEntry->simpleUnref();
                    }
                }

            private:
                static Iterator * generate ( ObjectListEntry<ObjectType, ArgType> * entry )
                {
                    Iterator * iter = Object<Iterator>::getFromPool();

                    assert ( !iter->_entry );

                    iter->_entry = entry;

                    if ( entry != 0 )
                        entry->simpleRef();

                    return iter;
                }

                void returnsToPool()
                {
                    if ( _entry != 0 )
                    {
                        _entry->simpleUnref();
                        _entry = 0;
                    }
                }

                ObjectListEntry<ObjectType, ArgType> * _entry;

                OBJECT_HDR ( Iterator );
                friend class ObjectList<ObjectType, ArgType>;
        };

        /// @brief Constructs empty ObjectList.
        ///
        ObjectList(): _size ( 0 ), _head ( 0 ), _tail ( 0 )
        {
        }

        /// @brief Destroys the list.
        ///
        /// Destroying the object unreferences all entries
        /// (which should unreference objects) that were in the list.
        ~ObjectList()
        {
            while ( _head != 0 )
            {
                ObjectListEntry<ObjectType, ArgType> * ptr = _head;
                _head = ptr->next;

                ptr->next = 0;
                ptr->simpleUnref();
            }
        }

        Iterator * generateIterator()
        {
            return Iterator::generate ( _head );
        }

        /// @brief Checks if the list is empty.
        ///
        /// @return True if the list is empty. false otherwise.
        inline bool isEmpty() const
        {
            return ( !_head );
        }

        /// @brief Returns size of the list.
        ///
        /// @return Size of the list.
        inline int size() const
        {
            return _size;
        }

        /// @brief Returns first object from the list.
        ///
        /// Does not remove that object from the list, so the next
        /// call to first() will return the same object, unless
        /// removeFirst() is called.
        /// @return First object in the list, or 0 if the list is empty.
        ObjectType * first() const
        {
            if ( !_head ) return 0;

            return _head->obj;
        }

        /// @brief Returns argument associated with the first object in the list.
        ///
        /// If the list is empty, reference to a special instance of the argument
        /// is returned. The content of that instance is not specified.
        /// @return Argument of the first object in the list.
        ArgType & firstArg()
        {
            if ( !_head )
                return ObjectListEntry<ObjectType, ArgType>::getInvalidArg();

            return _head->arg;
        }

        /// @brief Returns argument associated with the first object in the list.
        ///
        /// If the list is empty, reference to a special instance of the argument
        /// is returned. The content of that instance is not specified.
        /// @return Argument of the first object in the list.
        const ArgType & firstArg() const
        {
            if ( !_head )
                return ObjectListEntry<ObjectType, ArgType>::getInvalidArg();

            return _head->arg;
        }

        /// @brief Returns the last object from the list.
        ///
        /// Does not remove that object from the list, so the next
        /// call to last() will return the same object.
        /// @return First object in the list, or 0 if the list is empty.
        ObjectType * last() const
        {
            if ( !_tail ) return 0;

            return _tail->obj;
        }

        /// @brief Returns argument associated with the last object in the list.
        ///
        /// If the list is empty, reference to a special instance of the argument
        /// is returned. The content of that instance is not specified.
        /// @return Argument of the last object in the list.
        ArgType & lastArg()
        {
            if ( !_tail )
                return ObjectListEntry<ObjectType, ArgType>::getInvalidArg();

            return _tail->arg;
        }

        /// @brief Returns argument associated with the last object in the list.
        ///
        /// If the list is empty, reference to a special instance of the argument
        /// is returned. The content of that instance is not specified.
        /// @return Argument of the last object in the list.
        const ArgType & lastArg() const
        {
            if ( !_tail )
                return ObjectListEntry<ObjectType, ArgType>::getInvalidArg();

            return _tail->arg;
        }

        /// @brief Removes first object from the list.
        ///
        /// Removing object from a list decreases object's reference counter.
        void removeFirst()
        {
            assert ( _head != 0 );

            ObjectListEntry<ObjectType, ArgType> * ptr = _head;

            _head = _head->next;

            if ( !_head )
            {
                assert ( _tail == ptr );
                _tail = 0;
            }

            ptr->next = 0;
            ptr->simpleUnref();
            ptr = 0;

            --_size;
        }

        /// @brief Removes specified object from the list.
        ///
        /// Removing object from a list decreases object's reference counter.
        bool removeObject ( ObjectType * toRemove )
        {
            assert ( toRemove != 0 );
            assert ( _head != 0 );

            if ( !toRemove || !_head )
                return false;

            ObjectListEntry<ObjectType, ArgType> * ptr = _head;

            if ( _head->obj == toRemove )
            {
                _head = _head->next;

                if ( !_head )
                {
                    assert ( _tail == ptr );
                    _tail = 0;
                }

                ptr->next = 0;
                ptr->simpleUnref();
                ptr = 0;

                --_size;

                return true;
            }

            while ( ptr->next != 0 )
            {
                assert ( ptr->next != 0 );
                assert ( ptr->next->obj != 0 );

                if ( ptr->next->obj == toRemove )
                {
                    ObjectListEntry<ObjectType, ArgType> * rmPtr = ptr->next;

                    assert ( rmPtr != 0 );

                    ptr->next = rmPtr->next;

                    if ( !ptr->next )
                    {
                        assert ( _tail == rmPtr );

                        _tail = ptr;
                    }

                    rmPtr->next = 0;
                    rmPtr->simpleUnref();
                    rmPtr = 0;

                    --_size;

                    return true;
                }

                ptr = ptr->next;
            }

            return false;
        }

        /// @brief Appends the object at the end the list.
        ///
        /// Appending an object to a list increases object's reference count.
        /// @param [in] obj Pointer to an object to be appended to the list.
        void append ( ObjectType * obj )
        {
            assert ( obj != 0 );

            ObjectListEntry<ObjectType, ArgType> * oEntry = ObjectListEntry<ObjectType, ArgType>::generate ( obj );

            assert ( oEntry != 0 );
            assert ( !oEntry->next );

            if ( !_tail )
            {
                assert ( !_head );
                _head = _tail = oEntry;
            }
            else
            {
                assert ( _head != 0 );
                _tail->next = oEntry;
                _tail = oEntry;
            }

            ++_size;
        }

        /// @brief Appends the object at the end the list.
        ///
        /// Appending an object to a list increases object's reference count.
        /// @param [in] obj Pointer to an object to be appended to the list.
        /// @param [in] arg Argument to associate with this object
        void append ( ObjectType * obj, const ArgType & arg )
        {
            append ( obj );
            assert ( _tail != 0 );
            _tail->arg = arg;
        }

        /// @brief Inserts the object in the front of the list.
        ///
        /// Inserting an object to a list increases object's reference count.
        /// @param [in] obj Pointer to an object to be inserted into to the list.
        void insertAtFront ( ObjectType * obj )
        {
            assert ( obj != 0 );

            ObjectListEntry<ObjectType, ArgType> * oEntry = ObjectListEntry<ObjectType, ArgType>::generate ( obj );

            assert ( oEntry != 0 );
            assert ( !oEntry->next );

            if ( !_tail )
            {
                assert ( !_head );
                _head = _tail = oEntry;
            }
            else
            {
                assert ( _head != 0 );
                oEntry->next = _head;
                _head = oEntry;
            }

            ++_size;
        }

        /// @brief Inserts the object in the front of the list.
        ///
        /// Inserting an object to a list increases object's reference count.
        /// @param [in] arg Argument to associate with this object
        void insertAtFront ( ObjectType * obj, const ArgType & arg )
        {
            insertAtFront ( obj );
            assert ( _head != 0 );
            _head->arg = arg;
        }

        /// @brief Removes all elements of the list.
        void clear()
        {
            while ( !isEmpty() )
            {
                removeFirst();
            }
        }

    protected:
        int _size;
        ObjectListEntry<ObjectType, ArgType> * _head;
        ObjectListEntry<ObjectType, ArgType> * _tail;
};
}
