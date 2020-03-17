//
// Created by jonathan.hanks on 3/13/20.
//
#include "gap_check.hh"
#include <chrono>
#include <daq_core.h>

#include <sys/time.h>
#include <unistd.h>

#include <iostream>

namespace check_gap
{
    static std::int64_t
    time_now( )
    {
        timeval tv;
        gettimeofday( &tv, 0 );
        return static_cast< std::uint64_t >( tv.tv_sec * 1000 +
                                             tv.tv_usec / 1000 );
    }

    struct cycle_sample_t
    {
        unsigned int  cycle;
        unsigned int  gps;
        unsigned int  gps_nano;
        unsigned int  gps_cycle;
        std::uint64_t time_ms;
    };

    cycle_sample_t
    wait_for_time_change( volatile daq_multi_cycle_header_t& header )
    {
        cycle_sample_t results;
        unsigned int   cur_cycle = header.curCycle;
        while ( header.curCycle == cur_cycle )
        {
            usleep( 2000 );
        }
        results.time_ms = time_now( );
        results.cycle = header.curCycle;

        unsigned int   stride = header.cycleDataSize;
        volatile char* buffer_data =
            reinterpret_cast< volatile char* >( &header ) +
            sizeof( daq_multi_cycle_header_t );
        volatile daq_dc_data_t* daq =
            reinterpret_cast< volatile daq_dc_data_t* >(
                buffer_data + stride * header.curCycle );
        results.gps = daq->header.dcuheader[ 0 ].timeSec;
        results.gps_nano = daq->header.dcuheader[ 0 ].timeNSec;
        results.gps_cycle = daq->header.dcuheader[ 0 ].cycle;
        return results;
    }

    int
    check_gaps( volatile void* buffer, std::size_t buffer_size )
    {
        std::int64_t nano_step = 1000000000 / 16;
        std::int64_t nano_times[ 16 ];

        int cycle_jumps = 0;
        int cycle_mismatch = 0;
        int nano_mismatch = 0;
        int time_jump = 0;

        for ( int i = 0; i < 16; ++i )
        {
            nano_times[ i ] = i * nano_step;
        }
        if ( !buffer || buffer_size < 20 * 1024 * 1024 ||
             buffer_size > 100 * 1024 * 1024 )
        {
            return 0;
        }
        volatile daq_multi_cycle_data_t* multi_header =
            reinterpret_cast< volatile daq_multi_cycle_data_t* >( buffer );
        cycle_sample_t cur_sample =
            wait_for_time_change( multi_header->header );

        auto first = true;
        auto prev_sample_time = std::chrono::steady_clock::now();
        while ( true )
        {
            bool           error = false;
            cycle_sample_t new_sample =
                wait_for_time_change( multi_header->header );
            if (!first)
            {
                auto sample_time = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(sample_time - prev_sample_time);
                if ( duration.count() < 52 || duration.count() > 72 )
                {
                    std::cout << "Bad duration, cycle took " << duration.count() << "ms\n";
                }
                prev_sample_time = sample_time;
            }
            first = false;

            if ( new_sample.cycle !=
                 ( cur_sample.cycle + 1 ) % multi_header->header.maxCycle )
            {
                std::cout << "Cycle jump from " << cur_sample.cycle << " to "
                          << new_sample.cycle << "\n";
                ++cycle_jumps;
                error = true;
            }
            if ( new_sample.cycle != new_sample.gps_cycle )
            {
                std::cout << "Cycle counter mismatch " << new_sample.cycle
                          << ":" << new_sample.gps_cycle << "\n";
                ++cycle_mismatch;
                error = true;
            }
            if ( new_sample.gps_nano != nano_times[ new_sample.cycle ] )
            {
                std::cout << "Nanos/cycle mismatch " << new_sample.gps_nano
                          << ":" << new_sample.gps_cycle << "\n";
                ++nano_mismatch;
                error = true;
            }
            if ( ( new_sample.gps == cur_sample.gps &&
                   ( new_sample.gps_cycle == cur_sample.gps_cycle + 1 ) ) ||
                 ( new_sample.gps == cur_sample.gps + 1 &&
                   new_sample.gps_cycle == 0 && cur_sample.gps_cycle == 15 ) )
            {
            }
            else
            {
                std::cout << "Sample jump from " << cur_sample.gps << ":"
                          << cur_sample.gps_cycle << " to " << new_sample.gps
                          << ":" << new_sample.gps_cycle << "\n";
                ++time_jump;
                error = true;
            }
            std::cout << "Sample " << new_sample.gps << ":"
                      << new_sample.gps_cycle << " - deltat = "
                      << ( new_sample.time_ms - cur_sample.time_ms );
            if ( !error )
            {
                std::cout << "\t cycle_jumps " << cycle_jumps << " count_mis "
                          << cycle_mismatch << " nano_mis " << nano_mismatch
                          << " time_jumps " << time_jump << std::endl;
            }
            cur_sample = new_sample;
        }
    }
} // namespace check_gap