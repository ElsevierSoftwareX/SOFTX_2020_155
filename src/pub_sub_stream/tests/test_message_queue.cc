//
// Created by jonathan.hanks on 10/7/20.
//
#include "message_queue.hh"
#include "catch.hpp"
#include <iostream>
#include <thread>

TEST_CASE( "You can create a message queue" )
{
    Message_queue< int, 5 > test_queue;
    REQUIRE( test_queue.capacity( ) == 5 );
    REQUIRE( test_queue.size( ) == 0 );
    REQUIRE( test_queue.empty( ) == true );

    auto internal{ detail::introspect( test_queue ) };
    REQUIRE( internal.begin_index( ) == internal.end_index( ) );
    REQUIRE( internal.begin_index( ) == test_queue.capacity( ) - 1 );
}

TEST_CASE( "You can add and remove messages to the queue" )
{
    Message_queue< int, 5 > test_queue;
    test_queue.emplace( 5 );
    REQUIRE( test_queue.size( ) == 1 );
    REQUIRE( test_queue.capacity( ) == 5 );
    REQUIRE( test_queue.empty( ) == false );

    auto internal{ detail::introspect( test_queue ) };
    REQUIRE( internal.begin_index( ) == 3 );
    REQUIRE( internal.end_index( ) == 4 );

    test_queue.emplace( 42 );
    REQUIRE( internal.begin_index( ) == 2 );
    REQUIRE( internal.end_index( ) == 4 );
    REQUIRE( test_queue.size( ) == 2 );

    int val = test_queue.pop( );
    REQUIRE( val == 5 );
    REQUIRE( test_queue.capacity( ) == 5 );
    REQUIRE( test_queue.size( ) == 1 );
    REQUIRE( test_queue.empty( ) == false );
    REQUIRE( internal.begin_index( ) == 2 );
    REQUIRE( internal.end_index( ) == 3 );

    val = test_queue.pop( );
    REQUIRE( val == 42 );
    REQUIRE( test_queue.capacity( ) == 5 );
    REQUIRE( test_queue.size( ) == 0 );
    REQUIRE( test_queue.empty( ) == true );
    REQUIRE( internal.begin_index( ) == 2 );
    REQUIRE( internal.end_index( ) == 2 );
}

TEST_CASE( "The message queue is a ring buffer wrapping around works" )
{
    Message_queue< int, 5 > test_queue;
    auto                    internal{ detail::introspect( test_queue ) };

    test_queue.emplace( 0 );
    REQUIRE( internal.begin_index( ) == 3 );
    REQUIRE( internal.end_index( ) == 4 );
    test_queue.emplace( 1 );
    REQUIRE( internal.begin_index( ) == 2 );
    REQUIRE( internal.end_index( ) == 4 );
    test_queue.emplace( 2 );
    REQUIRE( internal.begin_index( ) == 1 );
    REQUIRE( internal.end_index( ) == 4 );
    test_queue.emplace( 3 );
    REQUIRE( internal.begin_index( ) == 0 );
    REQUIRE( internal.end_index( ) == 4 );
    test_queue.emplace( 4 );
    REQUIRE( internal.begin_index( ) == 4 );
    REQUIRE( internal.end_index( ) == 4 );

    REQUIRE( test_queue.pop( ) == 0 );
    REQUIRE( internal.begin_index( ) == 4 );
    REQUIRE( internal.end_index( ) == 3 );

    REQUIRE( test_queue.pop( ) == 1 );
    REQUIRE( internal.begin_index( ) == 4 );
    REQUIRE( internal.end_index( ) == 2 );

    test_queue.emplace( 5 );
    REQUIRE( internal.begin_index( ) == 3 );
    REQUIRE( internal.end_index( ) == 2 );

    REQUIRE( test_queue.pop( ) == 2 );
    REQUIRE( internal.begin_index( ) == 3 );
    REQUIRE( internal.end_index( ) == 1 );

    REQUIRE( test_queue.pop( ) == 3 );
    REQUIRE( internal.begin_index( ) == 3 );
    REQUIRE( internal.end_index( ) == 0 );

    test_queue.emplace( 6 );
    REQUIRE( internal.begin_index( ) == 2 );
    REQUIRE( internal.end_index( ) == 0 );

    REQUIRE( test_queue.pop( ) == 4 );
    REQUIRE( internal.begin_index( ) == 2 );
    REQUIRE( internal.end_index( ) == 4 );

    REQUIRE( test_queue.pop( ) == 5 );
    REQUIRE( internal.begin_index( ) == 2 );
    REQUIRE( internal.end_index( ) == 3 );

    REQUIRE( test_queue.pop( ) == 6 );
    REQUIRE( internal.begin_index( ) == 2 );
    REQUIRE( internal.end_index( ) == 2 );
    REQUIRE( test_queue.empty( ) );
}

TEST_CASE( "You can optionally time out a pop operation on a message_queue " )
{
    Message_queue< int, 5 > test_queue;
    boost::optional< int >  val =
        test_queue.pop( std::chrono::milliseconds( 20 ) );
    REQUIRE( val.operator bool( ) == false );
}

TEST_CASE( "An empty message queue blocks until data is present, if timeouts are not requested")
{
    Message_queue<int, 5> test_queue;

    std::thread t([&test_queue]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        test_queue.emplace(42);
    });
    auto start = std::chrono::steady_clock::now();
    int val = test_queue.pop();
    auto end = std::chrono::steady_clock::now();
    REQUIRE(val == 42);
    auto duration = end-start;
    REQUIRE(duration > std::chrono::milliseconds(200));
    t.join();
}

TEST_CASE(
    "You can push as much into a message queue as long as you take out things" )
{
    const int               TEST_LEN = 1000;
    Message_queue< int, 5 > test_queue;
    bool                    failed = false;

    std::thread pop_thread( [&test_queue, &failed, TEST_LEN]( ) {
        for ( int i = 0; i < TEST_LEN; ++i )
        {
            if ( test_queue.pop( ) != i )
            {
                failed = true;
            }
        }
    } );
    for ( int i = 0; i < TEST_LEN; ++i )
    {
        test_queue.emplace( i );
    }
    pop_thread.join( );
    REQUIRE( !failed );
}

TEST_CASE( "If a message queue is destroyed it calls the destructor on all "
           "items in the queue" )
{
    int destructor_count = 0;

    struct Data_obj
    {
        explicit Data_obj( int* p ) : p_{ p }
        {
        }
        ~Data_obj( )
        {
            ++( *p_ );
        }
        Data_obj( Data_obj&& ) noexcept = default;
        Data_obj& operator=( Data_obj&& ) noexcept = default;

        int* p_{ nullptr };
    };
    {
        Message_queue< Data_obj, 5 > test_queue;
        test_queue.emplace( &destructor_count );
        test_queue.emplace( &destructor_count );
        test_queue.emplace( &destructor_count );
        test_queue.emplace( &destructor_count );
        test_queue.emplace( &destructor_count );
    }
    REQUIRE( destructor_count == 5 );
}