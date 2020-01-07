//
// Created by jonathan.hanks on 10/10/19.
//

#include "analyze_rmipc.hh"

#include <time.h>
#include <unistd.h>
#include <daqmap.h>
#include <daq_core_defs.h>
#include <drv/fb.h>

#include <iomanip>
#include <iostream>
#include <sstream>

namespace analyze
{

    namespace rmipc
    {
        /*!
         * @brief a grouping of commonly needed offsets and structures in a
         * rmIpcStr style buffer
         */
        struct memory_layout
        {
            explicit memory_layout( volatile char* buf )
                : ipc( reinterpret_cast< volatile rmIpcStr* >(
                      buf + CDS_DAQ_NET_IPC_OFFSET ) ),
                  data( buf + CDS_DAQ_NET_DATA_OFFSET ),
                  tp_num( reinterpret_cast< volatile cdsDaqNetGdsTpNum* >(
                      buf + CDS_DAQ_NET_GDS_TP_TABLE_OFFSET ) )
            {
            }

            volatile rmIpcStr*          ipc;
            volatile char*              data;
            volatile cdsDaqNetGdsTpNum* tp_num;
        };

        static const std::size_t buf_size =
            DAQ_DCU_BLOCK_SIZE * DAQ_NUM_SWING_BUFFERS;

        double
        time_diff( const struct timespec& t0, const struct timespec& t1 )
        {
            double t0d = static_cast< double >( t0.tv_sec ) +
                static_cast< double >( t0.tv_nsec ) / 1000000000.0;
            double t1d = static_cast< double >( t1.tv_sec ) +
                static_cast< double >( t1.tv_nsec ) / 1000000000.0;
            return t1d - t0d;
        }

        void
        simple_running_dump( memory_layout layout, const DataDecoder& decoder )
        {
            unsigned int last_cycle = layout.ipc->cycle;

            bool use_decoder = decoder.required_size( ) > 0;

            struct timespec last_time;
            clock_gettime( CLOCK_MONOTONIC, &last_time );
            while ( true )
            {
                unsigned int cur_cycle =
                    wait_until_changed( &( layout.ipc->cycle ), last_cycle );
                struct timespec cur_time;
                clock_gettime( CLOCK_MONOTONIC, &cur_time );

                if ( cur_cycle > 1024 )
                {
                    break;
                }

                unsigned int dcu_id = layout.ipc->dcuId;
                unsigned int status = layout.ipc->status;
                unsigned int channel_count = layout.ipc->channelCount;
                unsigned int data_size = layout.ipc->dataBlockSize;
                unsigned int blk_sec = layout.ipc->bp[ cur_cycle ].timeSec;
                unsigned int blk_nsec = layout.ipc->bp[ cur_cycle ].timeNSec;
                unsigned int blk_crc = layout.ipc->bp[ cur_cycle ].crc;

                last_cycle = cur_cycle;
                std::ostringstream os;
                os << "Cycle: " << std::setw( 2 ) << cur_cycle
                   << std::setw( 0 );
                os << " DCU: " << dcu_id;
                os << " Chan#: " << channel_count;
                os << " DataSize: " << data_size;
                os << " Status: " << status;
                os << " BlkCrc: " << blk_crc;
                os << " BlkSec: " << std::setw( 10 ) << blk_sec;
                os << ":" << std::setw( 2 ) << blk_nsec;

                if ( use_decoder )
                {
                    os << "  -  ";
                    decoder.decode( reinterpret_cast< volatile void* >(
                                        layout.data + cur_cycle * buf_size ),
                                    os );
                }

                double time_delta = time_diff( last_time, cur_time );
                last_time = cur_time;

                os << " delta(ms): " << time_delta * 1000;

                std::cout << os.str( ) << std::endl;
            }
        }
    } // namespace rmipc

    void
    analyze_rmipc( volatile void*    buffer,
                   std::size_t       size,
                   const ConfigOpts& options )
    {
        rmipc::memory_layout layout(
            reinterpret_cast< volatile char* >( buffer ) );

        rmipc::simple_running_dump( layout, options.decoder );
    }
} // namespace analyze
