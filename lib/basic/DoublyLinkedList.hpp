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

extern "C"
{
#include <stdint.h>
}

#include <cstdlib>
#include <cassert>

#include "NoCopy.hpp"

namespace Pravala
{
/// @brief Doubly linked list class.
/// It is used for creating lists of objects.
/// @tparam T The type of the object to store in the list. It MUST inherit DoublyLinkedList::Entry.
/// @tparam S The type to use for storing the size of the list.
template<typename T, typename S = uint32_t> class DoublyLinkedList: private NoCopy
{
    public:
        /// @brief A base class for a single entry in the doubly linked list class.
        class Entry: private NoCopy
        {
            protected:
                /// @brief Default constructor.
                inline Entry(): _list ( 0 ), _next ( 0 ), _prev ( 0 )
                {
                }

                /// @brief Destructor.
                inline ~Entry()
                {
                    listRemove();

                    assert ( !_list );
                    assert ( !_next );
                    assert ( !_prev );
                }

                /// @brief Returns a pointer to the list this element is a part of.
                /// @return A pointer to the list this element is a part of.
                inline DoublyLinkedList * listGetList()
                {
                    return _list;
                }

                /// @brief Returns a pointer to the list this element is a part of.
                /// @return A pointer to the list this element is a part of.
                inline const DoublyLinkedList * listGetList() const
                {
                    return _list;
                }

                /// @brief Returns the next element in the list.
                /// @return The next element in the list;
                ///         0 if this element is not a part of any list, or it is the last element in the list.
                inline T * listGetNext()
                {
                    // Internally the list is circular, but it's easier to iterate over the elements
                    // (in the user code) if the last element has no 'next' element.
                    return ( _list != 0 && _next != _list->_head ) ? _next : 0;
                }

                /// @brief Returns the next element in the list.
                /// @return The next element in the list;
                ///         0 if this element is not a part of any list, or it is the last element in the list.
                inline const T * listGetNext() const
                {
                    // Internally the list is circular, but it's easier to iterate over the elements
                    // (in the user code) if the last element has no 'next' element.
                    return ( _list != 0 && _next != _list->_head ) ? _next : 0;
                }

                /// @brief Returns the previous element in the list.
                /// @return The previous element in the list;
                ///         0 if this element is not a part of any list, or it is the first element in the list.
                inline T * listGetPrev()
                {
                    // Internally the list is circular, but it's easier to iterate over the elements
                    // (in the user code) if the first element has no 'previous' element.
                    return ( _list != 0 && this != _list->_head ) ? _prev : 0;
                }

                /// @brief Returns the previous element in the list.
                /// @return The previous element in the list;
                ///         0 if this element is not a part of any list, or it is the first element in the list.
                inline const T * listGetPrev() const
                {
                    // Internally the list is circular, but it's easier to iterate over the elements
                    // (in the user code) if the first element has no 'previous' element.
                    return ( _list != 0 && this != _list->_head ) ? _prev : 0;
                }

                /// @brief Moves this element to the beginning of the same list.
                /// @return True if this element is now the head of the list (or was already);
                ///         False if it's not in any list.
                inline bool listMoveToFront()
                {
                    if ( !_list )
                    {
                        return false;
                    }

                    assert ( _next != 0 );

                    // We use 'next->prev' to get pointer to 'this' with the correct type:
                    _list->prepend ( _next->_prev );

                    assert ( _list->_head == this );

                    return true;
                }

                /// @brief Moves this element to the end of the same list.
                /// @return True if this element is now the tail of the list (or was already);
                ///         False if it's not in any list.
                inline bool listMoveToBack()
                {
                    if ( !_list )
                    {
                        return false;
                    }

                    assert ( _next != 0 );

                    // We use 'next->prev' to get pointer to 'this' with the correct type:
                    _list->append ( _next->_prev );

                    assert ( _list->_head->_prev == this );

                    return true;
                }

                /// @brief Inserts given element to the list, after 'this' element.
                /// @param [in] element The element to insert.
                /// @return True if the element was successfully inserted.
                ///         False if this element was not part of any list, it was being inserted after itself,
                ///         or element pointer was invalid.
                inline bool listInsertAfter ( T * element )
                {
                    if ( !element || element == this || !_list )
                    {
                        return false;
                    }

                    if ( element != _next )
                    {
                        element->listRemove();

                        assert ( !element->_list );
                        assert ( !element->_next );
                        assert ( !element->_prev );

                        assert ( _list != 0 );
                        assert ( _next != 0 );
                        assert ( _prev != 0 );

                        ++_list->_size;

                        element->_list = _list;
                        element->_next = _next;

                        // We use 'next->prev' to get pointer to 'this' with the correct type:
                        element->_prev = _next->_prev;

                        _next->_prev = element;
                        _next = element;
                    }
                    else if ( element == _list->_head )
                    {
                        // This happens if the element is right after us.
                        // If it also happens to be the head, the head needs to be moved
                        // (since now it's the last element).
                        _list->_head = element->_next;
                    }
                    else
                    {
                        assert ( element->_list == _list );
                    }

                    return true;
                }

                /// @brief Inserts given element to the list, before 'this' element.
                /// @param [in] element The element to insert.
                /// @return True if the element was successfully inserted.
                ///         False if this element was not part of any list, it was being inserted after itself,
                ///         or element pointer was invalid.
                inline bool listInsertBefore ( T * element )
                {
                    if ( !element || element == this || !_list )
                    {
                        return false;
                    }

                    if ( element != _prev )
                    {
                        element->listRemove();

                        assert ( !element->_list );
                        assert ( !element->_next );
                        assert ( !element->_prev );

                        assert ( _list != 0 );
                        assert ( _next != 0 );
                        assert ( _prev != 0 );

                        ++_list->_size;

                        element->_list = _list;
                        element->_prev = _prev;

                        // We use 'prev->next' to get pointer to 'this' with the correct type:
                        element->_next = _prev->_next;

                        _prev->_next = element;
                        _prev = element;
                    }
                    else
                    {
                        assert ( element->_list == _list );
                    }

                    if ( _list->_head == this )
                    {
                        // We do this even if the new element was already just before this one!
                        // It's possible that it was the tail, in which case we need to change the head.
                        _list->_head = element;
                    }

                    return true;
                }

                /// @brief Removes this object from the list.
                inline void listRemove()
                {
                    if ( !_list )
                    {
                        assert ( !_next );
                        assert ( !_prev );

                        return;
                    }

                    assert ( _next != 0 );
                    assert ( _prev != 0 );
                    assert ( _list != 0 );
                    assert ( _list->_size > 0 );

                    --_list->_size;

                    if ( _next == this )
                    {
                        // This is the only element in the list, which will become empty.
                        assert ( _prev == this );
                        assert ( _list->_head == this );
                        assert ( _list->_size == 0 );

                        _next = _prev = _list->_head = 0;
                        _list = 0;

                        return;
                    }

                    // This is NOT the only element in the list.

                    assert ( _next != this );
                    assert ( _prev != this );

                    if ( _list->_head == this )
                    {
                        // But it is list's head. Let's move it to the next one:
                        _list->_head = _next;
                    }

                    // And finally remove this element:

                    _prev->_next = _next;
                    _next->_prev = _prev;

                    _next = _prev = 0;
                    _list = 0;
                }

            private:
                /// @brief Pointer to the list this object is a part of.
                DoublyLinkedList * _list;

                /// @brief Pointer to the next entry in the list.
                T * _next;

                /// @brief Pointer to the previous entry in the list.
                T * _prev;

                friend class DoublyLinkedList<T>;
        };

        /// @brief Default constructor.
        /// Creates an empty list.
        inline DoublyLinkedList(): _head ( 0 ), _size ( 0 )
        {
        }

        /// @brief Returns the number of elements in the list.
        /// @return The number of elements in the list.
        inline S size() const
        {
            return _size;
        }

        /// @brief Checks if the list is empty.
        /// @return True if the list is empty; False otherwise.
        inline bool isEmpty() const
        {
            return ( !_head );
        }

        /// @brief Returns the first object in the list.
        /// @return The first object in the list, or 0 if the list is empty.
        inline T * getHead()
        {
            return _head;
        }

        /// @brief Returns the first object in the list.
        /// @return The first object in the list, or 0 if the list is empty.
        inline const T * getHead() const
        {
            return _head;
        }

        /// @brief Returns the last object in the list.
        /// @return The last object in the list, or 0 if the list is empty.
        inline T * getTail()
        {
            return ( _head != 0 ) ? _head->listGetPrev() : 0;
        }

        /// @brief Returns the last object in the list.
        /// @return The last object in the list, or 0 if the list is empty.
        inline const T * getTail() const
        {
            return ( _head != 0 ) ? _head->listGetPrev() : 0;
        }

        /// @brief Removes the first object from the list.
        /// @note It does NOT delete it!
        /// @return Pointer to the element removed; 0 if the list was empty.
        inline T * removeHead()
        {
            if ( !_head )
                return 0;

            T * const ptr = _head;
            ptr->listRemove();

            return ptr;
        }

        /// @brief Removes the last object from the list.
        /// @note It does NOT delete it!
        /// @return Pointer to the element removed; 0 if the list was empty.
        inline T * removeTail()
        {
            if ( !_head )
                return 0;

            T * const ptr = _head->_prev;
            ptr->listRemove();

            return ptr;
        }

        /// @brief Clears the list.
        /// @warning It only removes all the pointers from this list, it does NOT delete them!
        inline void clearList()
        {
            while ( _head != 0 )
            {
                _head->listRemove();
            }

            assert ( _size == 0 );
        }

        /// @brief Clears the list and deletes all elements.
        inline void deleteContent()
        {
            T * ptr;

            while ( ( ptr = _head ) != 0 )
            {
                ptr->listRemove();

                // Deleting it removes it from a list as well, but if deletion of the object
                // affects the list itself, it is safer to remove it in a separate step.
                delete ptr;
            }

            assert ( _size == 0 );
        }

        /// @brief Inserts element at the beginning of the list.
        /// @param [in] element The element to insert.
        inline void prepend ( T * element )
        {
            if ( !element )
                return;

            if ( !_head )
            {
                assert ( _size == 0 );

                element->listRemove();

                element->_list = this;
                _head = element->_next = element->_prev = element;
                _size = 1;
            }
            else if ( element != _head )
            {
                bool ret = _head->listInsertBefore ( element );

                ( void ) ret;
                assert ( ret );
            }

            assert ( _head == element );
            assert ( element->_list == this );
        }

        /// @brief Appends element at the end of the list.
        /// @param [in] element The element to append.
        inline void append ( T * element )
        {
            if ( !element )
                return;

            if ( !_head )
            {
                assert ( _size == 0 );

                element->listRemove();

                element->_list = this;
                _head = element->_next = element->_prev = element;
                _size = 1;
            }
            else if ( element != _head->_prev )
            {
                bool ret = _head->_prev->listInsertAfter ( element );

                ( void ) ret;
                assert ( ret );
            }

            assert ( _head != 0 );
            assert ( element->_list == this );
            assert ( element == _head->_prev );
        }

    private:
        T * _head; ///< The head of the list.
        S _size;   ///< The size of the list.
};
}
