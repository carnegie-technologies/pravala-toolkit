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

#ifdef SYSTEM_WINDOWS
extern "C"
{
#include <guiddef.h>
}
#endif

#include "List.hpp"

namespace Pravala
{
/// @brief Returns the hashing code for the value provided.
/// @param [in] hKey The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
inline size_t getHash ( uint8_t hKey )
{
    return ( size_t ) hKey;
}

/// @brief Returns the hashing code for the value provided.
/// @param [in] hKey The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
inline size_t getHash ( int8_t hKey )
{
    return ( size_t ) hKey;
}

/// @brief Returns the hashing code for the value provided.
/// @param [in] hKey The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
inline size_t getHash ( char hKey )
{
    return ( size_t ) hKey;
}

/// @brief Returns the hashing code for the value provided.
/// @param [in] hKey The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
inline size_t getHash ( uint16_t hKey )
{
    return ( size_t ) hKey;
}

/// @brief Returns the hashing code for the value provided.
/// @param [in] hKey The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
inline size_t getHash ( int16_t hKey )
{
    return ( size_t ) hKey;
}

/// @brief Returns the hashing code for the value provided.
/// @param [in] hKey The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
inline size_t getHash ( uint32_t hKey )
{
    return hKey;
}

/// @brief Returns the hashing code for the value provided.
/// @param [in] hKey The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
inline size_t getHash ( int32_t hKey )
{
    return ( size_t ) hKey;
}

/// @brief Returns the hashing code for the value provided.
/// @param [in] hKey The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
inline size_t getHash ( uint64_t hKey )
{
    return ( size_t ) ( ( ( hKey >> ( 8 * sizeof ( size_t ) - 1 ) ) ^ hKey ) & ( ~0U ) );
}

/// @brief Returns the hashing code for the value provided.
/// @param [in] hKey The value used as a key, used for generating the hashing code.
/// @return The hashing code for the value provided.
inline size_t getHash ( int64_t hKey )
{
    return ( size_t ) ( ( ( hKey >> ( 8 * sizeof ( size_t ) - 1 ) ) ^ hKey ) & ( ~0U ) );
}

#ifdef SYSTEM_WINDOWS
/// @brief Returns the hashing code for the Windows GUID provided.
/// @param guid The Windows GUID
/// @return The hashing code for the value provided.
inline size_t getHash ( const GUID & guid )
{
    return getHash ( ( uint32_t ) guid.Data1 )
           ^ getHash ( ( uint16_t ) guid.Data2 )
           ^ getHash ( ( uint16_t ) guid.Data3 )
           ^ getHash ( ( uint32_t ) &guid.Data4[ 0 ] )
           ^ getHash ( ( uint32_t ) &guid.Data4[ 4 ] );
}
#endif

/// @brief A convenience function: Generates a hash value from two unsigned 16bit values
/// It actually just returns the bitsum of the first value shifted by 16 bits, and the second value
/// @param [in] hKeyA The first unsigned 16bit value
/// @param [in] hKeyB The second unsigned 16bit value
/// @return The hash value to use
inline size_t getHash ( uint16_t hKeyA, uint16_t hKeyB )
{
    return ( size_t ) ( ( ( ( uint32_t ) hKeyA ) << 16 ) | ( ( uint32_t ) hKeyB ) );
}

/// @brief Returns the hashing code for the pointer provided.
/// This version simply hashes the pointer and has nothing to do with the value it points to.
/// @param [in] hKey The value used as a key, used for generating the hashing code.
/// @tparam T The type of the value that hKey points to.
/// @return The hashing code for the pointer provided.
template<class T> inline size_t getHash ( const T * hKey )
{
    return getHash ( ( ptr_cast_t ) hKey );
}

/// @brief Private data of the HashMap.
/// Common for all HashMap objects, no matter what type they use.
/// It is NOT thread safe!
class HashMapPriv
{
    public:
        /// @brief Structure storing two pointers - keys and values of specific hash entry.
        struct PointerPair
        {
            SharedMemory::Pointer keys; ///< Keys of this hash table entry
            SharedMemory::Pointer values; ///< Values of this hash table entry
        };

        /// @brief Copy constructor.
        /// Creates an empty HashMapPriv object with reference
        /// count set to 1.
        /// It does NOT copy the memory from the original HashMapPriv.
        /// It does NOT remove one reference from the original HashMapPriv.
        /// This class is NOT thread safe!
        HashMapPriv ( HashMapPriv * other, bool increaseSize );

        /// @brief Destructor
        ~HashMapPriv();

        /// @brief Reference counter for this data segment.
        SharedMemory::RefCounter ref;

        /// @brief Pointer to allocated memory
        PointerPair * buffer;

        /// @brief Size of the memory
        size_t bufSize;

        /// @brief Number of elements stored in this HashMap
        size_t elementCount;

    private:
        /// @brief Doesn't exist.
        HashMapPriv ( HashMapPriv & );

        /// @brief Doesn't exist.
        HashMapPriv();

        /// @brief Doesn't exist.
        const HashMapPriv & operator= ( const HashMapPriv & );
};

/// @brief HashMapImpl class
/// To be inherited by the regular HashMap class, or specialized HashMap classes
/// Offers hash-based dictionary
template<typename K, typename V> class HashMapImpl
{
    private:
        /// @brief Functionality shared between constant and mutable iterator classes.
        class IteratorCore
        {
            protected:
                /// @brief Current index in the hash table
                size_t _hIdx;

                /// @brief Current index in the list under current index of the hash table
                size_t _lIdx;

                /// @brief Whether the iterator is valid.
                /// If this is set to false, iterator is not valid even if the indexes are.
                bool _isValid;

                /// @brief Default constructor.
                IteratorCore(): _hIdx ( 0 ), _lIdx ( 0 ), _isValid ( false )
                {
                }

                /// @brief Checks if this iterator is pointing to an existing value in the HashMap.
                /// @param [in] hMap The HashMapImpl object to use.
                /// @return true if key/value can be retrieved; false otherwise.
                inline bool isValid ( const HashMapImpl<K, V> & hMap ) const
                {
                    return ( _isValid
                             && hMap._priv != 0
                             && _hIdx < hMap._priv->bufSize
                             && hMap._priv->buffer[ _hIdx ].keys != 0
                             && _lIdx < hMap.keys ( _hIdx )->size() );
                }

                /// @brief Sets the iterator to the first entry of the map.
                /// If the map is empty, this iterator will be invalid.
                /// @param [in] hMap The HashMapImpl object to use.
                /// @return True if the iterator is valid after this operation; False otherwise.
                bool findBeginning ( const HashMapImpl<K, V> & hMap );

                /// @brief Advances the iterator.
                /// It is safe to call this function on invalid iterator.
                /// It returns true if the new current key and value can be retrieved.
                /// It returns false otherwise, but 'false' doesn't mean that the iterator
                /// has not been advanced, it only means that the new current value
                /// is illegal. So it's the same as a result of following function:
                /// bool func(){ iterator.next(); return iterator.isValid(); }
                /// @param [in] hMap The HashMapImpl object to use.
                /// @return True if the current key/value can be retrieved, false otherwise.
                bool next ( const HashMapImpl<K, V> & hMap );

                /// @brief Returns current key
                /// Calling this function in iterator that is not valid is illegal
                /// (and crashes the program).
                /// @param [in] hMap The HashMapImpl object to use.
                /// @return Key of current element.
                const K & key ( const HashMapImpl<K, V> & hMap ) const;

                /// @brief Returns current value
                /// Calling this function in iterator that is not valid is illegal
                /// (and crashes the program).
                /// @param [in] hMap The HashMapImpl object to use.
                /// @return Value of current element.
                const V & value ( const HashMapImpl<K, V> & hMap ) const;

                /// @brief Returns a writable reference to the current value
                /// Calling this function in iterator that is not valid is illegal
                /// (and crashes the program).
                /// There is no similar function that would return a writable key
                /// of the current element, because it's not possible to modify
                /// the key this way.
                /// @param [in] hMap The HashMapImpl object to use.
                /// @return Writable reference to the value of current element.
                V & value ( HashMapImpl<K, V> & hMap );

                /// @brief Removes current value and advances the iterator.
                /// This function is similar to the next(), but instead of
                /// skipping the current value, it removes it.
                /// Calling it on the iterator that is not valid is legal.
                /// It returns true if the new value is legal, false otherwise.
                /// Even if it returns false, it doesn't mean that element
                /// has not been removed. It was removed (unless the iterator was
                /// already invalid), but that value was the last one.
                /// @param [in] hMap The HashMapImpl object to use.
                /// @return True if the new value is valid, false otherwise.
                bool remove ( HashMapImpl<K, V> & hMap );
        };

    public:
        /// @brief Constant iterator over all hash map's elements.
        /// It operates over a copy of the map. Thanks to implicit sharing,
        /// the data is not actually copied, unless the original map is modified.
        /// If that happens, iterator will keep iterating over the elements
        /// that were in the map when the iterator was created.
        class Iterator: public HashMapImpl<K, V>::IteratorCore
        {
            public:
                /// @brief Creates a constant iterator over given HashMap
                /// @param [in] hMap HashMap to iterate over.
                Iterator ( const HashMapImpl<K, V> & hMap );

                /// @brief Resets this iterator to the beginning of the map.
                /// @return True if the iterator is valid after this operation; False otherwise.
                inline bool reset()
                {
                    return IteratorCore::findBeginning ( _myMap );
                }

                /// @brief Checks if this iterator is pointing to an existing value in the HashMap.
                /// @return true if key/value can be retrieved; false otherwise.
                inline bool isValid() const
                {
                    return IteratorCore::isValid ( _myMap );
                }

                /// @brief Advances the iterator.
                /// It is safe to call this function on invalid iterator.
                /// It returns true if the new current key and value can be retrieved.
                /// It returns false otherwise, but 'false' doesn't mean that the iterator
                /// has not been advanced, it only means that the new current value
                /// is illegal. So it's the same as a result of following function:
                /// bool func(){ iterator.next(); return iterator.isValid(); }
                /// @return True if the current key/value can be retrieved, false otherwise.
                inline bool next()
                {
                    return IteratorCore::next ( _myMap );
                }

                /// @brief Returns current key
                /// Calling this function in iterator that is not valid is illegal
                /// (and crashes the program).
                /// @return Key of current element.
                inline const K & key() const
                {
                    return IteratorCore::key ( _myMap );
                }

                /// @brief Returns current value
                /// Calling this function in iterator that is not valid is illegal
                /// (and crashes the program).
                /// @return Value of current element.
                inline const V & value() const
                {
                    return IteratorCore::value ( _myMap );
                }

            private:
                /// @brief A copy of the HashMap (we iterate over a copy)
                HashMapImpl<K, V> _myMap;
        };

        /// @brief MutableIterator over all hash map's elements.
        /// Similar to Iterator, but allows HashMap and values to be modified.
        /// @note Unlike Iterator, MutableIterator does not create a copy of the map, but iterates over the original.
        /// Modifying the map while this iterator is running by inserting or removing elements
        /// (other than by calling MutableIterator::remove()) may have unexpected consequences.
        /// It could result in iterator skipping some elements, returning elements already returned,
        /// or becoming invalid.
        class MutableIterator: public HashMapImpl<K, V>::IteratorCore
        {
            public:
                /// @brief Creates a mutable iterator over given HashMap
                /// @param [in] map HashMap to iterate over. It CANNOT be a temporary object!
                MutableIterator ( HashMapImpl<K, V> & hMap );

                /// @brief Resets this iterator to the beginning of the map.
                /// @return True if the iterator is valid after this operation; False otherwise.
                inline bool reset()
                {
                    return IteratorCore::findBeginning ( _myMap );
                }

                /// @brief Checks if this iterator is pointing to an existing value in the HashMap.
                /// @return true if key/value can be retrieved; false otherwise.
                inline bool isValid() const
                {
                    return IteratorCore::isValid ( _myMap );
                }

                /// @brief Advances the iterator.
                /// It is safe to call this function on invalid iterator.
                /// It returns true if the new current key and value can be retrieved.
                /// It returns false otherwise, but 'false' doesn't mean that the iterator
                /// has not been advanced, it only means that the new current value
                /// is illegal. So it's the same as a result of following function:
                /// bool func(){ iterator.next(); return iterator.isValid(); }
                /// @return True if the current key/value can be retrieved, false otherwise.
                inline bool next()
                {
                    return IteratorCore::next ( _myMap );
                }

                /// @brief Returns current key
                /// Calling this function in iterator that is not valid is illegal
                /// (and crashes the program).
                /// @return Key of current element.
                inline const K & key() const
                {
                    return IteratorCore::key ( _myMap );
                }

                /// @brief Returns current value
                /// Calling this function in iterator that is not valid is illegal
                /// (and crashes the program).
                /// @return Value of current element.
                inline const V & value() const
                {
                    return IteratorCore::value ( _myMap );
                }

                /// @brief Returns a writable reference to the current value
                /// Calling this function in iterator that is not valid is illegal
                /// (and crashes the program).
                /// There is no similar function that would return a writable key
                /// of the current element, because it's not possible to modify
                /// the key this way.
                /// @return Writable reference to the value of current element.
                inline V & value()
                {
                    return IteratorCore::value ( _myMap );
                }

                /// @brief Removes current value and advances the iterator.
                /// This function is similar to the next(), but instead of
                /// skipping the current value, it removes it.
                /// Calling it on the iterator that is not valid is legal.
                /// It returns true if the new value is legal, false otherwise.
                /// Even if it returns false, it doesn't mean that element
                /// has not been removed. It was removed (unless the iterator was
                /// already invalid), but that value was the last one.
                /// @return True if the new value is valid, false otherwise.
                inline bool remove()
                {
                    return IteratorCore::remove ( _myMap );
                }

            private:
                /// @brief Writable reference to the HashMap
                HashMapImpl<K, V> & _myMap;
        };

        /// @brief Default constructor.
        /// Creates an empty HashMap.
        inline HashMapImpl(): _priv ( 0 )
        {
        }

        /// @brief Copy constructor.
        /// The HashMap uses implicit sharing, so the actual data
        /// is not really copied, unless one of the HashMap objects sharing
        /// data needs to modify it.
        /// @param [in] other HashMap object to copy.
        HashMapImpl ( const HashMapImpl & other );

        /// @brief Assignment operator.
        /// The HashMap uses implicit sharing, so the actual data
        /// is not really copied, unless one of the HashMap objects sharing
        /// data needs to modify it.
        /// @param [in] other HashMap object to copy.
        /// @return Reference to this HashMap.
        HashMapImpl & operator= ( const HashMapImpl & other );

        /// @brief Destructor.
        /// Since internal objects are stored in the lists, the behaviour of the destructor
        /// is the same as destructing lists declared as List<K> and List<V>.
        ~HashMapImpl();

        /// @brief Returns number of objects stored in the HashMap.
        /// @return Number of objects stored in the HashMap.
        inline size_t size() const
        {
            return ( _priv != 0 ) ? _priv->elementCount : 0;
        }

        /// @brief Returns true if the HashMap is empty
        /// @return True if the HashMap is empty
        inline bool isEmpty() const
        {
            return ( size() == 0 );
        }

        /// @brief Compares two HashMaps to determine equality.
        /// Two lists are equal if they contain the same elements.
        /// @param [in] other The second map to compare
        /// @return A value indicating whether the two maps are equal
        bool operator== ( const HashMapImpl & other ) const;

        /// @brief Compares two HashMaps to determine inequality.
        /// Two lists are equal if they contain the same elements.
        /// @param [in] other The second maps to compare
        /// @return A value indicating whether the two maps are inequal.
        inline bool operator!= ( const HashMapImpl & other ) const
        {
            return !( *this == other );
        }

        /// @brief Returns a value associated with a given key.
        /// If this HashMap doesn't include any value for the given key,
        /// the value constructed using default constructor is returned.
        /// @param [in] hKey Key for which the value should be returned.
        /// @return The value.
        V value ( const K & hKey ) const;

        /// @brief Returns a reference to the value associated with a given key.
        /// If the HashMap doesn't contain the requested key, the new element
        /// is created using its default constructor,
        /// and a reference to it is returned.
        /// @param [in] hKey Key for which the value should be returned.
        /// @return Reference to the value.
        V & operator[] ( const K & hKey );

        /// @brief Inserts the given key:value pair.
        /// If there already is a value associated with the given key,
        /// it is overwritten with the new one.
        /// @param [in] hKey Key to use.
        /// @param [in] hVal Value to insert.
        /// @return Reference to this object.
        HashMapImpl & insert ( const K & hKey, const V & hVal );

        /// @brief Inserts all entries from the other hash map into this hash map.
        /// If there already is a value associated with the given key,
        /// it is overwritten with the new one.
        /// @param [in] other The other HashMap from which entries should be added.
        /// @return Reference to this object.
        HashMapImpl & insertAll ( const HashMapImpl & other );

        /// @brief Checks if a given key exists in the HashMap.
        /// @param [in] hKey Key to look for.
        /// @return true if the given key has been found; false otherwise.
        bool contains ( const K & hKey ) const;

        /// @brief Finds an entry with the given key.
        /// If is similar to contains(), but if the key is found, the value associated with it is copied to value.
        /// @param [in] hKey Key to look for.
        /// @param [out] value A reference to the value to be set if the entry is found.
        /// @return True if the entry with the given key was found; False otherwise.
        bool find ( const K & hKey, V & value ) const;

        /// @brief Finds and removes an entry with the given key.
        /// It works like find(), but it also removes the entry from the map.
        /// @param [in] hKey Key to look for.
        /// @param [out] value A reference to the value to be set if the entry is found.
        /// @return True if the entry with the given key was found; False otherwise.
        bool findAndRemove ( const K & hKey, V & value );

        /// @brief Removes all values associated with the given key.
        /// @param [in] hKey Key to remove.
        /// @return Number of values removed (either 1 or 0).
        size_t remove ( const K & hKey );

        /// @brief Removes from this hash map all the entries whose keys are found (as keys) in the other HashMap.
        /// @param [in] other The HashMap whose keys should be removed from this one.
        /// @return Number of values removed.
        template<typename T> size_t removeAll ( const HashMapImpl<K, T> & other )
        {
            size_t cnt = 0;

            if ( _priv == other._priv )
            {
                cnt = size();
                clear();
                return cnt;
            }

            for ( typename HashMapImpl<K, T>::Iterator it ( other ); it.isValid(); it.next() )
            {
                cnt += remove ( it.key() );
            }

            return cnt;
        }

        /// @brief Clears the HashMap.
        /// Removes all the elements from the HashMap.
        void clear();

        /// @brief Returns a value of reference counter.
        /// This is probably useful only for debugging.
        /// @return Value of reference counter associated with shared data.
        inline uint32_t getRefCount() const
        {
            return ( _priv != 0 ) ? _priv->ref.count() : 1;
        }

    private:
        /// @brief Pointer to internal HashMap's data
        /// Should always be pointing to something (either valid
        /// data or shared null segment).
        HashMapPriv * _priv;

        /// @brief Ensures that the private data of this HashMap is not shared.
        /// If the internal data is shared with some other HashMap, its copy is created.
        /// @param [in] resize If it is true, the hash table might be resized
        /// (depending on its current size, number of elements, and the fact if we are
        /// copying the data or not).
        void ensureOwnCopy ( bool resize );

        /// @brief Unreferences current shared data
        /// Deletes the elements if needed (this map was the only user of this data)
        void unrefPriv();

        /// @brief Helper function for converting void* to a list of keys.
        /// @param [in] index Index of the hash table
        /// @return Const pointer to the list of keys at the given index.
        inline const List<K> * keys ( size_t index ) const
        {
            assert ( _priv != 0 );
            assert ( _priv->bufSize > 0 );
            assert ( index < _priv->bufSize );

            return reinterpret_cast<const List<K> *> ( _priv->buffer[ index ].keys );
        }

        /// @brief Helper function for converting void* to a list of keys.
        /// @param [in] index Index of the hash table
        /// @return Pointer to the list of keys at the given index.
        inline List<K> * keys ( size_t index )
        {
            assert ( _priv != 0 );
            assert ( _priv->bufSize > 0 );
            assert ( index < _priv->bufSize );

            return reinterpret_cast<List<K> *> ( _priv->buffer[ index ].keys );
        }

        /// @brief Helper function for converting void* to a list of values.
        /// @param [in] index Index of the hash table
        /// @return Const pointer to the list of values at the given index.
        inline const List<V> * values ( size_t index ) const
        {
            assert ( _priv != 0 );
            assert ( _priv->bufSize > 0 );
            assert ( index < _priv->bufSize );

            return reinterpret_cast<const List<V> *> ( _priv->buffer[ index ].values );
        }

        /// @brief Helper function for converting void* to a list of values.
        /// @param [in] index Index of the hash table
        /// @return Pointer to the list of values at the given index.
        inline List<V> * values ( size_t index )
        {
            assert ( _priv != 0 );
            assert ( _priv->bufSize > 0 );
            assert ( index < _priv->bufSize );

            return reinterpret_cast<List<V> *> ( _priv->buffer[ index ].values );
        }

        /// @brief Helper function that removes an element with a given hash and list index.
        /// Indexes have to describe existing element!
        /// @param [in] hIdx Index in the hash table.
        /// @param [in] lIdx Index in the list at the given hash table cell.
        void removeInternal ( size_t hIdx, size_t lIdx );

        /// @brief Helper function that inserts given key/value pair to the given shared memory segment.
        /// Has to be called with legal HashMapPriv, which has reference count set to 1!
        /// @param [in] hMapPriv Shared memory segment to add the element to.
        /// @param [in] hKey Key of the element.
        /// @param [in] hVal Value of the element.
        /// @param [in] checkExisting Should the given hMapPriv be tested for existing elements. If it is true,
        /// the element will be added to the appropriate list in the hash only if there is no existing element
        /// with the same key. If there is existing element with the same key, its value will me overwritten
        /// with the new one. If checkExisting is false, the element will be appended to that list
        /// without checking anything. It is useful for reallocating entire hash table (and is faster),
        /// but could result in multiple entries with the same key if used improperly.
        /// @return The reference to the element just inserted (or modified)
        static V & internalInsert ( HashMapPriv * hMapPriv, const K & hKey, const V & hVal, bool checkExisting );

        /// @brief Helper function for converting void* to a list of keys.
        /// @param [in] hMapPriv Shared memory segment to operate on.
        /// @param [in] index Index of the hash table
        /// @return Pointer to the list of keys at the given index.
        static inline List<K> * keys ( HashMapPriv * hMapPriv, size_t index )
        {
            return reinterpret_cast<List<K> *> ( hMapPriv->buffer[ index ].keys );
        }

        /// @brief Helper function for converting void* to a list of values.
        /// @param [in] hMapPriv Shared memory segment to operate on.
        /// @param [in] index Index of the hash table
        /// @return Pointer to the list of values at the given index.
        static inline List<V> * values ( HashMapPriv * hMapPriv, size_t index )
        {
            return reinterpret_cast<List<V> *> ( hMapPriv->buffer[ index ].values );
        }
};

template<typename K, typename V> HashMapImpl<K, V>::HashMapImpl ( const HashMapImpl<K, V> & other ):
    _priv ( const_cast<HashMapPriv *> ( other._priv ) )
{
    if ( _priv != 0 )
    {
        _priv->ref.ref();
    }
}

template<typename K, typename V> void HashMapImpl<K, V>::unrefPriv()
{
    if ( !_priv ) return;

    // Unlink from that data. If we were the only user of it - delete it

    if ( _priv->ref.unref() )
    {
        assert ( _priv->ref.count() == 0 );

        for ( size_t i = 0; i < _priv->bufSize; ++i )
        {
            if ( _priv->buffer[ i ].keys != 0 )
            {
                delete keys ( i );
            }

            if ( _priv->buffer[ i ].values != 0 )
            {
                delete values ( i );
            }
        }

        delete _priv;
    }

    _priv = 0;
}

template<typename K, typename V> HashMapImpl<K, V> & HashMapImpl<K, V>::operator= ( const HashMapImpl<K, V> & other )
{
    if ( &other == this || _priv == other._priv )
        return *this;

    unrefPriv();

    assert ( !_priv );

    if ( other._priv != 0 )
    {
        _priv = const_cast<HashMapPriv *> ( other._priv );

        assert ( _priv != 0 );

        _priv->ref.ref();
    }

    return *this;
}

template<typename K, typename V> HashMapImpl<K, V>::~HashMapImpl()
{
    unrefPriv();

    assert ( !_priv );
}

template<typename K, typename V> bool HashMapImpl<K, V>::operator== ( const HashMapImpl<K, V> & other ) const
{
    if ( &other == this || _priv == other._priv )
    {
        return true;
    }

    if ( size() != other.size() )
    {
        return false;
    }

    for ( Iterator it ( other ); it.isValid(); it.next() )
    {
        if ( !contains ( it.key() ) || value ( it.key() ) != it.value() )
            return false;
    }

    return true;
}

template<typename K, typename V> V HashMapImpl<K, V>::value ( const K & hKey ) const
{
    if ( _priv != 0 && _priv->elementCount > 0 )
    {
        const size_t hIdx = getHash ( hKey ) % _priv->bufSize;

        if ( _priv->buffer[ hIdx ].keys != 0 )
        {
            const List<K> & keyList = *keys ( hIdx );

            for ( size_t idx = 0; idx < keyList.size(); ++idx )
            {
                if ( keyList.at ( idx ) == hKey )
                {
                    return values ( hIdx )->at ( idx );
                }
            }
        }
    }

    return V();
}

template<typename K, typename V> V & HashMapImpl<K, V>::operator[] ( const K & hKey )
{
    bool incSize = false;

    if ( _priv != 0 && _priv->bufSize / 2 < _priv->elementCount )
    {
        // This means that we are potentially adding a new element.
        // If that happens, we may have to increase the size of the buffer,
        // but only if we actually need to add more elements!
        // So let's check.

        incSize = true;

        const size_t hIdx = getHash ( hKey ) % _priv->bufSize;

        if ( _priv->buffer[ hIdx ].keys != 0 )
        {
            const List<K> & keyList = *keys ( hIdx );

            for ( size_t idx = 0; idx < keyList.size(); ++idx )
            {
                if ( keyList.at ( idx ) == hKey )
                {
                    // We have that key!
                    // If we are the only owner of this _priv, we can simply return it,
                    // no need to do anything else!

                    if ( _priv->ref.count() < 2 )
                    {
                        assert ( _priv->ref.count() == 1 );

                        return values ( hIdx )->operator[] ( idx );
                    }

                    // Otherwise we need to create our own copy.
                    // But, since we know that we already have this element, there is no need to increase the size.

                    incSize = false;
                    break;
                }
            }
        }
    }

    ensureOwnCopy ( incSize );

    assert ( _priv != 0 );
    assert ( _priv->ref.count() == 1 );

    if ( _priv->elementCount > 0 )
    {
        const size_t hIdx = getHash ( hKey ) % _priv->bufSize;

        if ( _priv->buffer[ hIdx ].keys != 0 )
        {
            const List<K> & keyList = *keys ( hIdx );

            for ( size_t idx = 0; idx < keyList.size(); ++idx )
            {
                if ( keyList.at ( idx ) == hKey )
                {
                    return values ( hIdx )->operator[] ( idx );
                }
            }
        }
    }

    assert ( getRefCount() == 1 );

    return internalInsert ( _priv, hKey, V(), true );
}

template<typename K, typename V> HashMapImpl<K, V> & HashMapImpl<K, V>::insert ( const K & hKey, const V & hVal )
{
    bool incSize = false;

    if ( _priv != 0 && _priv->bufSize / 2 < _priv->elementCount )
    {
        // This means that we are potentially adding a new element.
        // If that happens, we may have to increase the size of the buffer,
        // but only if we actually need to add more elements!
        // So let's check.

        incSize = true;

        const size_t hIdx = getHash ( hKey ) % _priv->bufSize;

        if ( _priv->buffer[ hIdx ].keys != 0 )
        {
            const List<K> & keyList = *keys ( hIdx );

            for ( size_t idx = 0; idx < keyList.size(); ++idx )
            {
                if ( keyList.at ( idx ) == hKey )
                {
                    // We have that key!
                    // If we are the only owner of this _priv, we can simply return it,
                    // no need to do anything else!

                    if ( _priv->ref.count() < 2 )
                    {
                        assert ( _priv->ref.count() == 1 );

                        internalInsert ( _priv, hKey, hVal, true );

                        return *this;
                    }

                    // Otherwise we need to create our own copy.
                    // But, since we know that we already have this element, there is no need to increase the size.

                    incSize = false;
                    break;
                }
            }
        }
    }

    ensureOwnCopy ( incSize );

    assert ( _priv != 0 );
    assert ( _priv->ref.count() == 1 );

    internalInsert ( _priv, hKey, hVal, true );

    return *this;
}

template<typename K, typename V> HashMapImpl<K, V> & HashMapImpl<K, V>::insertAll ( const HashMapImpl<K, V> & other )
{
    if ( &other == this || _priv == other._priv )
        return *this;

    for ( Iterator it ( other ); it.isValid(); it.next() )
    {
        insert ( it.key(), it.value() );
    }

    return *this;
}

template<typename K, typename V> V & HashMapImpl<K, V>::internalInsert (
        HashMapPriv * hMapPriv,
        const K & hKey, const V & hVal, bool checkExisting )
{
    assert ( hMapPriv != 0 );
    assert ( hMapPriv->ref.count() == 1 );

    size_t index = getHash ( hKey ) % hMapPriv->bufSize;

    if ( hMapPriv->buffer[ index ].keys != 0 )
    {
        assert ( hMapPriv->buffer[ index ].values != 0 );
        assert ( keys ( hMapPriv, index )->size() == values ( hMapPriv, index )->size() );

        if ( checkExisting )
        {
            const List<K> & keyList = *keys ( hMapPriv, index );

            for ( size_t idx = 0; idx < keyList.size(); ++idx )
            {
                if ( keyList.at ( idx ) == hKey )
                {
                    V & ret = values ( hMapPriv, index )->operator[] ( idx );
                    ret = hVal;

                    return ret;
                }
            }
        }

        assert ( !keys ( hMapPriv, index )->findValue ( hKey ) );
    }
    else
    {
        assert ( !hMapPriv->buffer[ index ].keys );
        assert ( !hMapPriv->buffer[ index ].values );

        hMapPriv->buffer[ index ].keys = new List<K>();
        hMapPriv->buffer[ index ].values = new List<V>();
    }

    assert ( hMapPriv->buffer[ index ].keys != 0 );
    assert ( hMapPriv->buffer[ index ].values != 0 );

    keys ( hMapPriv, index )->append ( hKey );
    values ( hMapPriv, index )->append ( hVal );

    assert ( keys ( hMapPriv, index )->size() == values ( hMapPriv, index )->size() );

    ++( hMapPriv->elementCount );

    return values ( hMapPriv, index )->last();
}

template<typename K, typename V> bool HashMapImpl<K, V>::contains ( const K & hKey ) const
{
    if ( !_priv )
        return false;

    assert ( _priv->bufSize > 0 );

    const size_t hIdx = getHash ( hKey ) % _priv->bufSize;

    assert ( hIdx < _priv->bufSize );

    if ( _priv->buffer[ hIdx ].keys != 0 )
    {
        return keys ( hIdx )->findValue ( hKey );
    }

    return false;
}

template<typename K, typename V> bool HashMapImpl<K, V>::find ( const K & hKey, V & result ) const
{
    if ( !_priv )
        return false;

    assert ( _priv->bufSize > 0 );

    const size_t hIdx = getHash ( hKey ) % _priv->bufSize;

    assert ( hIdx < _priv->bufSize );

    if ( _priv->buffer[ hIdx ].keys != 0 )
    {
        const List<K> & keyList = *keys ( hIdx );

        for ( size_t idx = 0; idx < keyList.size(); ++idx )
        {
            if ( keyList.at ( idx ) == hKey )
            {
                result = values ( hIdx )->at ( idx );

                return true;
            }
        }
    }

    return false;
}

template<typename K, typename V> bool HashMapImpl<K, V>::findAndRemove ( const K & hKey, V & result )
{
    if ( !_priv )
        return false;

    ensureOwnCopy ( false );

    assert ( _priv != 0 );
    assert ( _priv->ref.count() == 1 );

    const size_t hIdx = getHash ( hKey ) % _priv->bufSize;

    if ( _priv->buffer[ hIdx ].keys != 0 )
    {
        List<K> & keyList = *keys ( hIdx );

        assert ( keyList.size() == values ( hIdx )->size() );

        for ( size_t idx = 0; idx < keyList.size(); ++idx )
        {
            if ( keyList.at ( idx ) == hKey )
            {
                result = values ( hIdx )->at ( idx );

                keyList.removeIndex ( idx );
                values ( hIdx )->removeIndex ( idx );

                assert ( keyList.size() == values ( hIdx )->size() );

                --( _priv->elementCount );

                return true;
            }
        }
    }

    // This assert validates that:
    // 1. keys and values should either both be null, or both be not null
    // 2. if they're both not null, then their sizes should be the same
    assert ( ( !_priv->buffer[ hIdx ].keys && !_priv->buffer[ hIdx ].values )
             || ( _priv->buffer[ hIdx ].keys != 0 && _priv->buffer[ hIdx ].values != 0
                  && keys ( hIdx )->size() == values ( hIdx )->size() ) );

    return false;
}

template<typename K, typename V> size_t HashMapImpl<K, V>::remove ( const K & hKey )
{
    if ( !_priv )
        return 0;

    ensureOwnCopy ( false );

    assert ( _priv != 0 );
    assert ( _priv->ref.count() == 1 );

    const size_t hIdx = getHash ( hKey ) % _priv->bufSize;

    if ( _priv->buffer[ hIdx ].keys != 0 )
    {
        List<K> & keyList = *keys ( hIdx );

        assert ( keyList.size() == values ( hIdx )->size() );

        size_t idx;

        if ( keyList.findValue ( hKey, &idx ) )
        {
            assert ( idx < keyList.size() );
            assert ( idx < values ( hIdx )->size() );
            assert ( keyList.at ( idx ) == hKey );

            keyList.removeIndex ( idx );
            values ( hIdx )->removeIndex ( idx );

            assert ( keyList.size() == values ( hIdx )->size() );

            --( _priv->elementCount );

            return 1;
        }
    }

    // This assert validates that:
    // 1. keys and values should either both be null, or both be not null
    // 2. if they're both not null, then their sizes should be the same
    assert ( ( !_priv->buffer[ hIdx ].keys && !_priv->buffer[ hIdx ].values )
             || ( _priv->buffer[ hIdx ].keys != 0 && _priv->buffer[ hIdx ].values != 0
                  && keys ( hIdx )->size() == values ( hIdx )->size() ) );

    return 0;
}

template<typename K, typename V> void HashMapImpl<K, V>::removeInternal ( size_t hIdx, size_t lIdx )
{
    ensureOwnCopy ( false );

    assert ( _priv != 0 );
    assert ( _priv->ref.count() == 1 );
    assert ( hIdx < _priv->bufSize );
    assert ( _priv->buffer[ hIdx ].keys != 0 );
    assert ( lIdx < keys ( hIdx )->size() );
    assert ( keys ( hIdx )->size() == values ( hIdx )->size() );

    keys ( hIdx )->removeIndex ( lIdx );
    values ( hIdx )->removeIndex ( lIdx );

    --( _priv->elementCount );

    assert ( keys ( hIdx )->size() == values ( hIdx )->size() );
}

template<typename K, typename V> void HashMapImpl<K, V>::clear()
{
    if ( !_priv )
        return;

    assert ( _priv != 0 );

    unrefPriv();

    assert ( !_priv );
}

template<typename K, typename V> void HashMapImpl<K, V>::ensureOwnCopy ( bool resize )
{
    if ( !_priv || _priv->ref.count() > 1 )
    {
        size_t curSize = 0;

        if ( _priv != 0 )
            curSize = _priv->bufSize;

        // Create a new HashMapPriv and, optionally, increase
        // its size if needed - since we are planning to
        // do some additions (if resize == true)
        // This does NOT remove one reference from orgPriv
        HashMapPriv * newPriv = new HashMapPriv ( _priv, resize );

        for ( size_t i = 0; i < curSize; ++i )
        {
            const List<K> * k = keys ( i );
            const List<V> * v = values ( i );

            if ( k != 0 )
            {
                assert ( k != 0 );
                assert ( v != 0 );

                size_t sz = k->size();
                assert ( sz == v->size() );

                for ( size_t j = 0; j < sz; ++j )
                {
                    internalInsert ( newPriv, k->operator[] ( j ), v->operator[] ( j ), false );
                }
            }
        }

        // Unreference at the end
        if ( _priv != 0 )
        {
            _priv->ref.unref();
            assert ( _priv->ref.count() > 0 );

            // No delete, something else still uses that priv
        }

        _priv = newPriv;

        assert ( _priv != 0 );

        return;
    }

    if ( resize )
    {
        assert ( _priv != 0 );
        assert ( _priv->ref.count() == 1 );

        size_t curSize = _priv->bufSize;

        // Create a new HashMapPriv and, optionally, increase
        // its size if needed - since we are planning to
        // do some additions (if resize == true)
        // This does NOT remove one reference from orgPriv
        HashMapPriv * newPriv = new HashMapPriv ( _priv, resize );

        for ( size_t i = 0; i < curSize; ++i )
        {
            List<K> * k = keys ( i );
            List<V> * v = values ( i );

            if ( k != 0 )
            {
                assert ( k != 0 );
                assert ( v != 0 );

                size_t sz = k->size();
                assert ( sz == v->size() );

                for ( size_t j = 0; j < sz; ++j )
                {
                    internalInsert ( newPriv, k->operator[] ( j ), v->operator[] ( j ), false );
                }

                delete k;
                delete v;
            }
            else
            {
                assert ( !k );
                assert ( !v );
            }
        }

        assert ( _priv->ref.count() == 1 );

        // Unreference at the end
        _priv->ref.unref();

        assert ( _priv->ref.count() == 0 );

        // Delete, nothing else uses that priv
        delete _priv;
        _priv = newPriv;
    }

    assert ( _priv != 0 );
}

template<typename K, typename V> bool HashMapImpl<K, V>::IteratorCore::findBeginning ( const HashMapImpl<K, V> &hMap )
{
    if ( hMap._priv != 0 )
    {
        const size_t hSize = hMap._priv->bufSize;

        for ( size_t i = 0; i < hSize; ++i )
        {
            if ( hMap._priv->buffer[ i ].keys != 0 && hMap.keys ( i )->size() > 0 )
            {
                _hIdx = i;
                _lIdx = 0;
                _isValid = true;
                return true;
            }
        }
    }

    _hIdx = 0;
    _lIdx = 0;
    _isValid = false;
    return false;
}

template<typename K, typename V> bool HashMapImpl<K, V>::IteratorCore::next ( const HashMapImpl<K, V> &hMap )
{
    if ( !isValid ( hMap ) )
    {
        _hIdx = _lIdx = 0;
        _isValid = false;
        return false;
    }

    assert ( _isValid
             && hMap._priv != 0
             && _hIdx < hMap._priv->bufSize
             && hMap._priv->buffer[ _hIdx ].keys != 0
             && _lIdx < hMap.keys ( _hIdx )->size() );

    if ( ++_lIdx < hMap.keys ( _hIdx )->size() )
    {
        return true;
    }

    _lIdx = 0;

    const size_t hSize = hMap._priv->bufSize;

    while ( ++_hIdx < hSize )
    {
        if ( hMap._priv->buffer[ _hIdx ].keys != 0 && hMap.keys ( _hIdx )->size() > 0 )
        {
            return true;
        }
    }

    _hIdx = 0;
    _isValid = false;

    return false;
}

template<typename K, typename V> const K &HashMapImpl<K, V>::IteratorCore::key (
    const HashMapImpl<K, V> &hMap ) const
{
    if ( isValid ( hMap ) )
    {
        assert ( _isValid
                 && hMap._priv != 0
                 && _hIdx < hMap._priv->bufSize
                 && hMap._priv->buffer[ _hIdx ].keys != 0
                 && _lIdx < hMap.keys ( _hIdx )->size() );

        return hMap.keys ( _hIdx )->operator[] ( _lIdx );
    }

    assert ( false );

    abort();
}

template<typename K, typename V> const V &HashMapImpl<K, V>::IteratorCore::value (
    const HashMapImpl<K, V> &hMap ) const
{
    if ( isValid ( hMap ) )
    {
        assert ( _isValid
                 && hMap._priv != 0
                 && _hIdx < hMap._priv->bufSize
                 && hMap._priv->buffer[ _hIdx ].keys != 0
                 && _lIdx < hMap.keys ( _hIdx )->size() );

        return hMap.values ( _hIdx )->operator[] ( _lIdx );
    }

    assert ( false );

    abort();
}

template<typename K, typename V> V & HashMapImpl<K, V>::IteratorCore::value ( HashMapImpl<K, V> &hMap )
{
    if ( isValid ( hMap ) )
    {
        assert ( _isValid
                 && hMap._priv != 0
                 && _hIdx < hMap._priv->bufSize
                 && hMap._priv->buffer[ _hIdx ].keys != 0
                 && _lIdx < hMap.keys ( _hIdx )->size() );

        hMap.ensureOwnCopy ( false );
        return hMap.values ( _hIdx )->operator[] ( _lIdx );
    }

    assert ( false );

    abort();
}

template<typename K, typename V> bool HashMapImpl<K, V>::IteratorCore::remove ( HashMapImpl<K, V> &hMap )
{
    if ( !isValid ( hMap ) )
    {
        _hIdx = _lIdx = 0;
        _isValid = false;
        return false;
    }

    assert ( _isValid
             && hMap._priv != 0
             && _hIdx < hMap._priv->bufSize
             && hMap._priv->buffer[ _hIdx ].keys != 0
             && _lIdx < hMap.keys ( _hIdx )->size() );

    hMap.removeInternal ( _hIdx, _lIdx );

    if ( _lIdx < hMap.keys ( _hIdx )->size() )
    {
        // We are done. The lists removed the requested elements
        // and we are just pointing to the next one (which has lower index now).
        return true;
    }

    // We are showing past the end of the list. We need to find the next valid list.

    _lIdx = 0;

    size_t hSize = hMap._priv->bufSize;

    while ( ++_hIdx < hSize )
    {
        if ( hMap._priv->buffer[ _hIdx ].keys != 0 && hMap.keys ( _hIdx )->size() > 0 )
        {
            return true;
        }
    }

    _hIdx = 0;
    _isValid = false;
    return false;
}

template<typename K, typename V> HashMapImpl<K, V>::Iterator::Iterator ( const HashMapImpl<K, V> & hMap ):
    _myMap ( hMap )
{
    IteratorCore::findBeginning ( _myMap );
}

template<typename K, typename V> HashMapImpl<K, V>::MutableIterator::MutableIterator ( HashMapImpl<K, V> & hMap ):
    _myMap ( hMap )
{
    IteratorCore::findBeginning ( _myMap );
}

template<typename K, typename V> class HashMap: public HashMapImpl<K, V>
{
};

DECLARE_EMBEDDED_2TEMPLATE_TYPE_INFO ( HashMapImpl, TYPE_CONF_STD_SHARED );
DECLARE_EMBEDDED_2TEMPLATE_TYPE_INFO ( HashMap, TYPE_CONF_STD_SHARED );
}
