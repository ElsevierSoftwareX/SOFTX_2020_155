//
// Created by jonathan.hanks on 5/27/20.
//

#include "recv_buffer.hh"

#include <algorithm>
#include <iostream>
#include <memory>

std::shared_ptr< daq_multi_dcu_data_t >
dummy_data( unsigned int dcu_id, unsigned int gps, int cycle )
{
    auto data = std::make_shared< daq_multi_dcu_data_t >( );
    data->header.dcuTotalModels = 1;
    daq_msg_header_t& header = data->header.dcuheader[ 0 ];
    header.dcuId = dcu_id;
    header.dataBlockSize = 1000;
    header.tpBlockSize = 0;
    header.timeSec = gps;
    header.timeNSec = ( 1000000000 / 16 ) * cycle;
    header.cycle = cycle;
    header.tpCount = 0;
    std::fill( std::begin( header.tpNum ), std::end( header.tpNum ), 0 );
    header.status = 0;
    auto start = &( data->dataBlock[ 0 ] );
    std::fill( start, start + 1000, 0xfe );
    data->header.fullDataBlockSize = 1000;
    return data;
}

void
test_buffer_entry_late_entry( )
{
    const int gps_time = 1000000000;

    std::atomic< int64_t > late_entries{ 0 };
    std::atomic< int64_t > discarded_entries{ 0 };
    std::atomic< int64_t > total_span{ 0 };
    auto                   buffer = std::make_shared< buffer_entry >(
        &late_entries, &discarded_entries, &total_span );

    auto data = dummy_data( 5, gps_time, 0 );
    buffer->ingest( *data );

    if ( late_entries != 0 || discarded_entries != 0 )
    {
        throw std::runtime_error( "unexpected late or discarded entry" );
    }

    data = dummy_data( 6, gps_time, 0 );
    buffer->ingest( *data );

    if ( late_entries != 0 || discarded_entries != 0 )
    {
        throw std::runtime_error( "unexpected late or discarded entry" );
    }

    bool cb_called{ false };
    auto cb = [&cb_called]( const daq_dc_data_t& dc ) { cb_called = true; };
    buffer->process_if( gps_key( gps_time, 0 ), cb );
    if ( !cb_called )
    {
        throw std::runtime_error( "Processing callback not called" );
    }

    data = dummy_data( 7, gps_time, 0 );
    buffer->ingest( *data );
    if ( late_entries != 1 )
    {
        throw std::runtime_error( "Expected late entry not marked as late" );
    }
    if ( discarded_entries != 0 )
    {
        throw std::runtime_error( "Unexpected discard entry" );
    }

    late_entries.store( 0 );
    data = dummy_data( 7, gps_time - 1, 0 );
    buffer->ingest( *data );
    if ( discarded_entries != 1 )
    {
        throw std::runtime_error(
            "Expected discard entry not marked as discarded" );
    }
    if ( late_entries != 0 )
    {
        throw std::runtime_error( "Unexpected entry marked late" );
    }
}

int
main( int argc, char* argv[] )
{
    test_buffer_entry_late_entry( );
    return 0;
}