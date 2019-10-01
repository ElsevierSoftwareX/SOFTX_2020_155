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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <daqmap.h>

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

// Function prototypes
// ****************************************************************************************
int checkFileCrc( const char* );

unsigned long daqFileCrc;
typedef struct daqd_c
{
    int   num_chans;
    int   con_chans;
    int   val_events;
    int   con_events;
    float channel_value[ EDCU_MAX_CHANS ];
    char  channel_name[ EDCU_MAX_CHANS ][ 64 ];
    int   channel_status[ EDCU_MAX_CHANS ];
    long  gpsTime;
    long  epicsSync;
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
    *channel_status = args.op == CA_OP_CONN_UP ? 0 : 0xbad;
    if ( args.op == CA_OP_CONN_UP )
        daqd_edcu1.con_chans++;
    else
        daqd_edcu1.con_chans--;
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
    if ( args.type == DBR_FLOAT )
    {
        float val = *( (float*)args.dbr );
        *( (float*)( args.usr ) ) = val;
    }
    else
    {
        printf( "Arg type unknown\n" );
    }
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
    if ( params->datatype != 4 )
    {
        std::cerr << "EDC channels may only be floats\n";
        exit( 1 );
    }
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

    parseConfigFile( const_cast< char* >( daqfilename ),
                     crc,
                     channel_parse_callback,
                     -1,
                     (char*)0,
                     reinterpret_cast< void* >( &daqd_edcu1 ) );

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
                DBR_FLOAT,
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
    float* daqData;
    int    buf_size;
    int    ii;

    if ( num_chans_index != -1 )
    {
        daqd_edcu1.channel_value[ num_chans_index ] = daqd_edcu1.num_chans;
    }

    if ( con_chans_index != -1 )
    {
        daqd_edcu1.channel_value[ con_chans_index ] = daqd_edcu1.con_chans;
    }

    if ( nocon_chans_index != -1 )
    {
        daqd_edcu1.channel_value[ nocon_chans_index ] =
            daqd_edcu1.num_chans - daqd_edcu1.con_chans;
    }

    buf_size = DAQ_DCU_BLOCK_SIZE * DAQ_NUM_SWING_BUFFERS;
    daqData = (float*)( shmDataPtr + ( buf_size * daqBlockNum ) );
    memcpy( daqData,
            daqd_edcu1.channel_value,
            daqd_edcu1.num_chans * sizeof( float ) );
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
    std::cout << "-d <dcu id> - The dcu id number to use [52]\n";
    std::cout << "-i <ini file name> - The ini file to read [edc.ini]\n";
    std::cout << "-w <wait time in ms> - Number of ms to wait after each 16Hz "
                 "segment has starts [0]\n";
    std::cout << "-p <prefix> - Prefix to add to the connection stats channel "
                 "names\n";
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

/// Called on EPICS startup; This is generic EPICS provided function, modified
/// for LIGO use.
int
main( int argc, char* argv[] )
{
    // Addresses for SDF EPICS records.
    // Initialize request for file load on startup.
    long        status;
    int         request;
    int         daqTrigger;
    long        ropts = 0;
    long        nvals = 1;
    int         rdstatus = 0;
    char        timestring[ 128 ];
    int         ii;
    int         fivesectimer = 0;
    long        coeffFileCrc;
    char        modfilemsg[] = "Modified File Detected ";
    struct stat st = { 0 };
    char        filemsg[ 128 ];
    char        logmsg[ 256 ];
    int         pageNum = 0;
    int         pageNumDisp = 0;
    int         daqreset = 0;
    char        errMsg[ 64 ];
    int         send_daq_reset = 0;

    const char* daqsharedmemname = "edc_daq";
    // const char* syncsharedmemname = "-";
    const char* daqFile = "edc.ini";
    const char* prefix = "";
    int         mydcuid = 52;
    char        logfilename[ 256 ] = "";
    char        edculogfilename[ 256 ] = "";

    int delay_multiplier = 0;

    int cur_arg = 0;
    while ( ( cur_arg = getopt( argc, argv, "b:d:i:w:p:h" ) ) != EOF )
    {
        switch ( cur_arg )
        {
        case 'b':
            daqsharedmemname = optarg;
            break;
        case 'd':
            mydcuid = atoi( optarg );
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
        case 'h':
        default:
            usage( argv[ 0 ] );
            exit( 1 );
            break;
        }
    }

    printf( "My dcuid is %d\n", mydcuid );
    sleep( 2 );
    // **********************************************
    //

    // EDCU STUFF
    // ********************************************************************************************************

    for ( ii = 0; ii < EDCU_MAX_CHANS; ii++ )
        daqd_edcu1.channel_status[ ii ] = 0xbad;
    edcuInitialize( daqsharedmemname, "-" );
    // edcuCreateChanFile(daqDir,daqFile,pref);
    edcuCreateChanList( prefix, daqFile, &daqFileCrc );
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

        edcuWriteData(
            daqd_edcu1.epicsSync, daqd_edcu1.gpsTime, mydcuid, send_daq_reset );

        cycle = ( cycle + 1 ) % 16;
        transmit_time = transmit_time + time_step;
    }

    return ( 0 );
}
