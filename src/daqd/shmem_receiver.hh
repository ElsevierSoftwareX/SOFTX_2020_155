//
// Created by jonathan.hanks on 3/19/20.
//

#ifndef DAQD_SHMEM_RECEIVER_HH
#define DAQD_SHMEM_RECEIVER_HH

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>

#include "daq_core.h"
#include "drv/shmem.h"

class ShMemReceiver
{
    volatile daq_multi_cycle_data_t* shmem_;

    daq_dc_data_t data_;
    unsigned int  prev_cycle_;
    unsigned int  prev_gps_;

    void
    wait( )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
    }

public:
    ShMemReceiver( ) = delete;
    ShMemReceiver( ShMemReceiver& other ) = delete;
    ShMemReceiver( const std::string& endpoint, size_t shmem_size )
        : shmem_( static_cast< volatile daq_multi_cycle_data_t* >(
              shmem_open_segment( endpoint.c_str( ), shmem_size ) ) ),
          prev_cycle_( 0xffffffff ), prev_gps_( 0xffffffff )
    {
    }

    explicit ShMemReceiver( volatile daq_multi_cycle_data_t* shmem_area )
        : shmem_( shmem_area ), prev_cycle_( 0xffffffff ),
          prev_gps_( 0xffffffff )
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
        unsigned int cur_cycle = *cycle_ptr;
        while ( cur_cycle == prev_cycle_ )
        {
            wait( );
            cur_cycle = *cycle_ptr;
        }
        cur_cycle = handle_cycle_jumps( cur_cycle );

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

    int
    handle_cycle_jumps( unsigned int new_cycle )
    {
        unsigned int max_cycle = get_max_cycle( );
        unsigned int prev = prev_cycle_;
        unsigned int prev_gps = prev_gps_;

        if ( new_cycle == ( prev + 1 ) % max_cycle )
        {
            return new_cycle;
        }
        auto next_cycle = ( prev + 1 ) % max_cycle;
        while ( next_cycle != new_cycle )
        {
            auto expected_gps = prev_gps;
            if ( next_cycle < prev )
            {
                ++expected_gps;
            }
            if ( get_gps( next_cycle ) == expected_gps )
            {
                return next_cycle;
            }
            prev = next_cycle;
            next_cycle = ( prev + 1 ) % max_cycle;
        }
        return new_cycle;
    }
};

#endif // DAQD_// SHMEM_RECEIVER_HH
