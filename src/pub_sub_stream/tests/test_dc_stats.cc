//
// Created by jonathan.hanks on 10/21/20.
//

#include "dc_stats.hh"

#include <iostream>
#include <thread>
#include <vector>

#include "daq_core.h"
#include "make_unique.hh"
#include "checksum_crc32.hh"

#include "catch.hpp"

namespace
{
    std::vector< DCUStats >
    basic_dcus( )
    {
        std::vector< DCUStats > dcus;
        dcus.emplace_back( DCUStats{
            0,
            0,
            0,
            0,
            "mod0",
        } );
        dcus.emplace_back( DCUStats{
            1,
            0,
            0,
            0,
            "mod1",
        } );
        dcus.emplace_back( DCUStats{
            2,
            0,
            0,
            0,
            "mod2",
        } );
        return dcus;
    }

    unsigned int
    get_data_offset( const daq_dc_data_t& data, unsigned int index )
    {
        unsigned int offset = 0;

        for ( unsigned int i = 0; i < index; ++i )
        {
            offset += data.header.dcuheader[ i ].dataBlockSize +
                data.header.dcuheader[ i ].tpBlockSize;
        }
        return offset;
    }

    void
    add_dcu_data( daq_dc_data_t& data,
                  unsigned int   dcuid,
                  unsigned int   gps,
                  unsigned int   cycle )
    {
        const unsigned int data_block_elements = 1024;
        const unsigned int data_block_size =
            data_block_elements * sizeof( unsigned int );

        auto index = data.header.dcuTotalModels;

        data.header.dcuTotalModels++;
        auto& cur_header = data.header.dcuheader[ index ];

        cur_header.dcuId = dcuid;
        cur_header.fileCrc = dcuid;
        cur_header.status = 2;
        cur_header.cycle = cycle;
        cur_header.timeSec = gps;
        cur_header.timeNSec = cycle * ( 1000000000 / 16 );
        cur_header.dataBlockSize = data_block_size;
        cur_header.tpBlockSize = 0;
        cur_header.tpCount = 0;

        auto offset = get_data_offset( data, index );
        auto start = reinterpret_cast< unsigned int* >(
            &( data.dataBlock[ 0 ] ) + offset );
        std::fill( start,
                   start + data_block_elements,
                   ( gps << 12 ) + ( cycle << 8 ) + ( dcuid & 0x0ff ) );

        checksum_crc32 crc;
        crc.add( reinterpret_cast< void* >( start ), data_block_size );
        cur_header.dataCrc = crc.result( );

        data.header.fullDataBlockSize += data_block_size;
    }
} // namespace

TEST_CASE( "You can create a DCStats object" )
{
    DCStats dc_stats( basic_dcus( ) );
}

TEST_CASE(
    "CRCs should only show up after you have seen a dcu and on a 'edge'" )
{
    DCStats dc_stats( basic_dcus( ) );

    // used to debug the threads involved
    volatile bool stopped{ false };

    auto queue = dc_stats.get_queue( );
    auto pause = [queue, &stopped]( ) {
        for ( int tries = 0; stopped || !queue->empty( ); ++tries )
        {
            if ( tries > 20 && !stopped )
            {
                throw std::runtime_error( "Took too long to process data" );
            }
            std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 15 ) );
    };
    REQUIRE( queue != nullptr );

    std::thread th(
        [&dc_stats]( ) { dc_stats.run( (simple_pv_handle)nullptr ); } );

    {
        auto data = make_unique_ptr< daq_dc_data_t >( );
        memset(
            (void*)data.get( ), 0, sizeof( decltype( data )::element_type ) );
        add_dcu_data( *data, 2, 1000000000, 0 );
        add_dcu_data( *data, 1, 1000000000, 0 );
        queue->emplace( std::move( data ) );
    }
    pause( );
    REQUIRE( dc_stats.peek_stats( 1 ).processed );
    REQUIRE( dc_stats.peek_stats( 1 ).crc_sum == 0 );
    REQUIRE( dc_stats.peek_stats( 1 ).status == 0 );
    REQUIRE( dc_stats.peek_stats( 2 ).processed );
    REQUIRE( dc_stats.peek_stats( 2 ).crc_sum == 0 );
    REQUIRE( dc_stats.peek_stats( 2 ).status == 0 );
    {
        auto data = make_unique_ptr< daq_dc_data_t >( );
        memset(
            (void*)data.get( ), 0, sizeof( decltype( data )::element_type ) );
        add_dcu_data( *data, 2, 1000000000, 1 );

        queue->emplace( std::move( data ) );
    }
    pause( );
    REQUIRE( dc_stats.peek_stats( 1 ).processed );
    REQUIRE( !dc_stats.peek_stats( 1 ).processed_this_cycle );
    REQUIRE( dc_stats.peek_stats( 1 ).crc_sum == 1 );
    REQUIRE( dc_stats.peek_stats( 1 ).status == 0xbad );
    REQUIRE( dc_stats.peek_stats( 2 ).processed );
    REQUIRE( dc_stats.peek_stats( 2 ).crc_sum == 0 );
    REQUIRE( dc_stats.peek_stats( 2 ).status == 0 );
    {
        auto data = make_unique_ptr< daq_dc_data_t >( );
        memset(
            (void*)data.get( ), 0, sizeof( decltype( data )::element_type ) );
        add_dcu_data( *data, 2, 1000000000, 2 );

        queue->emplace( std::move( data ) );
    }
    pause( );
    REQUIRE( dc_stats.peek_stats( 1 ).processed );
    REQUIRE( !dc_stats.peek_stats( 1 ).processed_this_cycle );
    REQUIRE( dc_stats.peek_stats( 1 ).crc_sum == 1 );
    REQUIRE( dc_stats.peek_stats( 1 ).status == 0xbad );
    REQUIRE( dc_stats.peek_stats( 2 ).processed );
    REQUIRE( dc_stats.peek_stats( 2 ).crc_sum == 0 );
    REQUIRE( dc_stats.peek_stats( 2 ).status == 0 );
    dc_stats.stop( );

    th.join( );
}

TEST_CASE( "CRCs are cleared at the second boundary" )
{
    DCStats dc_stats( basic_dcus( ) );

    // used to debug the threads involved
    volatile bool stopped{ false };

    auto queue = dc_stats.get_queue( );
    auto pause = [queue, &stopped]( ) {
        for ( int tries = 0; stopped || !queue->empty( ); ++tries )
        {
            if ( tries > 20 && !stopped )
            {
                throw std::runtime_error( "Took too long to process data" );
            }
            std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 15 ) );
    };
    REQUIRE( queue != nullptr );

    std::thread th(
        [&dc_stats]( ) { dc_stats.run( (simple_pv_handle)nullptr ); } );

    for ( auto i = 1; i < 16; ++i )
    {
        if ( i == 10 )
        {
            dc_stats.request_clear_crc( );
        }
        auto data = make_unique_ptr< daq_dc_data_t >( );
        memset(
            (void*)data.get( ), 0, sizeof( decltype( data )::element_type ) );
        add_dcu_data( *data, 2, 1000000000, 0 );
        if ( i % 2 == 0 )
        {
            add_dcu_data( *data, 1, 1000000000, 0 );
        }
        queue->emplace( std::move( data ) );
        pause( );
    }
    REQUIRE( dc_stats.peek_stats( 1 ).crc_sum == 7 );
    {
        auto data = make_unique_ptr< daq_dc_data_t >( );
        memset(
            (void*)data.get( ), 0, sizeof( decltype( data )::element_type ) );
        add_dcu_data( *data, 2, 1000000000, 0 );
        add_dcu_data( *data, 1, 1000000000, 0 );
        queue->emplace( std::move( data ) );
        pause( );
    }
    REQUIRE( dc_stats.peek_stats( 1 ).crc_sum == 0 );

    dc_stats.stop( );
    th.join( );
}