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

#include <gtest/gtest.h>

#include "basic/String.hpp"
#include "basic/DoublyLinkedList.hpp"

using namespace Pravala;

class TestEntry: public DoublyLinkedList<TestEntry>::Entry
{
    public:
        const int Value;

        TestEntry ( size_t & counter, int v ): Value ( v ), _counter ( counter )
        {
            ++_counter;
        }

        ~TestEntry()
        {
            --_counter;
        }

        using DoublyLinkedList<TestEntry>::Entry::listGetList;
        using DoublyLinkedList<TestEntry>::Entry::listGetNext;
        using DoublyLinkedList<TestEntry>::Entry::listInsertAfter;
        using DoublyLinkedList<TestEntry>::Entry::listInsertBefore;
        using DoublyLinkedList<TestEntry>::Entry::listMoveToFront;
        using DoublyLinkedList<TestEntry>::Entry::listMoveToBack;

    private:
        size_t & _counter;
};

/// @brief List class test
class DoublyLinkedListTest: public ::testing::Test
{
    protected:
        String _ret;

        const char * desc ( DoublyLinkedList<TestEntry> & list )
        {
            _ret.clear();

            const TestEntry * ptr = list.getHead();

            while ( ptr != 0 )
            {
                _ret.append ( String ( _ret.isEmpty() ? "%1" : ",%1" ).arg ( ptr->Value ) );

                ptr = ptr->listGetNext();
            }

            return _ret.c_str();
        }
};

TEST_F ( DoublyLinkedListTest, Basic )
{
    DoublyLinkedList<TestEntry> list;

    ASSERT_TRUE ( list.isEmpty() );
    ASSERT_EQ ( 0, list.size() );
    ASSERT_EQ ( ( TestEntry * ) 0, list.getHead() );

    EXPECT_STREQ ( "", desc ( list ) );

    size_t c = 0;

    list.append ( new TestEntry ( c, 1 ) );

    EXPECT_STREQ ( "1", desc ( list ) );

    ASSERT_EQ ( 1, c );
    ASSERT_EQ ( 1, list.size() );
    ASSERT_FALSE ( list.isEmpty() );
    ASSERT_NE ( ( TestEntry * ) 0, list.getHead() );
    EXPECT_EQ ( 1, list.getHead()->Value );

    list.append ( new TestEntry ( c, 2 ) );

    EXPECT_STREQ ( "1,2", desc ( list ) );

    ASSERT_EQ ( 2, c );
    ASSERT_EQ ( 2, list.size() );
    ASSERT_NE ( ( TestEntry * ) 0, list.getHead() );
    EXPECT_EQ ( 1, list.getHead()->Value );

    list.append ( new TestEntry ( c, 3 ) );

    EXPECT_STREQ ( "1,2,3", desc ( list ) );

    ASSERT_EQ ( 3, c );
    ASSERT_EQ ( 3, list.size() );
    ASSERT_NE ( ( TestEntry * ) 0, list.getHead() );
    EXPECT_EQ ( 1, list.getHead()->Value );

    list.append ( new TestEntry ( c, 4 ) );

    EXPECT_STREQ ( "1,2,3,4", desc ( list ) );

    ASSERT_EQ ( 4, c );
    ASSERT_EQ ( 4, list.size() );
    ASSERT_NE ( ( TestEntry * ) 0, list.getHead() );
    EXPECT_EQ ( 1, list.getHead()->Value );

    delete list.getHead();

    EXPECT_STREQ ( "2,3,4", desc ( list ) );

    ASSERT_EQ ( 3, c );
    ASSERT_EQ ( 3, list.size() );
    ASSERT_NE ( ( TestEntry * ) 0, list.getHead() );
    EXPECT_EQ ( 2, list.getHead()->Value );

    delete list.getHead();

    EXPECT_STREQ ( "3,4", desc ( list ) );

    ASSERT_EQ ( 2, c );
    ASSERT_EQ ( 2, list.size() );
    ASSERT_NE ( ( TestEntry * ) 0, list.getHead() );
    EXPECT_EQ ( 3, list.getHead()->Value );

    delete list.getHead();

    EXPECT_STREQ ( "4", desc ( list ) );

    ASSERT_EQ ( 1, c );
    ASSERT_EQ ( 1, list.size() );
    ASSERT_FALSE ( list.isEmpty() );
    ASSERT_NE ( ( TestEntry * ) 0, list.getHead() );
    EXPECT_EQ ( 4, list.getHead()->Value );

    delete list.getHead();

    EXPECT_STREQ ( "", desc ( list ) );

    ASSERT_EQ ( 0, c );
    ASSERT_EQ ( 0, list.size() );
    ASSERT_TRUE ( list.isEmpty() );
    ASSERT_EQ ( ( TestEntry * ) 0, list.getHead() );
}

TEST_F ( DoublyLinkedListTest, AppendPrepend )
{
    DoublyLinkedList<TestEntry> list;

    ASSERT_TRUE ( list.isEmpty() );
    EXPECT_STREQ ( "", desc ( list ) );

    size_t c = 0;

    list.append ( new TestEntry ( c, 1 ) );   // 1
    list.prepend ( new TestEntry ( c, 2 ) );  // 2,1
    list.append ( new TestEntry ( c, 3 ) );   // 2,1,3
    list.prepend ( new TestEntry ( c, 4 ) );  // 4,2,1,3
    list.append ( new TestEntry ( c, 5 ) );   // 4,2,1,3,5
    list.prepend ( new TestEntry ( c, 6 ) );  // 6,4,2,1,3,5
    list.append ( new TestEntry ( c, 7 ) );   // 6,4,2,1,3,5,7
    list.append ( new TestEntry ( c, 8 ) );   // 6,4,2,1,3,5,7,8
    list.prepend ( new TestEntry ( c, 9 ) );  // 9,6,4,2,1,3,5,7,8
    list.prepend ( new TestEntry ( c, 10 ) ); // 10,9,6,4,2,1,3,5,7,8
    list.append ( new TestEntry ( c, 11 ) );  // 10,9,6,4,2,1,3,5,7,8,11
    list.prepend ( new TestEntry ( c, 12 ) ); // 12,10,9,6,4,2,1,3,5,7,8,11

    EXPECT_EQ ( 12, c );
    EXPECT_EQ ( 12, list.size() );
    EXPECT_STREQ ( "12,10,9,6,4,2,1,3,5,7,8,11", desc ( list ) );

    list.deleteContent();

    EXPECT_EQ ( 0, c );
    EXPECT_EQ ( 0, list.size() );
}

TEST_F ( DoublyLinkedListTest, RandomInsert )
{
    DoublyLinkedList<TestEntry> list;

    ASSERT_TRUE ( list.isEmpty() );
    EXPECT_STREQ ( "", desc ( list ) );

    size_t c = 0;

    TestEntry * e1 = new TestEntry ( c, 1 );
    TestEntry * e2 = new TestEntry ( c, 2 );
    TestEntry * e3 = new TestEntry ( c, 3 );
    TestEntry * e4 = new TestEntry ( c, 4 );
    TestEntry * e5 = new TestEntry ( c, 5 );
    TestEntry * e6 = new TestEntry ( c, 6 );
    TestEntry * e7 = new TestEntry ( c, 7 );
    TestEntry * e8 = new TestEntry ( c, 8 );
    TestEntry * e9 = new TestEntry ( c, 9 );

    ASSERT_TRUE ( !e1->listGetList() );

    EXPECT_EQ ( 0, list.size() );

    list.append ( e1 ); // 1

    ASSERT_EQ ( &list, e1->listGetList() );

    list.append ( e2 ); // 1,2
    list.append ( e3 ); // 1,2,3

    EXPECT_EQ ( 3, list.size() );
    EXPECT_STREQ ( "1,2,3", desc ( list ) );

    e3->listInsertAfter ( e4 ); // 1,2,3,4

    EXPECT_EQ ( 4, list.size() );
    EXPECT_STREQ ( "1,2,3,4", desc ( list ) );

    e1->listInsertAfter ( e5 ); // 1,5,2,3,4

    EXPECT_EQ ( 5, list.size() );
    EXPECT_STREQ ( "1,5,2,3,4", desc ( list ) );

    e1->listInsertBefore ( e6 ); // 6,1,5,2,3,4

    EXPECT_EQ ( 6, list.size() );
    EXPECT_STREQ ( "6,1,5,2,3,4", desc ( list ) );

    e1->listInsertBefore ( e7 ); // 6,7,1,5,2,3,4

    EXPECT_EQ ( 7, list.size() );
    EXPECT_STREQ ( "6,7,1,5,2,3,4", desc ( list ) );

    e2->listInsertAfter ( e8 ); // 6,7,1,5,2,8,3,4

    EXPECT_EQ ( 8, list.size() );
    EXPECT_STREQ ( "6,7,1,5,2,8,3,4", desc ( list ) );

    e2->listInsertBefore ( e9 ); // 6,7,1,5,9,2,8,3,4

    EXPECT_EQ ( 9, list.size() );
    EXPECT_STREQ ( "6,7,1,5,9,2,8,3,4", desc ( list ) );

    // Re-inserting:
    e2->listInsertAfter ( e7 ); // 6,1,5,9,2,7,8,3,4

    EXPECT_EQ ( 9, list.size() );
    EXPECT_STREQ ( "6,1,5,9,2,7,8,3,4", desc ( list ) );

    e6->listInsertAfter ( e1 ); // 6,1,5,9,2,7,8,3,4 (no change)

    EXPECT_EQ ( 9, list.size() );
    EXPECT_STREQ ( "6,1,5,9,2,7,8,3,4", desc ( list ) );

    e6->listInsertBefore ( e1 ); // 1,6,5,9,2,7,8,3,4

    EXPECT_EQ ( 9, list.size() );
    EXPECT_STREQ ( "1,6,5,9,2,7,8,3,4", desc ( list ) );

    e1->listInsertBefore ( e4 ); // 4,1,6,5,9,2,7,8,3

    EXPECT_EQ ( 9, list.size() );
    EXPECT_STREQ ( "4,1,6,5,9,2,7,8,3", desc ( list ) );

    e4->listInsertBefore ( e3 ); // 3,4,1,6,5,9,2,7,8

    EXPECT_EQ ( 9, list.size() );
    EXPECT_STREQ ( "3,4,1,6,5,9,2,7,8", desc ( list ) );

    delete e5;

    EXPECT_EQ ( 8, list.size() );
    EXPECT_STREQ ( "3,4,1,6,9,2,7,8", desc ( list ) );

    delete e3;

    EXPECT_EQ ( 7, list.size() );
    EXPECT_STREQ ( "4,1,6,9,2,7,8", desc ( list ) );

    list.deleteContent();

    EXPECT_EQ ( 0, c );
    EXPECT_EQ ( 0, list.size() );
}

TEST_F ( DoublyLinkedListTest, Move )
{
    DoublyLinkedList<TestEntry> list;

    ASSERT_TRUE ( list.isEmpty() );
    EXPECT_STREQ ( "", desc ( list ) );

    size_t c = 0;

    TestEntry * e1 = new TestEntry ( c, 1 );
    TestEntry * e2 = new TestEntry ( c, 2 );
    TestEntry * e3 = new TestEntry ( c, 3 );
    TestEntry * e4 = new TestEntry ( c, 4 );
    TestEntry * e5 = new TestEntry ( c, 5 );
    TestEntry * e6 = new TestEntry ( c, 6 );
    TestEntry * e7 = new TestEntry ( c, 7 );
    TestEntry * e8 = new TestEntry ( c, 8 );
    TestEntry * e9 = new TestEntry ( c, 9 );

    EXPECT_EQ ( 9, c );
    EXPECT_EQ ( 0, list.size() );

    list.append ( e1 ); // 1

    EXPECT_EQ ( 1, list.size() );
    EXPECT_STREQ ( "1", desc ( list ) );

    e1->listMoveToBack();

    EXPECT_STREQ ( "1", desc ( list ) );

    e1->listMoveToFront();

    EXPECT_STREQ ( "1", desc ( list ) );

    list.append ( e2 ); // 1,2

    EXPECT_EQ ( 2, list.size() );
    EXPECT_STREQ ( "1,2", desc ( list ) );

    e1->listMoveToBack();

    EXPECT_STREQ ( "2,1", desc ( list ) );

    e1->listMoveToBack();

    EXPECT_STREQ ( "2,1", desc ( list ) );

    e1->listMoveToFront();

    EXPECT_STREQ ( "1,2", desc ( list ) );

    e1->listMoveToFront();

    EXPECT_STREQ ( "1,2", desc ( list ) );

    list.append ( e3 ); // 1,2,3
    list.append ( e4 ); // 1,2,3,4
    list.append ( e5 ); // 1,2,3,4,5
    list.append ( e6 ); // 1,2,3,4,5,6
    list.append ( e7 ); // 1,2,3,4,5,6,7
    list.append ( e8 ); // 1,2,3,4,5,6,7,8
    list.append ( e9 ); // 1,2,3,4,5,6,7,8,9

    EXPECT_EQ ( 9, list.size() );
    EXPECT_STREQ ( "1,2,3,4,5,6,7,8,9", desc ( list ) );

    e1->listMoveToBack();

    EXPECT_STREQ ( "2,3,4,5,6,7,8,9,1", desc ( list ) );

    e2->listMoveToBack();

    EXPECT_STREQ ( "3,4,5,6,7,8,9,1,2", desc ( list ) );

    e1->listMoveToFront();

    EXPECT_STREQ ( "1,3,4,5,6,7,8,9,2", desc ( list ) );

    e2->listMoveToFront();

    EXPECT_STREQ ( "2,1,3,4,5,6,7,8,9", desc ( list ) );

    e1->listMoveToFront();

    EXPECT_STREQ ( "1,2,3,4,5,6,7,8,9", desc ( list ) );

    list.deleteContent();

    EXPECT_EQ ( 0, c );
    EXPECT_EQ ( 0, list.size() );
}
