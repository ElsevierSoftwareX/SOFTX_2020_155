//
// Created by jonathan.hanks on 3/19/20.
//

#ifndef DAQD_SHMEM_RECEIVER_HH
#define DAQD_SHMEM_RECEIVER_HH

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>
#include <sstream>

#include "daq_core.h"
#include "drv/shmem.h"

class ShMemReceiver
{
public:
    ShMemReceiver( ) = delete;
    ShMemReceiver( ShMemReceiver& other ) = delete;
    ShMemReceiver( const std::string&            endpoint,
                   std::size_t                   shmem_size,
                   std::function< void( void ) > signal = []( ) {} )
        : shmem_( static_cast< volatile daq_multi_cycle_data_t* >(
              shmem_open_segment( endpoint.c_str( ), shmem_size ) ) ),
          shmem_size_( shmem_size ), signal_stalled_{ std::move( signal ) }
    {
    }

    ShMemReceiver( volatile daq_multi_cycle_data_t* shmem_area,
                   std::size_t                      shmem_size,
                   std::function< void( void ) >    signal = []( ) {} )
        : shmem_( shmem_area ),
          shmem_size_( shmem_size ), signal_stalled_{ std::move( signal ) }
    {
    }

    /*!
     * @brief a debugging/testing helper
     * @param new_value the value to set the previous to.
     */
    void
    reset_previous_cycle( unsigned int new_cycle, unsigned int new_gps )
    {
        prev_cycle_ = new_cycle;
        prev_gps_ = new_gps;
    }

    daq_dc_data_t*
    receive_data( )
    {
        std::atomic< unsigned int >* cycle_ptr =
            reinterpret_cast< std::atomic< unsigned int >* >(
                const_cast< unsigned int* >( &( shmem_->header.curCycle ) ) );
        if ( prev_cycle_ == 0xffffffff )
        {
            prev_cycle_ = *cycle_ptr;
            prev_gps_ = get_gps( prev_cycle_ );
        }
        std::uint64_t spin_count = 0;

        unsigned int cur_cycle = *cycle_ptr;
        unsigned int max_cycle = get_max_cycle( );

        while ( cur_cycle == prev_cycle_ || get_total_dcus( cur_cycle ) == 0 )
        {
            wait( );
            cur_cycle = *cycle_ptr;
            verify_cycle_state( cur_cycle, max_cycle );
            ++spin_count;
            if ( spin_count == 2000 )
            {
                signal_stalled_( );
            }
        }
        max_cycle = get_max_cycle( );
        verify_cycle_state( cur_cycle, max_cycle );
        cur_cycle = handle_cycle_jumps( cur_cycle, max_cycle );

        unsigned int cycle_stride = shmem_->header.cycleDataSize;
        // figure out offset to the right block
        // figure out how much data is actually there
        // memcpy into _data

        char* start = const_cast< char* >( &shmem_->dataBlock[ 0 ] );
        start += cur_cycle * cycle_stride;
        size_t copy_size = sizeof( daq_multi_dcu_header_t ) +
            reinterpret_cast< daq_multi_dcu_header_t* >( start )
                ->fullDataBlockSize;
        std::copy(
            start, start + copy_size, reinterpret_cast< char* >( &data_ ) );

        prev_cycle_ = cur_cycle;
        prev_gps_ = data_.header.dcuheader[ 0 ].timeSec;
        return &data_;
    }

private:
    unsigned int
    get_max_cycle( ) const
    {
        return shmem_->header.maxCycle;
    }

    unsigned int
    get_gps( unsigned int cycle )
    {
        unsigned int cycle_stride = shmem_->header.cycleDataSize;
        char*        start = const_cast< char* >( &shmem_->dataBlock[ 0 ] );
        start += cycle * cycle_stride;
        return reinterpret_cast< daq_multi_dcu_data_t* >( start )
            ->header.dcuheader[ 0 ]
            .timeSec;
    }

    unsigned int
    get_total_dcus( unsigned int cycle )
    {
        unsigned int cycle_stride = shmem_->header.cycleDataSize;
        char*        start = const_cast< char* >( &shmem_->dataBlock[ 0 ] );
        start += cycle * cycle_stride;
        return reinterpret_cast< daq_multi_dcu_data_t* >( start )
            ->header.dcuTotalModels;
    }

    void
    verify_cycle_state( unsigned int cur_cycle, unsigned int max_cycle )
    {
        if ( cur_cycle >= max_cycle || max_cycle > 64 )
        {
            throw std::runtime_error(
                "Invalid shmem header or nonsensical values" );
        }
        auto cycleDataSize = shmem_->header.cycleDataSize;
        if ( cycleDataSize * max_cycle + sizeof( daq_multi_cycle_header_t ) >
             shmem_size_ )
        {
            std::ostringstream os;
            os << "Overflow condition exists, the cycleDataSize is wrong for "
                  "the "
                  "shared memory size. cycleDataSize="
               << cycleDataSize << " max_cycle=" << max_cycle
               << " header_size=" << sizeof( daq_multi_cycle_header_t )
               << " shmem_size=" << shmem_size_;
            std::string msg = os.str( );
            throw std::runtime_error( msg );
        }
    }

    int
    handle_cycle_jumps( unsigned int new_cycle, unsigned int max_cycle )
    {
        unsigned int prev = prev_cycle_;
        unsigned int prev_gps = prev_gps_;

        auto next_cycle = ( prev + 1 ) % max_cycle;
        if ( new_cycle == next_cycle )
        {
            return new_cycle;
        }
        while ( next_cycle != new_cycle )
        {
            auto expected_gps = prev_gps;
            if ( next_cycle < prev )
            {
                ++expected_gps;
            }
            auto actual_gps = get_gps( next_cycle );
            if ( actual_gps == expected_gps &&
                 get_total_dcus( next_cycle ) != 0 )
            {
                std::cout
                    << "shmem_receiver handle_cycle_jumps found skipped cycle "
                    << actual_gps << ":" << next_cycle << "\n";
                return next_cycle;
            }
            std::cout << "shmem_receiver looking for gps=" << expected_gps
                      << " found " << actual_gps << " on cycle " << next_cycle
                      << "\n";
            prev = next_cycle;
            next_cycle = ( prev + 1 ) % max_cycle;
        }
        return new_cycle;
    }

    void
    wait( )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 3 ) );
    }

    volatile daq_multi_cycle_data_t* shmem_;
    std::size_t                      shmem_size_;

    daq_dc_data_t                 data_{};
    unsigned int                  prev_cycle_{ 0xffffffff };
    unsigned int                  prev_gps_{ 0xffffffff };
    std::function< void( void ) > signal_stalled_;
};

#endif // DAQD_// SHMEM_RECEIVER_HH
