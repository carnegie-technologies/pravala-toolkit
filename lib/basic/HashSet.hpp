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

#include <cstdlib>

#include "HashMap.hpp"

namespace Pravala
{
/// @brief HashSet class.
/// Offers hash-based set.
template<typename T> class HashSet: protected HashMapImpl<T, bool>
{
    public:
        /// @brief Constant iterator over all hash set's elements.
        /// It operates over a copy of the set. Thanks to implicit sharing,
        /// the data is not actually copied, unless the original set is modified.
        /// If that happens, iterator will keep iterating over the elements
        /// that were in the set when the iterator was created.
        class Iterator: protected HashMapImpl<T, bool>::Iterator
        {
            public:
                /// @brief Creates a constant iterator over given HashSet
                /// @param [in] hSet HashSet to iterate over.
                inline Iterator ( const HashSet<T> & hSet ): HashMapImpl<T, bool>::Iterator ( hSet )
                {
                }

                /// @brief Returns current value
                /// Calling this function in iterator that is not valid is illegal
                /// (and crashes the program).
                /// @return Value of current element.
                inline const T & value() const
                {
                    return HashMapImpl<T, bool>::Iterator::key();
                }

                using HashMapImpl<T, bool>::Iterator::isValid;
                using HashMapImpl<T, bool>::Iterator::next;
        };

        /// @brief Mutable iterator over HashSet.
        /// Similar to Iterator, but allows HashSet and values to be modified.
        /// @note Unlike Iterator, MutableIterator does not create a copy of the set, but iterates over the original.
        /// Modifying the set while this iterator is running by inserting or removing elements
        /// (other than by calling MutableIterator::remove()) may have unexpected consequences.
        /// It could result in iterator skipping some elements, or returning elements already returned.
        class MutableIterator: protected HashMapImpl<T, bool>::MutableIterator
        {
            public:
                /// @brief Creates a mutable iterator over given HashSet
                /// @param [in] hSet HashSet to iterate over. It CANNOT be a temporary object!
                inline MutableIterator ( HashSet<T> & hSet ): HashMapImpl<T, bool>::MutableIterator ( hSet )
                {
                }

                /// @brief Returns current value
                /// Calling this function in iterator that is not valid is illegal
                /// (and crashes the program).
                /// @return Value of current element.
                inline const T & value() const
                {
                    // This should NOT be a writable reference!
                    // We can't modify keys (or in this case, values)
                    // this way!
                    return HashMapImpl<T, bool>::MutableIterator::key();
                }

                using HashMapImpl<T, bool>::MutableIterator::isValid;
                using HashMapImpl<T, bool>::MutableIterator::next;
                using HashMapImpl<T, bool>::MutableIterator::remove;
        };

        /// @brief Default constructor.
        /// Creates an empty HashSet.
        inline HashSet()
        {
        }

        /// @brief Copy constructor.
        /// This version will not take advantage of implicit sharing!
        /// @param [in] other HashMap object to copy
        template<typename V> inline HashSet ( const HashMapImpl<T, V> & other )
        {
            insertAll ( other );
        }

        /// @brief Copy constructor.
        /// The HashSet uses implicit sharing, so the actual data
        /// is not really copied, unless one of the HashSet objects sharing
        /// data needs to modify it.
        /// @param [in] other HashSet object to copy.
        inline HashSet ( const HashMapImpl<T, bool> & other ): HashMapImpl<T, bool>::HashMapImpl ( other )
        {
        }

        /// @brief Constructor that converts a list of elements to HashSet
        /// @param [in] list A List to use for initializing the hash set
        inline HashSet ( const List<T> & list )
        {
            insertAll ( list );
        }

        /// @brief Assignment operator.
        /// This version will not take advantage of implicit sharing!
        /// @param [in] other HashMap whose keys to copy.
        /// @return Reference to this HashSet.
        template<typename V> inline HashSet & operator= ( const HashMapImpl<T, V> & other )
        {
            clear();
            insertAll ( other );
            return *this;
        }

        /// @brief Assignment operator.
        /// The HashSet uses implicit sharing, so the actual data
        /// is not really copied, unless one of the HashSet objects sharing
        /// data needs to modify it.
        /// @param [in] other HashSet object to copy.
        /// @return Reference to this HashSet.
        inline HashSet & operator= ( const HashMapImpl<T, bool> & other )
        {
            HashMapImpl<T, bool>::operator= ( other );
            return *this;
        }

        /// @brief Assignment operator.
        /// Converts a list of elements to HashSet
        /// @param [in] list A List to use for initializing the hash set
        /// @return Reference to this HashSet.
        inline HashSet & operator= ( const List<T> & list )
        {
            clear();

            for ( size_t idx = 0; idx < list.size(); ++idx )
            {
                insert ( list.at ( idx ) );
            }

            return *this;
        }

        /// @brief Cast to a List operator.
        /// @return A list that includes all elements of this HashSet.
        inline operator List<T> ( ) const
        {
            List<T> list;

            for ( Iterator it ( *this ); it.isValid(); it.next() )
            {
                list.append ( it.value() );
            }

            return list;
        }

        /// @brief Compares two HashSets to determine equality.
        /// Two lists are equal if they contain the same elements.
        /// @param [in] other The second sets to compare
        /// @return A value indicating whether the two sets are equal
        bool operator== ( const HashSet & other ) const
        {
            return HashMapImpl<T, bool>::operator== ( other );
        }

        /// @brief Compares two HashSets to determine inequality.
        /// Two lists are equal if they contain the same elements.
        /// @param [in] other The second sets to compare
        /// @return A value indicating whether the two sets are inequal.
        inline bool operator!= ( const HashSet & other ) const
        {
            return !( *this == other );
        }

        /// @brief Inserts the given value to the set.
        /// @param [in] val Value to insert.
        /// @return Reference to this object.
        inline HashSet & insert ( const T & val )
        {
            HashMapImpl<T, bool>::insert ( val, true );

            return *this;
        }

        /// @brief Inserts all elements of another HashSet to this set.
        /// @param [in] other The HashSet whose elements to add to this one.
        /// @return Reference to this object.
        template<typename V> inline HashSet & insertAll ( const HashMapImpl<T, V> & other )
        {
            for ( typename HashMapImpl<T, V>::Iterator it ( other ); it.isValid(); it.next() )
            {
                insert ( it.key() );
            }

            return *this;
        }

        /// @brief Inserts all elements of another HashSet to this set.
        /// @param [in] other The HashSet whose elements to add to this one.
        /// @return Reference to this object.
        inline HashSet & insertAll ( const HashSet<T> & other )
        {
            HashMapImpl<T, bool>::insertAll ( other );
            return *this;
        }

        /// @brief Inserts all elements from the list provided to this set.
        /// @param [in] list The List from which elements to add to this set.
        /// @return Reference to this object.
        inline HashSet & insertAll ( const List<T> & list )
        {
            const List<T> tmpCopy ( list );

            for ( size_t idx = 0; idx < tmpCopy.size(); ++idx )
            {
                insert ( tmpCopy.at ( idx ) );
            }

            return *this;
        }

        /// @brief Removes from this HashSet all the elements that are found, as keys, in the given HashMap.
        /// @param [in] hMap The HashMap whose keys to remove from this HashSet.
        /// @return Number of values removed.
        template<typename V> inline size_t removeAll ( const HashMapImpl<T, V> & hMap )
        {
            size_t cnt = 0;

            for ( typename HashMapImpl<T, V>::Iterator it ( hMap ); it.isValid(); it.next() )
            {
                cnt += remove ( it.key() );
            }

            return cnt;
        }

        /// @brief Removes all elements of another HashSet from this set.
        /// @param [in] other The HashSet whose elements to remove from this one.
        /// @return Number of values removed.
        inline size_t removeAll ( const HashSet<T> & other )
        {
            return HashMapImpl<T, bool>::removeAll ( other );
        }

        /// @brief Removes all elements from the list provided from this set.
        /// @param [in] list The List from which elements to remove from this set.
        /// @return Number of values removed.
        inline size_t removeAll ( const List<T> & list )
        {
            size_t cnt = 0;
            const List<T> tmpCopy ( list );

            for ( size_t idx = 0; idx < tmpCopy.size(); ++idx )
            {
                cnt += remove ( tmpCopy.at ( idx ) );
            }

            return cnt;
        }

        using HashMapImpl<T, bool>::size;
        using HashMapImpl<T, bool>::isEmpty;
        using HashMapImpl<T, bool>::contains;
        using HashMapImpl<T, bool>::remove;
        using HashMapImpl<T, bool>::clear;
        using HashMapImpl<T, bool>::getRefCount;
};

DECLARE_EMBEDDED_TEMPLATE_TYPE_INFO ( HashSet, TYPE_CONF_STD_SHARED );
}
