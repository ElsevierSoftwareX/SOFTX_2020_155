//
// Created by jonathan.hanks on 12/16/19.
//
#include "catch.hpp"
#include "recv_buffer.hh"

#include <algorithm>
#include <iterator>
#include <memory>
#include <iostream>
#include <vector>

std::shared_ptr< daq_multi_dcu_data_t >
create_test_blob( std::vector< unsigned int > dcus,
                  unsigned int                gps,
                  int                         cycle )
{
    std::shared_ptr< daq_multi_dcu_data_t > daq_data_p =
        std::make_shared< daq_multi_dcu_data_t >( );
    daq_multi_dcu_data_t& daq_data( *daq_data_p );

    daq_data.header.dcuTotalModels = 0;
    daq_data.header.fullDataBlockSize = 0;
    char* data = daq_data.dataBlock;

    for ( const auto& dcuid : dcus )
    {
        int dest_index = daq_data.header.dcuTotalModels;
        ++daq_data.header.dcuTotalModels;

        daq_data.header.fullDataBlockSize += 8;

        daq_msg_header_t& header = daq_data.header.dcuheader[ dest_index ];
        header.tpBlockSize = 0;
        header.dataBlockSize = 8;
        header.dcuId = dcuid;
        header.cycle = cycle;
        header.timeSec = gps;
        header.status = 0;
        header.timeNSec =
            cycle * static_cast< unsigned int >( 1000000000 / 16 );
        header.fileCrc = 42;
        header.dataCrc = 42;
        header.tpCount = 0;
        std::fill( std::begin( header.tpNum ), std::end( header.tpNum ), 0 );
        std::fill( data, data + 8, (char)dcuid );
        data += 8;
    }
    return daq_data_p;
}

TEST_CASE( "You can create a receive_buffer" )
{
    std::shared_ptr< receive_buffer< 4 > > buf =
        std::make_shared< receive_buffer< 4 > >( );
    std::cout << "receive_buffer<4> has a size of "
              << sizeof( receive_buffer< 4 > ) << std::endl;
}

TEST_CASE( "You can create a buffer entry" )
{
    buffer_entry entry;
}

TEST_CASE( "You can insert a test structure into a buffer entry" )
{
    buffer_entry entry;
    auto         daq_p = create_test_blob(
        std::vector< unsigned int >{ 5, 6, 7, 8 }, 1000000000, 5 );
    entry.ingest( *daq_p );

    gps_key target( 1000000000, 5 );
    REQUIRE( entry.latest == target );
    REQUIRE( entry.ifo_data.header.dcuTotalModels == 4 );
}

TEST_CASE(
    "You cannot insert a test structure into a buffer entry if it is too old" )
{
    buffer_entry entry;
    auto         daq_p = create_test_blob(
        std::vector< unsigned int >{ 5, 6, 7, 8 }, 1000000000, 5 );
    entry.ingest( *daq_p );

    gps_key target( 1000000000, 5 );
    REQUIRE( entry.latest == target );
    REQUIRE( entry.ifo_data.header.dcuTotalModels == 4 );

    daq_p = create_test_blob(
        std::vector< unsigned int >{ 9, 10, 11 }, 1000000000, 4 );
    entry.ingest( *daq_p );

    REQUIRE( entry.latest == target );
    REQUIRE( entry.ifo_data.header.dcuTotalModels == 4 );

    for ( int i = 0; i < 4; ++i )
    {
        REQUIRE( entry.ifo_data.header.dcuheader[ i ].dcuId == i + 5 );
    }
}

TEST_CASE( "Buffers clear old entries when a new timestamp is put in" )
{
    buffer_entry entry;
    auto         daq_p = create_test_blob(
        std::vector< unsigned int >{ 5, 6, 7, 8 }, 1000000000, 5 );
    entry.ingest( *daq_p );

    gps_key target( 1000000000, 5 );
    REQUIRE( entry.latest == target );
    REQUIRE( entry.ifo_data.header.dcuTotalModels == 4 );

    daq_p = create_test_blob(
        std::vector< unsigned int >{ 9, 10, 11 }, 1000000000, 6 );
    entry.ingest( *daq_p );

    gps_key new_target( 1000000000, 6 );

    REQUIRE( entry.latest == new_target );
    REQUIRE( entry.ifo_data.header.dcuTotalModels == 3 );

    for ( int i = 0; i < 3; ++i )
    {
        REQUIRE( entry.ifo_data.header.dcuheader[ i ].dcuId == i + 9 );
    }
}

TEST_CASE( "Buffers append in data when it is the same time" )
{
    buffer_entry entry;
    auto         daq_p = create_test_blob(
        std::vector< unsigned int >{ 5, 6, 7, 8 }, 1000000000, 5 );
    entry.ingest( *daq_p );

    gps_key target( 1000000000, 5 );
    REQUIRE( entry.latest == target );
    REQUIRE( entry.ifo_data.header.dcuTotalModels == 4 );

    daq_p = create_test_blob(
        std::vector< unsigned int >{ 9, 10, 11 }, 1000000000, 5 );
    entry.ingest( *daq_p );

    REQUIRE( entry.latest == target );
    REQUIRE( entry.ifo_data.header.dcuTotalModels == 7 );

    for ( int i = 0; i < 7; ++i )
    {
        REQUIRE( entry.ifo_data.header.dcuheader[ i ].dcuId == i + 5 );
    }
}

TEST_CASE( "You can process a given slice of a recieve buffer" )
{
    auto buffer = std::make_shared< receive_buffer< 2 > >( );
    auto daq_p = create_test_blob(
        std::vector< unsigned int >{ 5, 6, 7, 8 }, 1000000000, 5 );
    buffer->ingest( *daq_p );

    bool processed = false;
    bool correct_cycle = false;

    auto func = [&processed, &correct_cycle]( daq_dc_data_t& input ) {
        processed = true;
        if ( input.header.dcuheader[ 0 ].cycle == 5 &&
             input.header.dcuheader[ 0 ].timeSec == 1000000000 &&
             input.header.dcuTotalModels == 4 )
        {
            correct_cycle = true;
        }
    };
    buffer->process_slice_at( gps_key( 1000000000, 5 ), func );

    REQUIRE( processed );
    REQUIRE( correct_cycle );
}

TEST_CASE( "Processing a slice of a receive buffer that is not present will do "
           "nothing" )
{
    auto buffer = std::make_shared< receive_buffer< 2 > >( );
    auto daq_p = create_test_blob(
        std::vector< unsigned int >{ 5, 6, 7, 8 }, 1000000000, 5 );
    buffer->ingest( *daq_p );

    bool processed = false;

    auto func = [&processed]( daq_dc_data_t& input ) { processed = true; };
    buffer->process_slice_at( gps_key( 1000000000, 4 ), func );

    REQUIRE( !processed );
}