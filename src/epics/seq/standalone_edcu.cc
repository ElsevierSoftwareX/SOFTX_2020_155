///	@file /src/epics/seq/edcu.c
///	@brief Contains required 'main' function to startup EPICS sequencers,
/// along with supporting routines.
///<		This code is taken from EPICS example included in the EPICS
///< distribution and modified for LIGO use.

/********************COPYRIGHT NOTIFICATION**********************************
This software was developed under a United States Government license
described on the COPYRIGHT_UniversityOfChicago file included as part
of this distribution.
****************************************************************************/

// TODO:
// - Make appropriate log file entries
// - Get rid of need to build skeleton.st

#include <atomic>
#include <algorithm>
#include <array>
#include <iterator>
#include <string>
#include <thread>
#include <utility>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <daqmap.h>

#define BOOST_ASIO_USE_BOOST_DATE_TIME_FOR_SOCKET_IOSTREAM
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

/* taken from channel.h in the daqd source
 * find a way to do this better
 */
/* numbering must be contiguous */
typedef enum
{
    _undefined = 0,
    _16bit_integer = 1,
    _32bit_integer = 2,
    _64bit_integer = 3,
    _32bit_float = 4,
    _64bit_double = 5,
    _32bit_complex = 6,
    _32bit_uint = 7
} daq_data_t;

extern "C" {
#include "findSharedMemory.h"
#include "crc.h"
#include "param.h"
}
#include "cadef.h"
#include "fb.h"
#include "../../drv/gpstime/gpstime.h"
#include "gps.hh"

#include <iostream>

#define EDCU_MAX_CHANS 50000
#define DIAG_QUEUE_DEPTH 3

using boost::asio::ip::tcp;

#if BOOST_ASIO_VERSION < 101200
using io_context_t = boost::asio::io_service;
inline io_context_t&
get_context( tcp::acceptor& acceptor )
{
    return acceptor.get_io_service( );
}

inline boost::asio::ip::address
make_address( const char* str )
{
    return boost::asio::ip::address::from_string( str );
}
#else
using io_context_t = boost::asio::io_context;

inline io_context_t&
get_context( tcp::acceptor& acceptor )
{
    return acceptor.get_executor( ).context( );
}

inline boost::asio::ip::address
make_address( const char* str )
{
    return boost::asio::ip::make_address( str );
}
#endif

// Function prototypes
// ****************************************************************************************
int checkFileCrc( const char* );

typedef union edc_data_t
{
    int16_t data_int16;
    int32_t data_int32;
    float   data_float32;
    double  data_float64;
} edc_data_t;

unsigned long daqFileCrc;
typedef struct daqd_c
{
    int                num_chans;
    std::atomic< int > con_chans;
    int                val_events;
    int                con_events;
    daq_data_t         channel_type[ EDCU_MAX_CHANS ];
    edc_data_t         channel_value[ EDCU_MAX_CHANS ];
    char               channel_name[ EDCU_MAX_CHANS ][ 64 ];
    int                channel_status[ EDCU_MAX_CHANS ];
    long               gpsTime;
    long               epicsSync;
    char*              prefix;
    int                dcuid;
} daqd_c;

int num_chans_index = -1;
int con_chans_index = -1;
int nocon_chans_index = -1;
int internal_channel_count = 0;

daqd_c                           daqd_edcu1;
static struct rmIpcStr*          dipc;
static struct rmIpcStr*          sipc;
static char*                     shmDataPtr;
static struct cdsDaqNetGdsTpNum* shmTpTable;
static const int                 buf_size = DAQ_DCU_BLOCK_SIZE * 2;
static const int                 header_size =
    sizeof( struct rmIpcStr ) + sizeof( struct cdsDaqNetGdsTpNum );
static DAQ_XFER_INFO xferInfo;

static int symmetricom_fd = -1;
int        timemarks[ 16 ] = { 1000 * 1000,   63500 * 1000,  126000 * 1000,
                        188500 * 1000, 251000 * 1000, 313500 * 1000,
                        376000 * 1000, 438500 * 1000, 501000 * 1000,
                        563500 * 1000, 626000 * 1000, 688500 * 1000,
                        751000 * 1000, 813500 * 1000, 876000 * 1000,
                        938500 * 1000 };
int        nextTrig = 0;

/*!
 * @brief A collection of information used for diagnostic output
 */
struct diag_info_block
{
    diag_info_block( ) : con_chans( 0 ), status( )
    {
        std::fill( std::begin( status ), std::end( status ), 0xbad );
    }
    diag_info_block( const diag_info_block& other )
        : con_chans( other.con_chans ), status( )
    {
        std::copy( std::begin( other.status ),
                   std::end( other.status ),
                   std::begin( status ) );
    }
    diag_info_block&
    operator=( const diag_info_block& other )
    {
        con_chans = other.con_chans;
        std::copy( std::begin( other.status ),
                   std::end( other.status ),
                   std::begin( status ) );
        return *this;
    }

    int con_chans;
    int status[ EDCU_MAX_CHANS ];
};

typedef boost::lockfree::spsc_queue<
    diag_info_block*,
    boost::lockfree::capacity< DIAG_QUEUE_DEPTH > >
    diag_queue_t;

/*!
 * @brief this structure is used to refer to the message queues used between the
 * main thread and the diag thread.
 */
struct diag_thread_queues
{
    diag_thread_queues( diag_queue_t& message_queue,
                        diag_queue_t& free_list_queue )
        : msg_queue( message_queue ), free_queue( free_list_queue )
    {
    }

    diag_queue_t& msg_queue;
    diag_queue_t& free_queue;
};

/*!
 * @brief this structure is used to give the diag thread the information it
 * needs to start and run.
 */
struct diag_thread_args
{
    diag_thread_args( const std::string& hostname,
                      int                port,
                      diag_queue_t&      message_queue,
                      diag_queue_t&      free_list_queue )
        : address( hostname, port ), queues( message_queue, free_list_queue )
    {
    }
    diag_thread_args( diag_queue_t& message_queue,
                      diag_queue_t& free_list_queue )
        : address( "127.0.0.1", 9000 ), queues( message_queue, free_list_queue )
    {
    }
    std::pair< std::string, int > address;
    diag_thread_queues            queues;
};

/*!
 * @brief A Stream interface for rapid json to be paired with a writer.  It
 * doesn't write any data, instead calculating the size in bytes that would be
 * needed to actually encode the data written to it.
 */
class SizeCalcJsonStream
{
public:
    typedef char Ch;
    SizeCalcJsonStream( ) : count_( 0 )
    {
    }

    Ch
    Peek( ) const
    {
        throw std::runtime_error( "Peek not implemented" );
    }
    Ch
    Take( )
    {
        throw std::runtime_error( "Take not implemented" );
    }
    size_t
    Tell( )
    {
        throw std::runtime_error( "Tell not implemented" );
    }

    Ch*
    PutBegin( )
    {
        throw std::runtime_error( "PutBegin not implemented" );
    };

    void
    Put( Ch c )
    {
        ++count_;
    }
    void
    Flush( )
    {
    }
    size_t
    PutEnd( Ch* begin )
    {
        throw std::runtime_error( "PutEnd not implemented" );
    }

    bool
    SpaceAvailable( ) const
    {
        return true;
    }

    std::size_t
    GetSize( ) const
    {
        return count_;
    }

private:
    std::size_t count_;
};

/*!
 * @brief A Stream interface for rapid json to be paired with a writer.  It
 * writes into a fixed size buffer, and reports if there some space left.
 */
template < std::size_t MIN_SIZE >
class FixedSizeJsonStream
{
public:
    typedef char Ch;
    FixedSizeJsonStream( char* begin, char* end )
        : begin_( begin ), cur_( begin ), end_( end )
    {
    }

    Ch
    Peek( ) const
    {
        throw std::runtime_error( "Peek not implemented" );
    }
    Ch
    Take( )
    {
        throw std::runtime_error( "Take not implemented" );
    }
    size_t
    Tell( )
    {
        throw std::runtime_error( "Tell not implemented" );
    }

    Ch*
    PutBegin( )
    {
        throw std::runtime_error( "PutBegin not implemented" );
    };

    void
    Put( Ch c )
    {
        if ( cur_ < end_ )
        {
            *cur_ = c;
            ++cur_;
        }
        else
        {
            throw std::out_of_range( "The FixedSizeBuffer has overflown" );
        }
    }
    void
    Flush( )
    {
    }
    size_t
    PutEnd( Ch* begin )
    {
        throw std::runtime_error( "PutEnd not implemented" );
    }

    void
    AdvanceCounter( std::size_t count )
    {
        cur_ += count;
    }

    bool
    SpaceAvailable( ) const
    {
        return ( end_ - cur_ ) > MIN_SIZE;
    }

    Ch*
    GetString( ) const
    {
        return begin_;
    }

    std::size_t
    GetSize( ) const
    {
        return cur_ - begin_;
    }

    void
    Reset( )
    {
        cur_ = begin_;
    }

private:
    Ch* begin_;
    Ch* cur_;
    Ch* end_;
};

struct diag_client_conn
{
    explicit diag_client_conn( io_context_t& io )
        : io_context( io ), socket( io ), data( ), buffer( ),
          buffer_stream( buffer.data( ), buffer.data( ) + buffer.size( ) ),
          writer( buffer_stream ), list_progress( 0 )
    {
    }

    io_context_t&   io_context;
    tcp::socket     socket;
    diag_info_block data;

    std::array< char, 32000 >                       buffer;
    FixedSizeJsonStream< 256 >                      buffer_stream;
    rapidjson::Writer< FixedSizeJsonStream< 256 > > writer;
    int                                             list_progress;
};

// forward
void diag_thread_mainloop( diag_thread_args* args );
void update_diag_info( diag_thread_queues& queues );

// End Header ************************************************************
//

// **************************************************************************
/// Get current GPS time from the symmetricom IRIG-B card
unsigned long
symm_gps_time( unsigned long* frac, int* stt )
{
    // **************************************************************************
    unsigned long t[ 3 ];
    ioctl( symmetricom_fd, IOCTL_SYMMETRICOM_TIME, &t );
    t[ 1 ] *= 1000;
    t[ 1 ] += t[ 2 ];
    if ( frac )
        *frac = t[ 1 ];
    if ( stt )
        *stt = 0;
    // return  t[0] + daqd.symm_gps_offset;
    return t[ 0 ];
}
// **************************************************************************
void
waitGpsTrigger( unsigned long gpssec, int cycle )
// No longer used in favor of sync to IOP
// **************************************************************************
{
    unsigned long gpsSec, gpsuSec;
    int           gpsx;
    do
    {
        usleep( 1000 );
        gpsSec = symm_gps_time( &gpsuSec, &gpsx );
        gpsuSec /= 1000;
    } while ( gpsSec < gpssec || gpsuSec < timemarks[ cycle ] );
}

long
waitNewCycleGps( long* gps_sec )
{
    long          last_cycle = 0;
    static long   cycle = 0;
    static int    sync21pps = 1;
    unsigned long lastSec;

    unsigned long startTime = 0;
    unsigned long gpsSec, gpsNano;

    if ( sync21pps )
    {
        startTime = gpsSec = symm_gps_time( &gpsNano, 0 );
        printf( "\n%ld:%ld  (%d)\n",
                (long int)gpsSec,
                (long int)gpsNano,
                (long int)timemarks[ 0 ] );
        for ( ; startTime == gpsSec || gpsNano < timemarks[ 0 ]; )
        {
            printf( "%ld:%ld  (%d)\n",
                    (long int)gpsSec,
                    (long int)gpsNano,
                    (long int)timemarks[ 0 ] );

            usleep( 1000 );
            gpsSec = symm_gps_time( &gpsNano, 0 );
        }
        printf( "Found Sync at %ld %ld\nFrom start time of %ld\n",
                gpsSec,
                gpsNano,
                startTime );
        sync21pps = 0;
    }
    gpsSec = symm_gps_time( &gpsNano, 0 );
    while ( gpsNano < timemarks[ cycle ] )
    {
        usleep( 500 );
        gpsSec = symm_gps_time( &gpsNano, 0 );
    }
    *gps_sec = gpsSec;
    last_cycle = cycle;
    cycle++;
    cycle %= 16;
    // printf("new cycle %d %ld\n",newCycle,*gps_sec);
    return last_cycle;
}

// **************************************************************************
long
waitNewCycle( long* gps_sec )
// **************************************************************************
{
    static long   newCycle = 0;
    static int    lastCycle = 0;
    static int    sync21pps = 1;
    unsigned long lastSec;

    if ( !sipc )
    {
        return waitNewCycleGps( gps_sec );
    }

    if ( sync21pps )
    {
        for ( ; sipc->cycle; )
            usleep( 1000 );
        printf( "Found Sync at %ld %ld\n",
                sipc->bp[ lastCycle ].timeSec,
                sipc->bp[ lastCycle ].timeNSec );
        sync21pps = 0;
    }
    do
    {
        usleep( 1000 );
        newCycle = sipc->cycle;
    } while ( newCycle == lastCycle );
    *gps_sec = sipc->bp[ newCycle ].timeSec;
    // printf("new cycle %d %ld\n",newCycle,*gps_sec);
    lastCycle = newCycle;
    return newCycle;
}

// **************************************************************************
/// See if the GPS card is locked.
int
symm_gps_ok( )
{
    // **************************************************************************
    unsigned long req = 0;
    ioctl( symmetricom_fd, IOCTL_SYMMETRICOM_STATUS, &req );
    printf( "Symmetricom status: %s\n", req ? "LOCKED" : "UNCLOCKED" );
    return req;
}

// **************************************************************************
unsigned long
symm_initialize( )
// **************************************************************************
{
    symmetricom_fd = open( "/dev/gpstime", O_RDWR | O_SYNC );
    if ( symmetricom_fd < 0 )
    {
        perror( "/dev/gpstime" );
        exit( 1 );
    }
    unsigned long gpsSec, gpsuSec;
    int           gpsx;
    int           gpssync;
    gpssync = symm_gps_ok( );
    gpsSec = symm_gps_time( &gpsuSec, &gpsx );
    printf( "GPS SYNC = %d %d\n", gpssync, gpsx );
    printf( "GPS SEC = %ld  USEC = %ld  OTHER = %d\n", gpsSec, gpsuSec, gpsx );
    // Set system to start 2 sec from now.
    gpsSec += 2;
    return ( gpsSec );
}

// **************************************************************************
void
connectCallback( struct connection_handler_args args )
{
    // **************************************************************************
    int* channel_status = (int*)ca_puser( args.chid );
    int  new_status = ( args.op == CA_OP_CONN_UP ? 0 : 0xbad );
    /* In practice we have seen multiple disconnect events in a row w/o a
     * connect event. So only update when there is a change.  Otherwise this
     * code cannot count well.
     */
    if ( *channel_status != new_status )
    {
        *channel_status = new_status;
        if ( args.op == CA_OP_CONN_UP )
        {
            daqd_edcu1.con_chans++;
        }
        else
        {
            daqd_edcu1.con_chans--;
        }
    }
    daqd_edcu1.con_events++;
}

// **************************************************************************
void
subscriptionHandler( struct event_handler_args args )
{
    // **************************************************************************
    daqd_edcu1.val_events++;
    if ( args.status != ECA_NORMAL )
    {
        return;
    }
    switch ( args.type )
    {
    case DBR_SHORT:
    {
        int16_t val = *( (int16_t*)args.dbr );
        ( (edc_data_t*)( args.usr ) )->data_int16 = val;
    }
    break;
    case DBR_LONG:
    {
        int32_t val = *( (int32_t*)args.dbr );
        ( (edc_data_t*)( args.usr ) )->data_int32 = val;
    }
    break;
    case DBR_FLOAT:
    {
        float val = *( (float*)args.dbr );
        ( (edc_data_t*)( args.usr ) )->data_float32 = val;
    }
    break;
    case DBR_DOUBLE:
    {
        double val = *( (double*)args.dbr );
        ( (edc_data_t*)( args.usr ) )->data_float64 = val;
    }
    break;
    default:
        printf( "Arg type unknown\n" );
        break;
    }
}

bool
valid_data_type( daq_data_t datatype )
{
    switch ( datatype )
    {
    case _undefined:
    case _32bit_complex:
    case _32bit_uint:
    case _64bit_integer:
    default:
        return false;
    case _16bit_integer:
    case _32bit_integer:
    case _32bit_float:
    case _64bit_double:
        return true;
    }
    return true;
}

int
daq_data_t_to_epics( daq_data_t datatype )
{
    switch ( datatype )
    {
    case _16bit_integer:
        return DBR_SHORT;
    case _32bit_integer:
        return DBR_LONG;
    case _32bit_float:
        return DBR_FLOAT;
    case _64bit_double:
        return DBR_DOUBLE;
    default:
        throw std::runtime_error( "Unexpected data type given" );
    }
}

bool
channel_is_edcu_special_chan( daqd_c* edc, const char* channel_name )
{
    const char* dummy_prefix = "";
    const char* prefix = ( edc->prefix ? prefix : dummy_prefix );
    size_t      pref_len = strlen( prefix );
    size_t      name_len = strlen( channel_name );

    if ( name_len <= pref_len )
    {
        return false;
    }
    if ( strncmp( prefix, channel_name, pref_len ) != 0 )
    {
        return false;
    }
    const char* remainder = channel_name + pref_len;
    return ( strcmp( remainder, "EDCU_CHAN_CONN" ) == 0 ||
             strcmp( remainder, "EDCU_CHAN_CNT" ) == 0 ||
             strcmp( remainder, "EDCU_CHAN_NOCON" ) == 0 );
}

int
channel_parse_callback( char*              channel_name,
                        struct CHAN_PARAM* params,
                        void*              user )
{
    daqd_c* edc = reinterpret_cast< daqd_c* >( user );

    if ( !edc || !channel_name || !params )
    {
        return 0;
    }
    if ( edc->num_chans >= 50000 )
    {
        std::cerr << "Too many channels, aborting\n";
        exit( 1 );
    }
    if ( strlen( channel_name ) >=
         sizeof( edc->channel_name[ edc->num_chans ] ) )
    {
        std::cerr << "Channel name is too long '" << channel_name << "'\n";
        exit( 1 );
    }
    if ( params->datarate != 16 )
    {
        std::cerr << "EDC channels may only be 16Hz\n";
        exit( 1 );
    }
    if ( params->dcuid != edc->dcuid && edc->dcuid >= 0 )
    {
        std::cerr << "The edc can only have a single dcuid in its file\n";
        exit( 1 );
    }
    if ( edc->dcuid < 0 )
    {
        edc->dcuid = params->dcuid;
    }
    daq_data_t daq_data_type = static_cast< daq_data_t >( params->datatype );
    if ( !valid_data_type( daq_data_type ) )
    {
        std::cerr << "Invalid data type given for " << channel_name << "\n";
        exit( 1 );
    }
    if ( channel_is_edcu_special_chan( edc, channel_name ) &&
         daq_data_type != _32bit_integer )
    {
        std::cerr << "The edcu special variables (EDCU_CHAN_CONN/CNT/NOCON) "
                     "must be 32 bit ints ("
                  << static_cast< int >( _32bit_integer ) << ")\n";
        exit( 1 );
    }
    edc->channel_type[ edc->num_chans ] = daq_data_type;
    strncpy( edc->channel_name[ edc->num_chans ],
             channel_name,
             sizeof( edc->channel_name[ edc->num_chans ] ) );
    ++( edc->num_chans );
    return 1;
}

// **************************************************************************
void
edcuCreateChanList( const char*    pref,
                    const char*    daqfilename,
                    unsigned long* crc )
{
    // **************************************************************************
    int           i = 0;
    int           status = 0;
    unsigned long dummy_crc = 0;

    char eccname[ 256 ];
    sprintf( eccname, "%s%s", pref, "EDCU_CHAN_CONN" );
    char chcntname[ 256 ];
    sprintf( chcntname, "%s%s", pref, "EDCU_CHAN_CNT" );
    char cnfname[ 256 ];
    sprintf( cnfname, "%s%s", pref, "EDCU_CHAN_NOCON" );

    if ( !crc )
    {
        crc = &dummy_crc;
    }
    daqd_edcu1.num_chans = 0;

    daqd_edcu1.dcuid = -1;
    parseConfigFile( const_cast< char* >( daqfilename ),
                     crc,
                     channel_parse_callback,
                     -1,
                     (char*)0,
                     reinterpret_cast< void* >( &daqd_edcu1 ) );
    if ( daqd_edcu1.num_chans < 1 )
    {
        std::cerr << "No channels to record, aborting\n";
        exit( 1 );
    }

    xferInfo.crcLength = 4 * daqd_edcu1.num_chans;
    printf( "CRC data length = %d\n", xferInfo.crcLength );

    chid chid1;
    if ( ca_context_create( ca_enable_preemptive_callback ) != ECA_NORMAL )
    {
        fprintf( stderr, "Error creating the EPCIS CA context\n" );
        exit( 1 );
    }

    for ( i = 0; i < daqd_edcu1.num_chans; i++ )
    {
        if ( strcmp( daqd_edcu1.channel_name[ i ], chcntname ) == 0 )
        {
            num_chans_index = i;
            internal_channel_count = internal_channel_count + 1;
            daqd_edcu1.channel_status[ i ] = 0;
        }
        else if ( strcmp( daqd_edcu1.channel_name[ i ], eccname ) == 0 )
        {
            con_chans_index = i;
            internal_channel_count = internal_channel_count + 1;
            daqd_edcu1.channel_status[ i ] = 0;
        }
        else if ( strcmp( daqd_edcu1.channel_name[ i ], cnfname ) == 0 )
        {
            nocon_chans_index = i;
            internal_channel_count = internal_channel_count + 1;
            daqd_edcu1.channel_status[ i ] = 0;
        }
        else
        {
            status =
                ca_create_channel( daqd_edcu1.channel_name[ i ],
                                   connectCallback,
                                   (void*)&( daqd_edcu1.channel_status[ i ] ),
                                   0,
                                   &chid1 );
            if ( status != ECA_NORMAL )
            {
                fprintf( stderr,
                         "Error creating connection to %s\n",
                         daqd_edcu1.channel_name[ i ] );
            }
            status = ca_create_subscription(
                daq_data_t_to_epics( daqd_edcu1.channel_type[ i ] ),
                0,
                chid1,
                DBE_VALUE,
                subscriptionHandler,
                (void*)&( daqd_edcu1.channel_value[ i ] ),
                0 );
            if ( status != ECA_NORMAL )
            {
                fprintf( stderr,
                         "Error creating subscription for %s\n",
                         daqd_edcu1.channel_name[ i ] );
            }
        }
    }

    daqd_edcu1.con_chans = daqd_edcu1.con_chans + internal_channel_count;
}

// **************************************************************************
void
edcuWriteData( int           daqBlockNum,
               unsigned long cycle_gps_time,
               int           dcuId,
               int           daqreset )
// **************************************************************************
{
    char* daqData;
    int   buf_size;
    int   ii;

    if ( num_chans_index != -1 )
    {
        daqd_edcu1.channel_value[ num_chans_index ].data_int32 =
            daqd_edcu1.num_chans;
    }

    if ( con_chans_index != -1 )
    {
        daqd_edcu1.channel_value[ con_chans_index ].data_int32 =
            daqd_edcu1.con_chans;
    }

    if ( nocon_chans_index != -1 )
    {
        daqd_edcu1.channel_value[ nocon_chans_index ].data_int32 =
            daqd_edcu1.num_chans - daqd_edcu1.con_chans;
    }

    buf_size = DAQ_DCU_BLOCK_SIZE * DAQ_NUM_SWING_BUFFERS;
    daqData = (char*)( shmDataPtr + ( buf_size * daqBlockNum ) );
    for ( ii = 0; ii < daqd_edcu1.num_chans; ++ii )
    {
        switch ( daqd_edcu1.channel_type[ ii ] )
        {
        case _16bit_integer:
        {
            *reinterpret_cast< int16_t* >( daqData ) =
                daqd_edcu1.channel_value[ ii ].data_int16;
            daqData += sizeof( int16_t );
            break;
        }
        case _32bit_integer:
        {
            *reinterpret_cast< int32_t* >( daqData ) =
                daqd_edcu1.channel_value[ ii ].data_int32;
            daqData += sizeof( int32_t );
            break;
        }
        case _32bit_float:
        {
            *reinterpret_cast< float* >( daqData ) =
                daqd_edcu1.channel_value[ ii ].data_float32;
            daqData += sizeof( float );
            break;
        }
        case _64bit_double:
        {
            *reinterpret_cast< double* >( daqData ) =
                daqd_edcu1.channel_value[ ii ].data_float64;
            daqData += sizeof( double );
            break;
        }
        default:
            std::cerr << "Unknown data type found, the edc does not know how "
                         "to layout the data, aborting\n";
            exit( 1 );
        }
    }
    // memcpy( daqData,
    //        daqd_edcu1.channel_value,
    //        daqd_edcu1.num_chans * sizeof( float ) );
    dipc->dcuId = dcuId;
    dipc->crc = daqFileCrc;
    dipc->dataBlockSize = xferInfo.crcLength;
    dipc->bp[ daqBlockNum ].cycle = daqBlockNum;
    dipc->bp[ daqBlockNum ].crc = xferInfo.crcLength;
    dipc->bp[ daqBlockNum ].timeSec = (unsigned int)cycle_gps_time;
    dipc->bp[ daqBlockNum ].timeNSec = (unsigned int)daqBlockNum;
    if ( daqreset )
    {
        shmTpTable->count = 1;
        shmTpTable->tpNum[ 0 ] = 1;
    }
    else
    {
        shmTpTable->count = 0;
        shmTpTable->tpNum[ 0 ] = 0;
    }
    dipc->cycle = daqBlockNum; // Triggers sending of data by mx_stream.
}

// **************************************************************************
void
edcuInitialize( const char* shmem_fname, const char* sync_source )
// **************************************************************************
{
    void* sync_addr = 0;
    sipc = 0;

    // Find start of DAQ shared memory
    void* dcu_addr = (void*)findSharedMemory( (char*)shmem_fname );
    // Find the IPC area to communicate with mxstream
    dipc = (struct rmIpcStr*)( (char*)dcu_addr + CDS_DAQ_NET_IPC_OFFSET );
    // Find the DAQ data area.
    shmDataPtr = (char*)( (char*)dcu_addr + CDS_DAQ_NET_DATA_OFFSET );
    shmTpTable = (struct cdsDaqNetGdsTpNum*)( (char*)dcu_addr +
                                              CDS_DAQ_NET_GDS_TP_TABLE_OFFSET );

    if ( sync_source && strcmp( sync_source, "-" ) != 0 )
    {
        // Find Sync source
        sync_addr = (void*)findSharedMemory( (char*)sync_source );
        sipc = (struct rmIpcStr*)( (char*)sync_addr + CDS_DAQ_NET_IPC_OFFSET );
    }
}

/// Common routine to check file CRC.
///	@param[in] *fName	Name of file to check.
///	@return File CRC or -1 if file not found.
int
checkFileCrc( const char* fName )
{
    char        buffer[ 256 ];
    FILE*       pipePtr;
    struct stat statBuf;
    long        chkSum = -99999;
    strcpy( buffer, "cksum " );
    strcat( buffer, fName );
    if ( !stat( fName, &statBuf ) )
    {
        if ( ( pipePtr = popen( buffer, "r" ) ) != NULL )
        {
            fgets( buffer, 256, pipePtr );
            pclose( pipePtr );
            sscanf( buffer, "%ld", &chkSum );
        }
        return ( chkSum );
    }
    return ( -1 );
}

void
usage( const char* prog )
{
    std::cout << "Usage:\n\t" << prog << " <options>\n\n";
    std::cout
        << "-b <mbuf name> - The name of the mbuf to write to [edc_daq]\n";
    std::cout << "-i <ini file name> - The ini file to read [edc.ini]\n";
    std::cout << "-w <wait time in ms> - Number of ms to wait after each 16Hz "
                 "segment has starts [0]\n";
    std::cout << "-p <prefix> - Prefix to add to the connection stats channel "
                 "names\n";
    std::cout << "-l <interface:port> - Where to bind to for the diagnostic "
                 "http output\n";
    std::cout << "-h - this help\n";
    std::cout << "\nThe standalone edcu is used to record epics data and put "
                 "it into a memory buffer which can ";
    std::cout << "be consumed by the daqd tools.\n";
    std::cout << "Channels to record are listed in the input ini file.  They "
                 "must be floats (datatype=4) and 16Hz, ";
    std::cout << "datarate=16.\n";
    std::cout << "\nThe standalone daq requires the LIGO mbuf and gpstime "
                 "modules to be loaded.\n";
    std::cout << "\nSome special channels are produced by the standalone_edcu, "
                 "and may be send in the data stream.\n";
    std::cout << "\n\t<prefix>EDCU_CHAN_CONN\n\t<prefix>EDCU_CHAN_NOCON\n\t<"
                 "prefix>EDCU_CHAN_CNT\n";
    std::cout << "\nIn a typical setup standalone_edcu, local_dc, and "
                 "daqd_shmem would be run.\n";
    std::cout << "\n";
}

std::pair< std::string, int >
parse_address( const std::string& str )
{
    std::string hostname = str;
    int         port = 20222;
    auto        pos = str.find( ':' );
    if ( pos != std::string::npos )
    {
        hostname = str.substr( 0, pos );
        port = std::stoi( str.substr( pos + 1 ) );
    }
    return std::make_pair( std::move( hostname ), port );
}

/// Called on EPICS startup; This is generic EPICS provided function, modified
/// for LIGO use.
int
main( int argc, char* argv[] )
{
    // Addresses for SDF EPICS records.
    // Initialize request for file load on startup.
    int send_daq_reset = 0;

    diag_queue_t    diag_msg_queue;
    diag_queue_t    diag_free_queue;
    diag_info_block diag_info[ DIAG_QUEUE_DEPTH ];
    for ( int i = 0; i < DIAG_QUEUE_DEPTH; ++i )
    {
        diag_free_queue.push( &( diag_info[ i ] ) );
    }

    const char* daqsharedmemname = "edc_daq";
    // const char* syncsharedmemname = "-";
    const char* daqFile = "edc.ini";
    const char* prefix = "";

    int delay_multiplier = 0;

    diag_thread_args diag_args( diag_msg_queue, diag_free_queue );

    int cur_arg = 0;
    while ( ( cur_arg = getopt( argc, argv, "b:i:w:p:l:h" ) ) != EOF )
    {
        switch ( cur_arg )
        {
        case 'b':
            daqsharedmemname = optarg;
            break;
        case 'i':
            daqFile = optarg;
            break;
        case 'w':
            delay_multiplier = atoi( optarg );
            break;
        case 'p':
            prefix = optarg;
            break;
        case 'l':
            diag_args.address = parse_address( optarg );
            break;
        case 'h':
        default:
            usage( argv[ 0 ] );
            exit( 1 );
            break;
        }
    }

    memset( (void*)&daqd_edcu1, 0, sizeof( daqd_edcu1 ) );

    // **********************************************
    //

    // EDCU STUFF
    // ********************************************************************************************************

    for ( int ii = 0; ii < EDCU_MAX_CHANS; ii++ )
    {
        daqd_edcu1.channel_status[ ii ] = 0xbad;
    }
    edcuInitialize( daqsharedmemname, "-" );
    edcuCreateChanList( prefix, daqFile, &daqFileCrc );
    std::cout << "The edc dcuid = " << daqd_edcu1.dcuid << "\n";
    int datarate = daqd_edcu1.num_chans * 64 / 1000;

    // Start SPECT
    daqd_edcu1.gpsTime = symm_initialize( );
    daqd_edcu1.epicsSync = 0;

    // End SPECT

    int dropout = 0;
    int numDC = 0;
    int cycle = 0;
    int numReport = 0;

    printf( "DAQ file CRC = %u \n", daqFileCrc );
    fprintf( stderr,
             "%s\n%s = %u\n%s = %d",
             "EDCU code restart",
             "File CRC",
             daqFileCrc,
             "Chan Cnt",
             daqd_edcu1.num_chans );

    GPS::gps_clock clock( 0 );
    GPS::gps_time  time_step = GPS::gps_time( 0, 1000000000 / 16 );
    GPS::gps_time  transmit_time = clock.now( );
    ++transmit_time.sec;
    transmit_time.nanosec = 0;

    int cyle = 0;

    update_diag_info( diag_args.queues );
    std::thread diag_thread( diag_thread_mainloop, &diag_args );

    // Start Infinite Loop
    // *******************************************************************************
    for ( ;; )
    {
        dropout = 0;
        // daqd_edcu1.epicsSync = waitNewCycle( &daqd_edcu1.gpsTime );
        GPS::gps_time now = clock.now( );
        while ( now < transmit_time )
        {
            usleep( 1 );
            now = clock.now( );
        }
        usleep( delay_multiplier * 1000 );

        daqd_edcu1.gpsTime = now.sec;
        daqd_edcu1.epicsSync = cycle;

        edcuWriteData( daqd_edcu1.epicsSync,
                       daqd_edcu1.gpsTime,
                       daqd_edcu1.dcuid,
                       send_daq_reset );

        cycle = ( cycle + 1 ) % 16;
        transmit_time = transmit_time + time_step;

        if ( cycle == 0 )
        {
            update_diag_info( diag_args.queues );
        }
    }

    return ( 0 );
}

/*!
 * @brief Route to be called from the main thread to send a diag blog to the
 * diag thread.  This holds a snapshot of the status of the channels and
 * connections.
 * @param queues The set of queues used to communicate with between the threads
 */
void
update_diag_info( diag_thread_queues& queues )
{
    if ( !queues.free_queue.read_available( ) )
    {
        return;
    }
    diag_info_block* info = nullptr;
    queues.free_queue.pop( &info );

    info->con_chans = daqd_edcu1.con_chans;
    std::copy( std::begin( daqd_edcu1.channel_status ),
               std::begin( daqd_edcu1.channel_status ) + daqd_edcu1.num_chans,
               std::begin( info->status ) );

    queues.msg_queue.push( info );
}

/*!
 * @brief handler called from the diag thread periodically to pull diag
 * information out of the shared diag message queue.
 * @param queues The queues used to communicate with the main thread
 * @param dest The data block to copy the diag information to.
 */
void
get_diag_info( diag_thread_queues& queues, diag_info_block& dest )
{
    if ( !queues.msg_queue.read_available( ) )
    {
        return;
    }
    diag_info_block* info = nullptr;
    queues.msg_queue.pop( &info );

    dest = *info;

    queues.free_queue.push( info );
}

/*!
 * @brief timer callback for get_diag_info,  this just ensure the get_diag_info
 * is called and resets the time
 * @param timer the timer
 * @param queues The message queues
 * @param dest where to write the diag info
 */
void
get_diag_info_handler( boost::asio::deadline_timer& timer,
                       diag_thread_queues&          queues,
                       diag_info_block&             dest )
{
    get_diag_info( queues, dest );
    timer.expires_from_now( boost::posix_time::seconds( 1 ) );
    timer.async_wait(
        [&timer, &queues, &dest]( const boost::system::error_code& ) {
            get_diag_info_handler( timer, queues, dest );
        } );
}

void accept_loop( io_context_t&          io_context,
                  tcp::acceptor&         acceptor,
                  const diag_info_block& cur_diag );

/*!
 * @brief start writing the json by doing the initial header
 * @tparam Writer The writer
 * @param writer the writer
 * @param conn the connection diag info to dump
 */
template < typename Writer >
void
client_open_json( Writer& writer, diag_client_conn& conn )
{
    writer.StartObject( );
    writer.Key( "TotalChannels" );
    writer.Int( daqd_edcu1.num_chans );
    writer.Key( "ConnectedChannelCount" );
    writer.Int( conn.data.con_chans );
    writer.Key( "DisconnectedChannelCount" );
    writer.Int( daqd_edcu1.num_chans - conn.data.con_chans );
    writer.Key( "DisconnectedChannels" );
    writer.StartArray( );
}

/*!
 * @brief iteratate over the channel list and output channel entries until
 * the list is done, or the supplied buffer reports no more room.
 * @tparam Writer The writer
 * @tparam Buffer The buffer
 * @param writer The writer
 * @param buf The buffer
 * @param conn The connection and diag info to iterate over
 * @param cur The index to start on
 * @return The next index to start at (daqd_edc1.num_chans when it is complete)
 */
template < typename Writer, typename Buffer >
int
client_fill_json( Writer& writer, Buffer& buf, diag_client_conn& conn, int cur )
{
    for ( ; cur != daqd_edcu1.num_chans && buf.SpaceAvailable( ); ++cur )
    {
        if ( conn.data.status[ cur ] != 0 )
        {
            writer.String( daqd_edcu1.channel_name[ cur ] );
        }
    }
    return cur;
}

/*!
 * @brief Close out the client diagnositcs json message.
 * @tparam Writer The writer
 * @param writer the writer
 */
template < typename Writer >
void
client_close_json( Writer& writer )
{
    writer.EndArray( );
    writer.EndObject( );
}

/*!
 * @brief calculate the size needed to hold a json message representing the
 * diagnostics for the given client connection
 * @param conn client connection and diagnostics object
 * @return the number of bytes needed to hold the json message
 */
std::size_t
calc_required_size_for_json( diag_client_conn& conn )
{
    SizeCalcJsonStream                      counting_buffer;
    rapidjson::Writer< SizeCalcJsonStream > writer( counting_buffer );

    client_open_json( writer, conn );
    client_fill_json( writer, counting_buffer, conn, 0 );
    client_close_json( writer );
    return counting_buffer.GetSize( );
}

void
client_finalize( std::shared_ptr< diag_client_conn > conn )
{
    conn->buffer_stream.Reset( );
    client_close_json( conn->writer );
    boost::asio::async_write(
        conn->socket,
        boost::asio::buffer( conn->buffer_stream.GetString( ),
                             conn->buffer_stream.GetSize( ) ),
        [conn]( const boost::system::error_code& ec, std::size_t ) {} );
}

void
client_write( std::shared_ptr< diag_client_conn > conn )
{
    if ( conn->list_progress == daqd_edcu1.num_chans )
    {
        client_finalize( std::move( conn ) );
        return;
    }
    conn->buffer_stream.Reset( );
    conn->list_progress = client_fill_json(
        conn->writer, conn->buffer_stream, *conn, conn->list_progress );

    if ( conn->buffer_stream.GetSize( ) == 0 )
    {
        client_finalize( std::move( conn ) );
        return;
    }

    boost::asio::async_write(
        conn->socket,
        boost::asio::buffer( conn->buffer_stream.GetString( ),
                             conn->buffer_stream.GetSize( ) ),
        [conn]( const boost::system::error_code& ec, std::size_t ) {
            if ( !ec )
            {
                client_write( conn );
            }
        } );
}

void
start_client_write( std::shared_ptr< diag_client_conn > conn )
{
    const static char* header{ "HTTP/1.0 200 Ok\r\n"
                               "Content-Type: application/json\r\n"
                               "Cache-Control: max-age=1\r\n"
                               "Content-Length: %d\r\n"
                               "Connection: close\r\n\r\n" };

    std::size_t req_size = calc_required_size_for_json( *conn );

    std::size_t header_size = static_cast< std::size_t >(
        snprintf( conn->buffer.data( ),
                  conn->buffer.size( ),
                  header,
                  static_cast< int >( req_size ) ) );
    if ( header_size >= conn->buffer.size( ) )
    {
        throw std::out_of_range( "Buffer overflowed while sending headers" );
    }
    conn->buffer_stream.Reset( );
    conn->buffer_stream.AdvanceCounter( header_size );
    conn->writer.Reset( conn->buffer_stream );
    conn->list_progress = 0;
    client_open_json( conn->writer, *conn );

    conn->list_progress = client_fill_json(
        conn->writer, conn->buffer_stream, *conn, conn->list_progress );

    boost::asio::async_write(
        conn->socket,
        boost::asio::buffer( conn->buffer_stream.GetString( ),
                             conn->buffer_stream.GetSize( ) ),
        [conn]( const boost::system::error_code& ec, std::size_t ) {
            if ( !ec )
            {
                client_write( conn );
            }
        } );
}

void
handle_accept( io_context_t&                       io_context,
               tcp::acceptor&                      acceptor,
               std::shared_ptr< diag_client_conn > conn,
               const boost::system::error_code&    ec,
               const diag_info_block&              cur_diag )
{
    if ( !ec )
    {
        conn->data = cur_diag;
    }
    std::cerr << "Accepting a new connection\n";
    conn->io_context.post( [conn]( ) { start_client_write( conn ); } );
    accept_loop( io_context, acceptor, cur_diag );
}

void
accept_loop( io_context_t&          io_context,
             tcp::acceptor&         acceptor,
             const diag_info_block& cur_diag )
{
    std::shared_ptr< diag_client_conn > conn =
        std::make_shared< diag_client_conn >( io_context );
    acceptor.async_accept( conn->socket,
                           [&io_context, &acceptor, conn, &cur_diag](
                               const boost::system::error_code& ec ) {
                               handle_accept(
                                   io_context, acceptor, conn, ec, cur_diag );
                           } );
}

/*!
 * @brief the diag thread main loop
 * @param args Arguments to the diag thread
 */
void
diag_thread_mainloop( diag_thread_args* args )
{
    diag_info_block cur_diag;

    io_context_t       io_context;
    io_context_t::work net_work( io_context );

    boost::asio::deadline_timer diag_timer( io_context );
    get_diag_info_handler( diag_timer, args->queues, cur_diag );

    tcp::acceptor acceptor(
        io_context,
        tcp::endpoint( make_address( args->address.first.c_str( ) ),
                       args->address.second ) );

    accept_loop( io_context, acceptor, cur_diag );
    io_context.run( );
}
