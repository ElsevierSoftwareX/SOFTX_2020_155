/*!
 * @file
 * @brief a subscription interface to the mbuf based rmIpcStr data
 */

#include <iostream>
#include "sub_plugin_rmipc.hh"
#include <array>
#include <atomic>
#include <sstream>
#include <thread>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <boost/algorithm/string.hpp>

#include "make_unique.hh"

#include "daqmap.h"
#include "drv/fb.h"
#include "daq_core.h"
#include "../../drv/gpstime/gpstime.h"
#include "arena.hh"
#include "drv/shmem.h"
#include "modelrate.h"

//#include "../../drv/crc.c"
extern unsigned int crc_ptr( char*, unsigned int, unsigned int );
extern unsigned int crc_len( unsigned int, unsigned int );

namespace cps_plugins
{
    namespace detail
    {
        constexpr const int rmipc_max_subs = 128;
        class RmIpcSub : public pub_sub::plugins::Subscription
        {
            class File
            {
            public:
                File( const char* fname, int flags )
                    : fd_{ open( fname, flags ) }
                {
                }
                int
                get( ) const
                {
                    return fd_;
                }
                ~File( )
                {
                    if ( fd_ )
                    {
                        close( fd_ );
                    }
                }

            private:
                int fd_;
            };

        public:
            explicit RmIpcSub( const std::vector< std::string >& mbuf_names,
                               pub_sub::SubHandler               handler )
                : Subscription( ), subscriptions_{ mbuf_names.size( ) },
                  symmetricom_fd( "/dev/gpstime", O_RDWR | O_SYNC ),
                  handler_{ std::move( handler ) },
                  memory_arena_( 5 ), stopping_{ false }, th_{}
            {
                std::size_t i = 0;
                size_t FE_MBUF_SIZE = 64*1024*1024;

                if ( !symmetricom_fd.get( ) )
                {
                    throw std::runtime_error( "Unable to open /dev/gpstime" );
                }
                for ( const auto& name : mbuf_names )
                {
                    std::string shmem_fname = name + "_daq";
                    void*       dcu_addr = (void*)shmem_open_segment(
                        shmem_fname.c_str( ), FE_MBUF_SIZE );
                    if ( !dcu_addr )
                    {
                        throw std::runtime_error(
                            "Unable to map shared memory" );
                    }
                    shmIpcPtr[ i ] =
                        (struct rmIpcStr*)( (char*)dcu_addr +
                                            CDS_DAQ_NET_IPC_OFFSET );
                    shmDataPtr[ i ] =
                        ( (char*)dcu_addr + CDS_DAQ_NET_DATA_OFFSET );
                    shmTpTable[ i ] =
                        (struct
                         cdsDaqNetGdsTpNum*)( (char*)dcu_addr +
                                              CDS_DAQ_NET_GDS_TP_TABLE_OFFSET );
                    auto status = get_model_rate_dcuid(
                        &modelrates[ i ], &dcuid[ i ], name.c_str( ), nullptr );
                    if ( status != 0 || modelrates[ i ] == 0 )
                    {
                        std::ostringstream os;
                        os << "Unable to get the modelrate of " << name;
                        throw std::runtime_error( os.str( ) );
                    }
                    ++i;
                }
                th_ = std::thread( [this]( ) { rmipc_sub_loop( ); } );
            }
            ~RmIpcSub( ) override
            {
                stopping_ = true;
                th_.join( );
            }

            static constexpr int
            max_subs( )
            {
                return 128;
            }

        private:
            // **********************************************************************************************
            /// Get current GPS time from the symmetricom IRIG-B card
            unsigned long
            symm_gps_time( unsigned long* frac, int* stt )
            {
                unsigned long t[ 3 ];

                ioctl( symmetricom_fd.get( ), IOCTL_SYMMETRICOM_TIME, &t );
                t[ 1 ] *= 1000;
                t[ 1 ] += t[ 2 ];
                if ( frac )
                    *frac = t[ 1 ];
                if ( stt )
                    *stt = 0;
                return t[ 0 ];
            }

            // *******************************************************************************
            /// See if the GPS card is locked.
            int
            symm_ok( )
            {
                unsigned long req = 0;
                ioctl( symmetricom_fd.get( ), IOCTL_SYMMETRICOM_STATUS, &req );
                fprintf( stderr,
                         "Symmetricom status: %s\n",
                         req ? "LOCKED" : "UNCLOCKED" );
                return req;
            }



            // *******************************************************************************
            // Wait for data ready from FE models
            // *******************************************************************************
            int waitNextCycle2(
                unsigned int cyclereq, // Cycle to wait for
                int          reset ) // Request to reset model ipc shared memory

            {
                int iopRunning = 0;
                int ii;
                int threads_rdy = 0;
                int timeout = 0;

                // if reset, want to set all models cycle counters to impossible
                // number this takes care of uninitialized or stopped models
                if ( reset )
                {
                    for ( ii = 0; ii < subscriptions_; ++ii )
                    {
                        shmIpcPtr[ ii ]->cycle = 50;
                    }
                }
                usleep( 1000 );
                // Wait until received data from at least 1 FE or timeout
                do
                {
                    usleep( 2000 );
                    if ( shmIpcPtr[ 0 ]->cycle == cyclereq )
                    {
                        iopRunning = 1;
                        data_ready[ 0 ] = true;
                    }
                    timeout += 1;
                } while ( !iopRunning && timeout < 500 );

                // Wait until data received from everyone or timeout
                timeout = 0;
                do
                {
                    usleep( 100 );
                    for ( ii = 1; ii < subscriptions_; ii++ )
                    {
                        if ( shmIpcPtr[ ii ]->cycle == cyclereq &&
                             !data_ready[ ii ] )
                            threads_rdy++;
                        if ( shmIpcPtr[ ii ]->cycle == cyclereq )
                            data_ready[ ii ] = true;
                    }
                    timeout += 1;
                } while ( threads_rdy < subscriptions_ && timeout < 20 );

                return ( iopRunning );
            }

            // **********************************************************************************************
            int
            loadMessageBuffer( int                   lastCycle,
                               int                   status,
                               daq_multi_dcu_data_t& dest )
            {
                static const int buf_size = DAQ_DCU_BLOCK_SIZE * 2;
                static const int header_size =
                    sizeof( struct daq_multi_dcu_header_t );

                int   sendLength = 0;
                int   ii;
                int   dataXferSize;
                char* dataBuff;
                int   myCrc = 0;
                int   crcLength = 0;
                int   daqStatBit[ 2 ] = { 1, 2 };

                // Set pointer to 0MQ message data block
                char* zbuffer = (char*)&dest.dataBlock[ 0 ];
                // Initialize data send length to size of message header
                sendLength = header_size;
                // Set number of FE models that have data in this message
                dest.header.fullDataBlockSize = 0;
                int db = 0;
                // Loop thru all FE models
                for ( ii = 0; ii < subscriptions_; ii++ )
                {
                    if ( data_ready[ ii ] )
                    {
                        // Set heartbeat monitor for return to DAQ software
                        if ( lastCycle == 0 )
                            shmIpcPtr[ ii ]->reqAck ^= daqStatBit[ 0 ];
                        // Set DCU ID in header
                        dest.header.dcuheader[ db ].dcuId =
                            shmIpcPtr[ ii ]->dcuId;
                        // Set DAQ .ini file CRC checksum
                        dest.header.dcuheader[ db ].fileCrc =
                            shmIpcPtr[ ii ]->crc;
                        // Set 1/16Hz cycle number
                        dest.header.dcuheader[ db ].cycle =
                            shmIpcPtr[ ii ]->cycle;
                        // Set GPS seconds
                        dest.header.dcuheader[ db ].timeSec =
                            shmIpcPtr[ ii ]->bp[ lastCycle ].timeSec;
                        // Set GPS nanoseconds
                        dest.header.dcuheader[ db ].timeNSec =
                            shmIpcPtr[ ii ]->bp[ lastCycle ].timeNSec;
                        crcLength = shmIpcPtr[ ii ]->bp[ lastCycle ].crc;
                        // Set Status -- as running
                        dest.header.dcuheader[ db ].status = 2;
                        // Indicate size of data block
                        // ********ixDataBlock.header.dcuheader[db].dataBlockSize
                        // = shmIpcPtr[ii]->dataBlockSize;
                        dest.header.dcuheader[ db ].dataBlockSize = crcLength;
                        // Prevent going beyond MAX allowed data size
                        if ( dest.header.dcuheader[ db ].dataBlockSize >
                             DAQ_DCU_BLOCK_SIZE )
                            dest.header.dcuheader[ db ].dataBlockSize =
                                DAQ_DCU_BLOCK_SIZE;
                        // Calculate TP data size
                        dest.header.dcuheader[ db ].tpCount =
                            (unsigned int)shmTpTable[ ii ]->count & 0xff;
                        dest.header.dcuheader[ db ].tpBlockSize =
                            sizeof( float ) * modelrates[ ii ] *
                            dest.header.dcuheader[ db ].tpCount /
                            DAQ_NUM_DATA_BLOCKS_PER_SECOND;

                        // Copy GDSTP table to xmission buffer header
                        memcpy( &( dest.header.dcuheader[ db ].tpNum[ 0 ] ),
                                &( shmTpTable[ ii ]->tpNum[ 0 ] ),
                                sizeof( int ) *
                                    dest.header.dcuheader[ db ].tpCount );

                        // Set pointer to dcu data in shared memory
                        dataBuff =
                            (char*)( shmDataPtr[ ii ] + lastCycle * buf_size );
                        // Copy data from shared memory into local buffer
                        dataXferSize =
                            dest.header.dcuheader[ db ].dataBlockSize +
                            dest.header.dcuheader[ db ].tpBlockSize;
                        // if the dataXferSize is too large, something is wrong
                        // so return error message.
                        if ( dataXferSize > DAQ_DCU_BLOCK_SIZE )
                            return ( -1 );
                        memcpy( (void*)zbuffer, dataBuff, dataXferSize );

                        // Calculate CRC on the data and add to header info
                        myCrc = 0;
                        myCrc = crc_ptr( (char*)zbuffer, crcLength, 0 );
                        myCrc = crc_len( crcLength, myCrc );
                        dest.header.dcuheader[ db ].dataCrc = myCrc;

                        // Increment the 0mq data buffer pointer for next FE
                        zbuffer += dataXferSize;
                        // Increment the 0mq message size with size of FE data
                        // block
                        sendLength += dataXferSize;
                        // Increment the data block size for the message, this
                        // includes regular data + TP data
                        dest.header.fullDataBlockSize += dataXferSize;

                        // Update heartbeat monitor to DAQ code
                        if ( lastCycle == 0 )
                            shmIpcPtr[ ii ]->reqAck ^= daqStatBit[ 1 ];
                        db++;
                    }
                }
                dest.header.dcuTotalModels = db;
                return sendLength;
            }

            void
            rmipc_sub_loop( )
            {
                int          ii = 0;
                int          lastCycle = 0;
                unsigned int nextCycle = 0;

                int sync2iop = 1;
                int status = 0;

                int cur_req = 0;

                int myErrorSignal = 1;

                while ( !stopping_ )
                {

                    myErrorSignal = 0;

                    for ( ii = 0; ii < subscriptions_; ii++ )
                        data_ready[ ii ] = false;
                    status = waitNextCycle2( nextCycle, sync2iop );
                    // status = waitNextCycle(nextCycle,sync2iop,shmIpcPtr[0]);
                    if ( !status )
                    {
                        /* how do we handle timeout ? */
                        continue;
                    }
                    else
                        sync2iop = 0;

                    // IOP will be first model ready
                    // Need to wait for 2K models to reach end of their cycled
                    usleep( ( 1 * 1000 ) );

                    auto                  data_ptr = memory_arena_.get( );
                    daq_multi_dcu_data_t* multi_data =
                        (daq_multi_dcu_data_t*)data_ptr.get( );
                    int sendLength =
                        loadMessageBuffer( nextCycle, status, *multi_data );
                    if ( sendLength == -1 ||
                         sendLength > sizeof( *multi_data ) )
                    {
                        throw std::runtime_error(
                            "Message buffer overflow error" );
                    }
                    // Print diags in verbose mode
                    //                        if ( nextCycle == 8 && do_verbose
                    //                        )
                    //                            print_diags( nsys, lastCycle,
                    //                            sendLength, ixDataBlock );

                    pub_sub::KeyType key =
                        ( static_cast< pub_sub::KeyType >(
                              multi_data->header.dcuheader->timeSec )
                          << 4 ) |
                        ( nextCycle & 0x0f );
                    // std::cout << "Sending messge " << key << std::endl;
                    handler_( pub_sub::SubMessage(
                        sub_id( ),
                        key,
                        pub_sub::Message( std::move( data_ptr ),
                                          sendLength ) ) );
                    nextCycle = ( nextCycle + 1 ) % 16;
                }
            }

            std::array< struct rmIpcStr*, rmipc_max_subs >          shmIpcPtr;
            std::array< char*, rmipc_max_subs >                     shmDataPtr;
            std::array< struct cdsDaqNetGdsTpNum*, rmipc_max_subs > shmTpTable;
            std::array< bool, rmipc_max_subs >                      data_ready;
            std::array< int, rmipc_max_subs >                       modelrates;
            std::array< int, rmipc_max_subs >                       dcuid;
            File                symmetricom_fd;
            std::size_t         subscriptions_;
            Arena               memory_arena_;
            pub_sub::SubHandler handler_;
            std::atomic< bool > stopping_;
            std::thread         th_;
        };
    } // namespace detail

    SubPluginRmIpcApi::SubPluginRmIpcApi( )
        : SubscriptionPluginApi( ), subscriptions_( )
    {
    }
    SubPluginRmIpcApi::~SubPluginRmIpcApi( ) = default;

    const std::string&
    SubPluginRmIpcApi::prefix( ) const
    {
        static const std::string my_prefix = "rmipc://";
        return my_prefix;
    }

    const std::string&
    SubPluginRmIpcApi::version( ) const
    {
        static const std::string my_version = "0";
        return my_version;
    }

    const std::string&
    SubPluginRmIpcApi::name( ) const
    {
        static const std::string my_name = "rmipc";
        return my_name;
    }

    pub_sub::SubId
    SubPluginRmIpcApi::subscribe( const std::string&        conn_str,
                                  pub_sub::SubDebugNotices& debug_hooks,
                                  pub_sub::SubHandler       handler )
    {
        if ( conn_str.find( prefix( ) ) != 0 )
        {
            throw std::runtime_error(
                "Invalid subscription type passed the the rmipc subscriber" );
        }
        auto conn = conn_str.substr( prefix( ).size( ) );

        std::vector< std::string > mbuf_names;
        boost::algorithm::split( mbuf_names, conn, []( const char ch ) -> bool {
            return ch == ',';
        } );

        if ( mbuf_names.empty( ) )
        {
            throw std::runtime_error(
                "No subscriptions passed to the rmipc subscriber" );
        }
        subscriptions_.emplace_back( make_unique_ptr< detail::RmIpcSub >(
            mbuf_names, std::move( handler ) ) );
        return subscriptions_.back( )->sub_id( );
    }

} // namespace cps_plugins