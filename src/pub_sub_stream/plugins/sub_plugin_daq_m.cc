/*!
 * @file
 * @brief a subscription interface to the mbuf based daq_m data
 */

#include <iostream>
#include "sub_plugin_daq_m.hh"
#include <array>
#include <atomic>
#include <sstream>
#include <thread>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "make_unique.hh"

#include "daq_core.h"
#include "../../drv/gpstime/gpstime.h"
#include "arena.hh"
#include "drv/shmem.h"

//#include "../../drv/crc.c"
extern unsigned int crc_ptr( char*, unsigned int, unsigned int );
extern unsigned int crc_len( unsigned int, unsigned int );

namespace cps_plugins
{
    namespace detail
    {

        template < typename T >
        static T
        atomic_read( T* src )
        {
            return reinterpret_cast< std::atomic< T >* >( src )->load( );
        }

        class DaqMSub : public pub_sub::plugins::Subscription
        {
        public:
            explicit DaqMSub( const std::string&        name,
                              std::size_t               size_in_mb,
                              pub_sub::SubDebugNotices& debug,
                              pub_sub::SubHandler       handler )
                : Subscription( ),
                  shmem_{ (daq_multi_cycle_data_t*)shmem_open_segment(
                      name.c_str( ), size_in_mb * 1024 * 1024 ) },
                  handler_{ std::move( handler ) },
                  memory_arena_( 5 ), stopping_{ false }, th_{}
            {
                if ( !shmem_ )
                {
                    throw std::runtime_error(
                        "Unable to open the shared memory buffer" );
                }
                th_ = std::thread( [this]( ) { sub_loop( ); } );
            }
            ~DaqMSub( ) override
            {
                stopping_ = true;
                th_.join( );
            }

        private:
            const unsigned int INVALID_CYCLE = 50;

            unsigned int
            waitNextCycle( unsigned int prev_cycle )
            {
                const int check_for_stop_count = 200;

                int check_for_stop = check_for_stop_count;

                unsigned int cycle =
                    atomic_read( &( shmem_->header.curCycle ) );
                while ( cycle == prev_cycle )
                {
                    if ( --check_for_stop <= 0 )
                    {
                        if ( stopping_ )
                        {
                            return INVALID_CYCLE;
                        }
                        check_for_stop = check_for_stop_count;
                    }

                    std::this_thread::sleep_for(
                        std::chrono::milliseconds( 2 ) );
                    cycle = atomic_read( &( shmem_->header.curCycle ) );
                }
                return cycle;
            }

            void
            sub_loop( )
            {

                // wait for two cycles to make sure we don't just have random
                // noise in the buffer, when we have a changing buffer we will
                // assume it is being filled as expected
                auto cur_cycle = waitNextCycle( INVALID_CYCLE );
                if ( cur_cycle == INVALID_CYCLE )
                {
                    return;
                }

                while ( !stopping_ )
                {
                    cur_cycle = waitNextCycle( cur_cycle );
                    auto cycle_data_size =
                        atomic_read( &shmem_->header.cycleDataSize );
                    if ( cur_cycle > DAQ_NUM_DATA_BLOCKS )
                    {
                        continue;
                    }

                    auto* shmem_dc_data =
                        reinterpret_cast< volatile daq_dc_data_t* >(
                            ( &shmem_->dataBlock[ 0 ] ) +
                            cur_cycle * cycle_data_size );
                    if ( shmem_dc_data->header.dcuTotalModels < 1 )
                    {
                        continue;
                    }
                    std::size_t msg_size = sizeof( daq_multi_dcu_header_t ) +
                        shmem_dc_data->header.fullDataBlockSize;

                    if ( msg_size > sizeof( daq_dc_data_t ) )
                    {
                        // is there a better way to handle overly large
                        // messages than just silently truncating it?
                        msg_size = sizeof( daq_dc_data_t );
                    }

                    auto  data_ptr = memory_arena_.get( );
                    auto* dest = (char*)data_ptr.get( );

                    auto start =
                        reinterpret_cast< volatile char* >( shmem_dc_data );
                    auto stop = start + msg_size;

                    std::copy( start, stop, dest );
                    auto* dest_struct =
                        reinterpret_cast< daq_dc_data_t* >( dest );

                    pub_sub::KeyType key =
                        ( static_cast< pub_sub::KeyType >(
                              dest_struct->header.dcuheader[ 0 ].timeSec )
                          << 4 ) |
                        ( cur_cycle & 0x0f );
                    // std::cout << "Sending messge " << key << std::endl;
                    handler_( pub_sub::SubMessage(
                        sub_id( ),
                        key,
                        pub_sub::Message( std::move( data_ptr ), msg_size ) ) );
                }
            }

            daq_multi_cycle_data_t* shmem_;
            Arena                   memory_arena_;
            pub_sub::SubHandler     handler_;
            std::atomic< bool >     stopping_;
            std::thread             th_;
        };
    } // namespace detail

    SubPluginDaqMApi::SubPluginDaqMApi( )
        : SubscriptionPluginApi( ), subscriptions_( )
    {
    }
    SubPluginDaqMApi::~SubPluginDaqMApi( ) = default;

    const std::string&
    SubPluginDaqMApi::prefix( ) const
    {
        static const std::string my_prefix = "daqm://";
        return my_prefix;
    }

    const std::string&
    SubPluginDaqMApi::version( ) const
    {
        static const std::string my_version = "0";
        return my_version;
    }

    const std::string&
    SubPluginDaqMApi::name( ) const
    {
        static const std::string my_name = "daq multi dcu memory buffer";
        return my_name;
    }

    pub_sub::SubId
    SubPluginDaqMApi::subscribe( const std::string&        address,
                                 pub_sub::SubDebugNotices& debug_hooks,
                                 pub_sub::SubHandler       handler )
    {
        if ( address.find( prefix( ) ) != 0 )
        {
            throw std::runtime_error( "Invalid subscription type passed the "
                                      "the daq memory subscriber" );
        }
        auto        conn_str = address.substr( prefix( ).size( ) );
        auto        sep_index = conn_str.find( ':' );
        auto        name = conn_str;
        std::size_t buffer_size_mb = 100;
        if ( sep_index != std::string::npos )
        {
            name = conn_str.substr( 0, sep_index );
            auto size_str = conn_str.substr( sep_index + 1 );
            buffer_size_mb = std::stoi( size_str );
        }
        subscriptions_.emplace_back( make_unique_ptr< detail::DaqMSub >(
            name, buffer_size_mb, debug_hooks, std::move( handler ) ) );
        return subscriptions_.back( )->sub_id( );
    }

} // namespace cps_plugins