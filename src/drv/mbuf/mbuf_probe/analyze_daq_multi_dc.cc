//
// Created by jonathan.hanks on 10/11/19.
//

#include "analyze_daq_multi_dc.hh"

#include "mbuf.h"
#include "daq_core.h"

#include <algorithm>
#include <iterator>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace analyze
{
    namespace multi_dc
    {
        void
        simple_running_dump_single_dcu(
            volatile daq_multi_cycle_data_t* multi_data,
            int                              dcu_id,
            const DataDecoder&               decoder )
        {

            unsigned int last_cycle = multi_data->header.curCycle;

            bool use_decoder = ( decoder.required_size( ) > 0 );

            while ( true )
            {
                unsigned int cur_cycle = wait_until_changed(
                    &( multi_data->header.curCycle ), last_cycle );
                if ( cur_cycle > 1024 )
                {
                    break;
                }

                last_cycle = cur_cycle;

                unsigned int data_size = multi_data->header.cycleDataSize;
                std::size_t  cycle_offset = data_size * cur_cycle;

                volatile daq_dc_data_t* dc_data =
                    reinterpret_cast< volatile daq_dc_data_t* >(
                        &( multi_data->dataBlock[ 0 ] ) + cycle_offset );
                unsigned int dcu_count = dc_data->header.dcuTotalModels;
                volatile daq_msg_header_t* header =
                    (volatile daq_msg_header_t*)0;
                volatile char* data = &( dc_data->dataBlock[ 0 ] );
                for ( int i = 0; i < dcu_count; ++i )
                {
                    if ( dc_data->header.dcuheader[ i ].dcuId == dcu_id )
                    {
                        header = &( dc_data->header.dcuheader[ i ] );
                        break;
                    }
                    data += dc_data->header.dcuheader[ i ].dataBlockSize +
                        dc_data->header.dcuheader[ i ].tpBlockSize;
                }

                std::ostringstream os;
                os << "Cycle: " << std::setw( 2 ) << cur_cycle;
                if ( header )
                {
                    os << " dcu: " << std::setw( 3 ) << header->dcuId;
                    os << " gps: " << std::setw( 10 ) << header->timeSec << ":"
                       << std::setw( 2 ) << header->timeNSec;
                    os << " fileCrc " << header->fileCrc;
                    os << " data: " << header->dataBlockSize;
                    os << " tp: " << header->tpBlockSize;
                    if ( header->tpCount > 0 )
                    {
                        os << "(";
                        std::copy(
                            const_cast< unsigned int* >(
                                &( header->tpNum[ 0 ] ) ),
                            const_cast< unsigned int* >(
                                &( header->tpNum[ header->tpCount ] ) ),
                            std::ostream_iterator< unsigned int >( os, "," ) );
                        os << ") ";
                    }
                    if ( use_decoder )
                    {
                        os << "  -  ";
                        decoder.decode(
                            reinterpret_cast< volatile void* >( data ), os );
                    }
                }
                else
                {
                    os << " not found in this cycle\n";
                }

                std::cout << os.str( ) << std::endl;
            }
        }

        void
        simple_running_dump( volatile daq_multi_cycle_data_t* multi_data )
        {

            unsigned int last_cycle = multi_data->header.curCycle;

            while ( true )
            {
                unsigned int cur_cycle = wait_until_changed(
                    &( multi_data->header.curCycle ), last_cycle );
                if ( cur_cycle > 1024 )
                {
                    break;
                }

                unsigned int data_size = multi_data->header.cycleDataSize;
                unsigned int max_cycle = multi_data->header.maxCycle;

                std::size_t cycle_offset = data_size * cur_cycle;

                volatile daq_dc_data_t* dc_data =
                    reinterpret_cast< volatile daq_dc_data_t* >(
                        &( multi_data->dataBlock[ 0 ] ) + cycle_offset );
                unsigned int dcu_count = dc_data->header.dcuTotalModels;

                last_cycle = cur_cycle;

                std::ostringstream os;

                os << "Cycle: " << std::setw( 2 ) << cur_cycle << "/"
                   << std::setw( 2 ) << max_cycle;
                os << " DataSize: " << data_size;
                os << " DcuCount: " << dcu_count;
                os << " DataBlockSize: " << dc_data->header.fullDataBlockSize;

                std::uint64_t cycle_data_in_bytes = 0;
                for ( unsigned int i = 0; i < dcu_count; ++i )
                {
                    volatile daq_msg_header_t& header =
                        dc_data->header.dcuheader[ i ];
                    os << "\n\tdcu: " << std::setw( 3 ) << header.dcuId;
                    os << " gps: " << std::setw( 10 ) << header.timeSec << ":"
                       << std::setw( 2 ) << header.timeNSec;
                    os << " fileCrc: " << header.fileCrc;
                    os << " data: " << header.dataBlockSize;
                    os << " tp: " << header.tpBlockSize;
                    cycle_data_in_bytes += header.dataBlockSize;
                    cycle_data_in_bytes += header.tpBlockSize;

                    if ( header.tpCount > 0 )
                    {
                        os << "(";
                        std::copy(
                            const_cast< unsigned int* >(
                                &( header.tpNum[ 0 ] ) ),
                            const_cast< unsigned int* >(
                                &( header.tpNum[ header.tpCount ] ) ),
                            std::ostream_iterator< unsigned int >( os, "," ) );
                        os << ")";
                    }
                }
                if (cycle_data_in_bytes > dc_data->header.fullDataBlockSize)
                {
                    os << " overflow " << cycle_data_in_bytes << " > " << dc_data->header.fullDataBlockSize;
                }
                else if (cycle_data_in_bytes != static_cast<std::int64_t>(dc_data->header.fullDataBlockSize))
                {
                    os << " data  " << cycle_data_in_bytes << " != " << dc_data->header.fullDataBlockSize;
                }

                std::cout << os.str( ) << std::endl;
            }
        }
    } // namespace multi_dc

    void
    analyze_multi_dc( volatile void*    buffer,
                      std::size_t       size,
                      const ConfigOpts& options )
    {
        volatile daq_multi_cycle_data_t* multi_data =
            reinterpret_cast< volatile daq_multi_cycle_data_t* >( buffer );

        if ( options.dcu_id > 0 )
        {
            multi_dc::simple_running_dump_single_dcu(
                multi_data, options.dcu_id, options.decoder );
        }
        else
        {
            multi_dc::simple_running_dump( multi_data );
        }
    }
} // namespace analyze
