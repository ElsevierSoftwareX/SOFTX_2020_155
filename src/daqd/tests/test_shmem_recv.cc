//
// Created by jonathan.hanks on 3/19/20.
//
#include "shmem_receiver.hh"

#include <algorithm>
#include "raii.hh"
#include "catch.hpp"

TEST_CASE(
    "ShMemReceiver should get the next available cycle when it starts up" )
{
    auto shmem = raii::make_unique_ptr< daq_multi_cycle_data_t >( );

    shmem->header.maxCycle = 16;
    shmem->header.curCycle = 5;
    shmem->header.cycleDataSize = sizeof( shmem->dataBlock ) / 16;
    auto cycle_data_size = shmem->header.cycleDataSize;

    for ( int i = 0; i < 16; ++i )
    {
        auto  start = ( &shmem->dataBlock[ 0 ] ) + i * cycle_data_size;
        auto* cur_segment = reinterpret_cast< daq_multi_dcu_data_t* >( start );
        cur_segment->header.dcuTotalModels = 1;
        cur_segment->header.dcuheader[ 0 ].cycle = i;
        cur_segment->header.dcuheader[ 0 ].timeSec = 1000000000;
        cur_segment->header.fullDataBlockSize = 64;

        auto start_data =
            reinterpret_cast< char* >( &cur_segment->dataBlock[ 0 ] );
        std::fill( start_data, start_data + 64, i );
    }

    ShMemReceiver recv(
        reinterpret_cast< volatile daq_multi_cycle_data_t* >( shmem.get( ) ), sizeof( daq_multi_cycle_data_t) );
    recv.reset_previous_cycle( 4, 1000000000 );
    auto data = recv.receive_data( );
    REQUIRE( data->dataBlock[ 0 ] == 5 );
}

TEST_CASE( "ShMemReceiver should try to catch up if it sees a jump" )
{
    auto shmem = raii::make_unique_ptr< daq_multi_cycle_data_t >( );

    shmem->header.maxCycle = 16;
    shmem->header.curCycle = 5;
    shmem->header.cycleDataSize = sizeof( shmem->dataBlock ) / 16;
    auto cycle_data_size = shmem->header.cycleDataSize;

    for ( int i = 0; i < 16; ++i )
    {
        auto  start = ( &shmem->dataBlock[ 0 ] ) + i * cycle_data_size;
        auto* cur_segment = reinterpret_cast< daq_multi_dcu_data_t* >( start );
        cur_segment->header.dcuTotalModels = 1;
        cur_segment->header.dcuheader[ 0 ].cycle = i;
        cur_segment->header.dcuheader[ 0 ].timeSec = 1000000000;
        cur_segment->header.fullDataBlockSize = 64;

        auto start_data =
            reinterpret_cast< char* >( &cur_segment->dataBlock[ 0 ] );
        std::fill( start_data, start_data + 64, i );
    }

    ShMemReceiver recv(
        reinterpret_cast< volatile daq_multi_cycle_data_t* >( shmem.get( ) ), sizeof( daq_multi_cycle_data_t ) );
    recv.reset_previous_cycle( 0, 1000000000 );
    auto data = recv.receive_data( );
    REQUIRE( data->dataBlock[ 0 ] == 1 );
}

TEST_CASE( "ShMemReceiver should try to catch up if it sees a jump, even when "
           "the cycle counter wraps around" )
{
    auto shmem = raii::make_unique_ptr< daq_multi_cycle_data_t >( );

    shmem->header.maxCycle = 16;
    shmem->header.curCycle = 5;
    shmem->header.cycleDataSize = sizeof( shmem->dataBlock ) / 16;
    auto cycle_data_size = shmem->header.cycleDataSize;

    for ( int i = 0; i < 16; ++i )
    {
        auto  start = ( &shmem->dataBlock[ 0 ] ) + i * cycle_data_size;
        auto* cur_segment = reinterpret_cast< daq_multi_dcu_data_t* >( start );
        cur_segment->header.dcuTotalModels = 1;
        cur_segment->header.dcuheader[ 0 ].cycle = i;
        cur_segment->header.dcuheader[ 0 ].timeSec =
            1000000000 + ( i <= 5 ? 1 : 0 );
        cur_segment->header.fullDataBlockSize = 64;

        auto start_data =
            reinterpret_cast< char* >( &cur_segment->dataBlock[ 0 ] );
        std::fill( start_data, start_data + 64, i );
    }

    ShMemReceiver recv(
        reinterpret_cast< volatile daq_multi_cycle_data_t* >( shmem.get( ) ),
        sizeof( daq_multi_cycle_data_t ) );
    recv.reset_previous_cycle( 10, 1000000000 );
    auto data = recv.receive_data( );
    REQUIRE( data->dataBlock[ 0 ] == 11 );
}

TEST_CASE( "ShMemReceiver should try to catch up but will jump if needed" )
{
    auto shmem = raii::make_unique_ptr< daq_multi_cycle_data_t >( );

    shmem->header.maxCycle = 16;
    shmem->header.curCycle = 5;
    shmem->header.cycleDataSize = sizeof( shmem->dataBlock ) / 16;
    auto cycle_data_size = shmem->header.cycleDataSize;

    for ( int i = 0; i < 16; ++i )
    {
        auto  start = ( &shmem->dataBlock[ 0 ] ) + i * cycle_data_size;
        auto* cur_segment = reinterpret_cast< daq_multi_dcu_data_t* >( start );
        cur_segment->header.dcuTotalModels = 1;
        cur_segment->header.dcuheader[ 0 ].cycle = i;
        cur_segment->header.dcuheader[ 0 ].timeSec = 1000000000;
        cur_segment->header.fullDataBlockSize = 64;

        auto start_data =
            reinterpret_cast< char* >( &cur_segment->dataBlock[ 0 ] );
        std::fill( start_data, start_data + 64, i );
    }

    ShMemReceiver recv(
        reinterpret_cast< volatile daq_multi_cycle_data_t* >( shmem.get( ) ),
        sizeof( daq_multi_cycle_data_t ) );
    recv.reset_previous_cycle( 0, 1000000000 - 1 );
    auto data = recv.receive_data( );
    REQUIRE( data->dataBlock[ 0 ] == 5 );
}

TEST_CASE( "ShMemReceiver will throw an exception if max cycles is too big")
{
    auto shmem = raii::make_unique_ptr< daq_multi_cycle_data_t >( );
    shmem->header.maxCycle = 65;
    shmem->header.curCycle = 5;
    shmem->header.cycleDataSize = sizeof( shmem->dataBlock ) / 16;
    ShMemReceiver recv(
            reinterpret_cast< volatile daq_multi_cycle_data_t* >( shmem.get( ) ),
            sizeof( daq_multi_cycle_data_t ) );
    recv.reset_previous_cycle( 0, 1000000000 - 1 );
    REQUIRE_THROWS_AS(recv.receive_data(), std::runtime_error);
}

TEST_CASE( "ShMemReceiver will throw an exception if curCycle is too big")
{
    auto shmem = raii::make_unique_ptr< daq_multi_cycle_data_t >( );
    shmem->header.maxCycle = 16;
    shmem->header.curCycle = 15;
    shmem->header.cycleDataSize = sizeof( shmem->dataBlock ) / 16;
    ShMemReceiver recv(
            reinterpret_cast< volatile daq_multi_cycle_data_t* >( shmem.get( ) ),
            sizeof( daq_multi_cycle_data_t ) );
    recv.reset_previous_cycle( 15, 1000000000 - 1 );
    shmem->header.curCycle = 16;
    REQUIRE_THROWS_AS(recv.receive_data(), std::runtime_error);
}

TEST_CASE( "ShMemReceiver will throw an exception if cycleDataSize*maxCycle is too big")
{
    auto shmem = raii::make_unique_ptr< daq_multi_cycle_data_t >( );
    shmem->header.maxCycle = 16;
    shmem->header.curCycle = 15;
    shmem->header.cycleDataSize = sizeof( shmem->dataBlock ) / 16;
    ShMemReceiver recv(
            reinterpret_cast< volatile daq_multi_cycle_data_t* >( shmem.get( ) ),
            sizeof( daq_multi_cycle_data_t ) );
    recv.reset_previous_cycle( 15, 1000000000 - 1 );
    shmem->header.curCycle = 0;
    // this does not take away the header size, so it should lead to an overflow condition
    shmem->header.cycleDataSize = sizeof(daq_multi_cycle_data_t)/16;
    REQUIRE_THROWS_AS(recv.receive_data(), std::runtime_error);
}