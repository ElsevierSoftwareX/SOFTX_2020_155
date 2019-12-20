//
// Created by jonathan.hanks on 12/16/19.
//

#ifndef DAQD_TRUNK_RECV_BUFFER_HH
#define DAQD_TRUNK_RECV_BUFFER_HH

#include "daq_core.h"

#include <array>
#include <functional>
#include <iterator>
#include <mutex>
#include <stdint.h>

#include <sys/time.h>

/*!
 * @brief std::begin for arrays on old gcc
 */
template < typename T, size_t N >
T* array_begin( T ( &array )[ N ] )
{
    return array;
}

/*!
 * @brief std::end for arrays on old gcc
 */
template < typename T, size_t N >
T* array_end( T ( &array )[ N ] )
{
    return array + N;
}

/*!
 * @brief a combination of gps seconds and cycle number put together in a way
 * that can be used as an indexing key.
 */
struct gps_key
{
    typedef uint64_t key_type;
    typedef uint32_t gps_type;
    typedef uint32_t cycle_type;

    key_type key;

    gps_key( ) : key( 0 )
    {
    }
    gps_key( key_type sec, cycle_type cycle )
        : key( ( static_cast< key_type >( sec ) << shift_amount( ) ) |
               ( cycle & cycle_mask( ) ) )
    {
    }
    gps_key( const gps_key& other ) = default;
    gps_key& operator=( const gps_key& other ) = default;

    gps_type
    gps( ) const
    {
        return static_cast< gps_type >( key >> shift_amount( ) );
    }

    cycle_type
    cycle( ) const
    {
        return static_cast< cycle_type >( key & cycle_mask( ) );
    }

    bool
    operator==( const gps_key& other ) const
    {
        return key == other.key;
    }

    bool
    operator!=( const gps_key& other ) const
    {
        return key != other.key;
    }

    bool
    operator<( const gps_key& other ) const
    {
        return key < other.key;
    }

    bool
    operator<=( const gps_key& other ) const
    {
        return key <= other.key;
    }

    bool
    operator>( const gps_key& other ) const
    {
        return key > other.key;
    }

    bool
    operator>=( const gps_key& other ) const
    {
        return key >= other.key;
    }

    void
    atomic_load_from( const gps_key* source )
    {
        key = __sync_or_and_fetch( const_cast< key_type* >( &( source->key ) ),
                                   static_cast< key_type >( 0 ) );
    }

    void
    atomic_store_to( gps_key* dest )
    {
        gps_key old_key;
        old_key.atomic_load_from( dest );
        while ( old_key != *this )
        {
            old_key.key =
                __sync_val_compare_and_swap( &( dest->key ), old_key.key, key );
        }
    }

    template < typename Pred >
    void
    atomic_store_to_if( gps_key* dest, Pred& pred = Pred( ) )
    {
        gps_key old_key;
        old_key.atomic_load_from( dest );
        while ( pred( old_key, *this ) )
        {
            old_key.key =
                __sync_val_compare_and_swap( &( dest->key ), old_key.key, key );
        }
    }

private:
    explicit gps_key( key_type new_key ) : key( new_key )
    {
    }

    static key_type
    shift_amount( )
    {
        return 4;
    }

    static cycle_type
    cycle_mask( )
    {
        return static_cast< cycle_type >( 0xf );
    }
};

struct buffer_headers
{
    std::array< daq_msg_header_t, DAQ_TRANSIT_MAX_DCU > headers;
    std::array< int64_t, DAQ_TRANSIT_MAX_DCU >          time_ingested;
    gps_key                                             latest;
    unsigned int                                        dcu_count;
};

struct buffer_entry
{
    buffer_entry( )
        : m( ), latest( ), ifo_data( ), data( &ifo_data.dataBlock[ 0 ] ),
          time_ingested( )
    {
    }
    std::mutex                                 m;
    gps_key                                    latest;
    daq_dc_data_t                              ifo_data;
    char*                                      data;
    std::array< int64_t, DAQ_TRANSIT_MAX_DCU > time_ingested;

    static int64_t
    time_now( )
    {
        timeval tv;
        gettimeofday( &tv, 0 );
        return static_cast< int64_t >( tv.tv_sec * 1000 + tv.tv_usec / 1000 );
    }

    void
    ingest( const daq_multi_dcu_data_t& input )
    {
        gps_key key( input.header.dcuheader[ 0 ].timeSec,
                     input.header.dcuheader[ 0 ].cycle );

        int64_t timestamp = time_now( );

        std::lock_guard< std::mutex > l_( m );
        if ( key > latest )
        {
            clear( );
        }
        else if ( key < latest )
        {
            return;
        }

        const char* input_data = &input.dataBlock[ 0 ];
        for ( int i = 0; i < input.header.dcuTotalModels; ++i )
        {
            const daq_msg_header_t& cur_header = input.header.dcuheader[ i ];
            if ( cur_header.dcuId == 0 )
            {
                continue;
            }
            int dest_index = ifo_data.header.dcuTotalModels;
            if ( dest_index >= DAQ_TRANSIT_MAX_DCU )
            {
                return;
            }
            ifo_data.header.dcuheader[ dest_index ] = cur_header;
            std::size_t block_size =
                cur_header.dataBlockSize + cur_header.tpBlockSize;
            if ( data + block_size > array_end( ifo_data.dataBlock ) )
            {
                return;
            }
            std::copy( input_data, input_data + block_size, data );
            data += block_size;
            input_data += block_size;
            ifo_data.header.fullDataBlockSize += block_size;
            ++ifo_data.header.dcuTotalModels;
            time_ingested[ dest_index ] = timestamp;
        }
        latest = key;
    }

    template < typename Callback >
    void
    process_if( const gps_key process_key, Callback& cb = Callback( ) )
    {
        std::lock_guard< std::mutex > l_( m );
        if ( process_key != latest )
        {
            return;
        }
        cb( ifo_data );
    }

    void
    copy_headers( buffer_headers& dest )
    {
        std::lock_guard< std::mutex > l_( m );
        std::copy( array_begin( ifo_data.header.dcuheader ),
                   array_end( ifo_data.header.dcuheader ),
                   dest.headers.begin( ) );
        std::copy( time_ingested.begin( ),
                   time_ingested.end( ),
                   dest.time_ingested.begin( ) );
        dest.latest = latest;
        dest.dcu_count = ifo_data.header.dcuTotalModels;
    }

private:
    void
    clear( )
    {
        ifo_data.header.dcuTotalModels = 0;
        ifo_data.header.fullDataBlockSize = 0;
        data = &ifo_data.dataBlock[ 0 ];
    }
};

template < int N >
struct receive_buffer
{
#ifdef __cpp_static_assert
    static_assert( N > 0, "The receive buffer must have at least 1 segment" );
    static_assert( N <= 16, "A second is the most the buffer will hold" );
#endif
    typedef std::mutex mutex_type;

    receive_buffer( ) = default;
    receive_buffer( const receive_buffer& other ) = delete;
    receive_buffer( receive_buffer&& other ) = delete;
    receive_buffer& operator=( const receive_buffer& other ) = delete;
    receive_buffer& operator=( receive_buffer&& other ) = delete;

    void
    ingest( const daq_multi_dcu_data_t& input )
    {
        if ( input.header.dcuTotalModels <= 0 )
        {
            return;
        }
        int index = cycle_to_index( input.header.dcuheader[ 0 ].cycle );
        buffer_entry& target_buffer = buffer_[ index ];

        gps_key key( input.header.dcuheader[ 0 ].timeSec,
                     input.header.dcuheader[ 0 ].cycle );

        target_buffer.ingest( input );

        std::less< gps_key > comp;
        key.atomic_store_to_if( &latest_, comp );
    }

    gps_key
    latest( ) const
    {
        gps_key key;
        key.atomic_load_from( &latest_ );
        return key;
    }

    /*!
     * @brief Act on/process a slice of the buffer, as indexed by the given
     * gps_key
     * @tparam Callback The handler to process data with
     * @param process_key The key to index
     * @param cb The instance of the handler, a callable which is invoked with
     * the daq data as its argument
     * @note This will acquire any locks or do any synchronization to ensure
     * that cb is called with exclusive access to the data block.
     * @note Note that if process_key is not in the buffer, then cb will NOT be
     * called.
     */
    template < typename Callback >
    void
    process_slice_at( const gps_key process_key, Callback& cb = Callback( ) )
    {
        int           index = cycle_to_index( process_key.cycle( ) );
        buffer_entry& target_buffer = buffer_[ index ];

        target_buffer.process_if( process_key, cb );
    }

    template < typename It >
    void
    copy_headers( It dest )
    {
        for ( size_t i = 0; i < N; ++i )
        {
            buffer_[ i ].copy_headers( *dest );
            ++dest;
        }
    }

private:
    static int
    cycle_to_index( gps_key::cycle_type cycle )
    {
        return cycle % N;
    }

    gps_key                       latest_;
    std::array< buffer_entry, N > buffer_;
};

#endif // DAQD_TRUNK_RECV_BUFFER_HH
