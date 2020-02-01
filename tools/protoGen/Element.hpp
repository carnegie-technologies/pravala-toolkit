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

#include "basic/String.hpp"
#include "basic/HashSet.hpp"

namespace Pravala
{
class Symbol;

/// @brief A class that stores a single 'element' of the protocol description
///
/// An 'element' is one entry in the complex symbol. For example, each 'message'
/// is a symbol, and it usually contains several elements, which represent fields of this class.
/// Each element within a message can have either a simple (primitive) type, like int32, or string,
/// or a complex element - another message, or an enum, IP packet, etc.
///
/// There is a special type of element, which is a 'bit alias' to another field (element)
/// in this message (or another message, inherited by this one). The element that
/// is an alias doesn't take up any space. It simply points to a bit range in another
/// field. This way, one element can be used as a 'storage' field. For example,
/// it can be a single byte value (only some types can be used as storage types).
/// Then, several other elements (in the same or inheriting messages) can
/// point to different bit ranges of that storage type. For example, a 1 byte
/// storage value (8 bits) could be aliased by a 4bit alias, 2 bit alias, and two 1 bit each aliases.
///
/// The regular elements can have modifier rules (public, private, repeated, etc.), as well
/// as options (which describe additional properties, like the default value, or minimum allowed value).
/// Those properties depend on the actual type of the element.
///
/// Another type of an 'element' is the enumerator code. Each enumerator class
/// looks like a message class, but it can't inherit other messages/enums (or be inherited),
/// and each element, instead of representing a field with field code/ID is just another
/// enum's code. Instead of storing a 'field code' it stores a 'value', which is the value
/// for that specific enum's code.
///
/// Also, whenever a message 'defines' an element, that defined value is stored as a special element.
///
class Element
{
    public:
        /// The 'rule' for the element.
        ///
        /// These onle apply to regular elements, not enum values.
        ///
        /// Each element can either be required, optional or repeated.
        ///
        /// When 'required' is used, a message that doesn't have this element set will not serialize properly.
        /// When a message is deserialized, some sanity checks are performed on it. If it doesn't contain all the
        /// elements marked as 'required' it is considered broken and 'protocol error' is returned.
        ///
        /// Optional elements can either be in the message, or not.
        ///
        /// There can be a number of elements of the 'repeated' type. Those elements are not stored directly.
        /// Instead a list of elements of this type is created. An element cannot be required and repeated
        /// at the same time. To achieve 'repeated, but at least one' a special option for 'repeated' elements
        /// has to be used: 'min_list_size = 1'.
        ///
        enum ElemRule
        {
            RuleUnknown, ///< Unknown/not-set rule
            RuleRequired, ///< Element is required
            RuleOptional, ///< Element is optional
            RuleRepeated ///< Element is repeated
        };

        /// How the element can be accessed
        ///
        /// These onle apply to regular elements, not enum values.
        ///
        /// Each regular element can be public, protected or private.
        /// This modifier only applies to methods that can modify the value of the element,
        /// all getters are always public.
        ///
        /// A public element can be modified by anything.
        /// A private element can only be modified from the same class (which only really makes
        /// sense for alias storage fields aliased in the same message.
        /// A protected element can only be modified from the same message, or any message that inherits it.
        /// This mode is useful for alias storage types, so that they can't be set directly from the outside,
        /// only through their aliases.
        ///
        enum ElemAccess
        {
            AccessUnknown, ///< Access is unknown/not-set
            AccessPublic, ///< Element is public
            AccessProtected, ///< Element is protected
            AccessPrivate ///< Element is private
        };

        /// Name of the element
        String name;

        /// If enum's code uses extended syntax ("foo bar") the 'name' is simplified and the original
        /// is stored as extName. If it is a regular enum code without quotes, this field is empty.
        String extName;

        /// Value of the element, used by the elements that represent enum codes and defines.
        String value;

        /// The code/field ID of the element. Only for the regular elements (those that are part of messages),
        /// except for aliases (they are not stored directly, so they don't use codes). Defines
        /// also don't use codes.
        uint32_t code;

        /// A comment associated with the element
        String comment;

        /// Name of the type of this element
        String typeName;

        /// The beginning of the alias range (only for aliases) - the number of the first aliased bit
        String strAliasRangeFrom;

        /// The end of the alias range (only for aliases) - the number of the last aliased bit
        String strAliasRangeTo;

        /// Used temporarily during parsing, the name of the option being parsed.
        String curOption;

        /// A mapping with all 'defined' values for this element.
        /// Each message can 'define' elements in the messages that it inherits to have certain value.
        /// If this element is 'defined' to have a specific value by one of the messages, this
        /// value is stored here, together with the name of the class that defines it.
        /// The key is the defined value, the value is the name of the class.
        /// The key is used for detecting conflicts. When an element is declared as 'unique' it means
        /// that each message that wants to define it has to do that to a different value.
        /// There is an exception, messages can define values to the same value as defined by some other
        /// message by using a special syntax, but at this point they explicitly do that.
        /// The conflicts are checked only for regular defines.
        /// The value of this mapping is only for printing a meaningful error message when the conflict
        /// happens.
        HashMap<String, String> defValues;

        /// A hash map with all options set for this element.
        /// The format is option_name:option_value
        /// There are different options, depending on the type and modifiers of each element.
        HashMap<String, String> options;

        /// It is used temporarily during parsing, for detecting bit alias conflicts.
        /// Each bit in the storage type, when aliased, is marked as used in this mapping,
        /// but if it is already used by some other message, an error is returned.
        /// The format is bit_number:path_to_the_element_that_aliases_it
        /// For a multi-bit aliases, each bit of the range is marked separately in this mapping.
        HashMap<int, String> tmpAliasedBits;

        /// This is used for detecting alias-define conflicts. The elements that are
        /// used as storage types for aliases cannot be 'defined', and aliases cannot
        /// be created to elements that are 'defined'. This simply stores the name of
        /// the last message that contains an element that aliases it. It doesn't matter
        /// which of the symbols is stored here, it is only used for detecting that something
        /// aliases this element (and checked while 'defining' it).
        String lastAliasedIn;

        /// The 'rule' for this element
        ElemRule rule;

        /// The 'access' mode for this element
        ElemAccess access;

        /// Set when the element is declared as 'unique'. When an element is declared as unique,
        /// it means that when it is 'defined' to a specific value by different messages,
        /// each of them has to use a different value. Two different messages cannot define
        /// the same element to have the same value.
        bool isUnique;

        /// Points to the symbol representing the type of this element.
        /// Enum elements, defines and aliases don't have it set.
        Symbol * typeSymbol;

        /// The symbol that contains this element.
        Symbol * containerSymbol;

        /// The element whose value is defined by this element
        Element * definedTarget;

        /// The length (in the number of bits) of the alias
        int aliasTargetBitLength;

        /// The element that is used as the storage type for this element (which must be an alias)
        Element * aliasTarget;

        /// @brief Presence index, if used by the language.
        /// It's up to specific language generator to set it
        /// (it's just a helper field, not set or used by the parser itself)
        /// By default it's set to -1.
        /// If the language generator (like C++ generator) uses presence fields
        /// to determine whether an element is present or not (instead of using null like Java),
        /// instead of using a separate, boolean field for each of the elements (which uses
        /// up more space than just one bit), a special variable is used for marking the presence
        /// of several elements. Each of the elements is assigned a single bit in that field, this
        /// field is used for marking which bit it is.
        int presenceIndex;

        /// The number of the first bit of the storage field that this alias contains
        int iAliasRangeFrom;

        /// The number of the first payload bit of the storage field that this alias contains
        /// If it's a normal alias it will be the same as iAliasRangeFrom,
        /// It is is an s-alias, it will be iAliasRangeFrom + 1.
        int iAliasPayloadRangeFrom;

        /// The number of the last bit of the storage field that this alias contains
        int iAliasRangeTo;

        /// @brief Creates a new element inside the symbol given.
        /// @param [in] contSymbol The "container" symbol, the message/enumerator that this element
        ///              is included in.
        Element ( Symbol * contSymbol );

        /// @return The number of bits of actual data payload this alias spans over
        ///         If this is an s-alias, this size will be one bit smaller than the actual
        ///         'range-from - range-to' size.
        inline int getAliasPayloadBitLength() const
        {
            if ( !aliasTarget )
                return 0;

            if ( isSalias() )
            {
                return iAliasRangeTo - iAliasRangeFrom;
            }
            else
            {
                return iAliasRangeTo - iAliasRangeFrom + 1;
            }
        }

        /// @return True if this is not an alias, or an alias that can be represented using a 'full type'.
        inline bool usesFullType() const
        {
            const int numBits = getAliasPayloadBitLength();

            return ( numBits == 0 || numBits == 1 || numBits == 8 || numBits == 16 || numBits == 32 || numBits == 64 );
        }

        /// @return True if this is a 'private' element
        inline bool isPrivate() const
        {
            return ( access == AccessPrivate );
        }

        /// @return True if this is a 'public' element, or the access mode was not defined (public by default)
        inline bool isPublic() const
        {
            return ( access == AccessUnknown || access == AccessPublic );
        }

        /// @return True if this is a 'protected' element
        inline bool isProtected() const
        {
            return ( access == AccessProtected );
        }

        /// @return True if this is a 'required' element
        inline bool isRequired() const
        {
            return ( rule == RuleRequired );
        }

        /// @return True if this is a 'repeated' element
        inline bool isRepeated() const
        {
            return ( rule == RuleRepeated );
        }

        /// @return True if this is an 'optional' element
        inline bool isOptional() const
        {
            return ( rule == RuleOptional );
        }

        /// @return True if this is an 'alias' or 'salias' element.
        bool isAlias() const;

        /// @return True if this is a 'salias' element.
        bool isSalias() const;

        /// @return True if this element is the 'default' value of the enum type
        bool isEnumDefault() const;

        /// @brief Returns the camel case name of the element including an optional prefix
        ///
        /// For example, when no prefix is used it will change "field_id" to "fieldId"
        /// When we use "get_foo" prefix, it will generate "getFooFieldId".
        /// If 'usePascalCase' is set to true, it will generate FieldId and GetFooFieldId respectively.
        ///
        /// @param [in] prefix Prefix to add to the name
        /// @param [in] usePascalCase If set to true, the first element of the name will be capitalized as well
        /// @return The version of the name that uses camel case format
        String getCamelCaseName ( const String & prefix = String::EmptyString, bool usePascalCase = false ) const;

        /// @brief A function used for testing whether a specific value could be assigned to this element
        ///
        /// It is used in several places (like defined values, enum codes, default value option)
        /// to verify whether the type of this element can accept that value.
        /// It is not always perfect, but should detect a number of problems early (while parsing
        /// the protocol description file), before even generating the language output.
        ///
        /// If the value is incorrect, an error is thrown.
        ///
        /// @param [in] value The value that could potentially be assigned to this element
        /// @return The value that should be used. Potentially different than the value passed (for example
        ///   a conversion could be performed for boolean types).
        String checkAssValue ( String value );
};
}
