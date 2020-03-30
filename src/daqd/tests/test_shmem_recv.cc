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
        reinterpret_cast< volatile daq_multi_cycle_data_t* >( shmem.get( ) ) );
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
        reinterpret_cast< volatile daq_multi_cycle_data_t* >( shmem.get( ) ) );
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
        reinterpret_cast< volatile daq_multi_cycle_data_t* >( shmem.get( ) ) );
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
        reinterpret_cast< volatile daq_multi_cycle_data_t* >( shmem.get( ) ) );
    recv.reset_previous_cycle( 0, 1000000000 - 1 );
    auto data = recv.receive_data( );
    REQUIRE( data->dataBlock[ 0 ] == 5 );
}