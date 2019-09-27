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

#include "crc.h"

#include <daqmap.h>
#include <param.h>
extern "C" {
#include "findSharedMemory.h"
}
#include "cadef.h"
#include "fb.h"
#include "../../drv/gpstime/gpstime.h"
#include "gps.hh"

#include <iostream>

#define EDCU_MAX_CHANS 50000
// Gloabl variables
// ****************************************************************************************
char naughtyList[ EDCU_MAX_CHANS ][ 64 ];

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
static float         dataBuffer[ 2 ][ EDCU_MAX_CHANS ];
static int           timeIndex;
static int           cycleIndex;
static int           symmetricom_fd = -1;
int timemarks[ 16 ] = { 1000 * 1000,   63500 * 1000,  126000 * 1000,
                        188500 * 1000, 251000 * 1000, 313500 * 1000,
                        376000 * 1000, 438500 * 1000, 501000 * 1000,
                        563500 * 1000, 626000 * 1000, 688500 * 1000,
                        751000 * 1000, 813500 * 1000, 876000 * 1000,
                        938500 * 1000 };
int nextTrig = 0;

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

// **************************************************************************
int
edcuFindUnconnChannels( )
// **************************************************************************
{
    int ii;
    int dcc = 0;

    for ( ii = 0; ii < daqd_edcu1.num_chans; ii++ )
    {
        if ( daqd_edcu1.channel_status[ ii ] != 0 )
        {
            sprintf( naughtyList[ dcc ], "%s", daqd_edcu1.channel_name[ ii ] );
            dcc++;
        }
    }
    return ( dcc );
}

/**
 * Scan the input text for the first non-whitespace character and return a
 * pointer to that location.
 * @param line NULL terminated string to check.
 * @return Pointer to the the first non whitespace (space, tab, nl, cr)
 * character.  Returns NULL iff line is NULL.
 */
const char*
skip_whitespace( const char* line )
{
    const char* cur = line;
    char        ch = 0;
    if ( !line )
    {
        return NULL;
    }
    ch = *cur;
    while ( ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' )
    {
        ++cur;
        ch = *cur;
    }
    return cur;
}

/**
 * Given a line with an comment denoted by '#' terminate
 * the line at the start of the comment.
 * @param line The line to modify, a NULL terminated string
 * @note This may modify the string pointed to by line.
 * This is safe to call with a NULL pointer.
 */
void
remove_line_comments( char* line )
{
    char ch = 0;

    if ( !line )
    {
        return;
    }
    while ( ( ch = *line ) )
    {
        if ( ch == '#' )
        {
            *line = '\0';
            return;
        }
        ++line;
    }
}

/**
 * Given a NULL terminated string remove any trailing whitespace
 * @param line The line to modify, a NULL terminated string
 * @note This may modify the string pointed to by line.
 * This is safe to call with a NULL pointer.
 */
void
remove_trailing_whitespace( char* newname )
{
    char* cur = newname;
    char* last_non_ws = NULL;
    char  ch = 0;

    if ( !newname )
    {
        return;
    }
    ch = *cur;
    while ( ch )
    {
        if ( ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n' )
        {
            last_non_ws = cur;
        }
        ++cur;
        ch = *cur;
    }
    if ( !last_non_ws )
    {
        *newname = '\0';
    }
    else
    {
        last_non_ws++;
        *last_non_ws = '\0';
    }
}

// **************************************************************************
void
edcuCreateChanFile( char* fdir, char* edcuinifilename, char* fecid )
{
    // **************************************************************************
    int   ok = 0;
    int   i = 0;
    int   status = 0;
    char  errMsg[ 64 ] = "";
    FILE* daqfileptr = NULL;
    FILE* edcuini = NULL;
    FILE* edcumaster = NULL;
    char  masterfile[ 64 ] = "";
    char  edcuheaderfilename[ 64 ] = "";
    char  line[ 128 ] = "";
    char* newname = 0;
    char  edcufilename[ 64 ] = "";
    char* dcuid = 0;

    sprintf( errMsg, "%s", fecid );
    dcuid = strtok( errMsg, "-" );
    dcuid = strtok( NULL, "-" );
    sprintf( masterfile, "%s%s", fdir, "edcumaster.txt" );
    sprintf( edcuheaderfilename, "%s%s", fdir, "edcuheader.txt" );

    // Open the master file which contains list of EDCU files to read channels
    // from.
    edcumaster = fopen( masterfile, "r" );
    if ( edcumaster == NULL )
    {
        sprintf(
            errMsg, "DAQ FILE ERROR: FILE %s DOES NOT EXIST\n", masterfile );
        fprintf( stderr, "%s", errMsg );
        // logFileEntry( errMsg );
        goto done;
    }
    // Open the file to write the composite channel list.
    edcuini = fopen( edcuinifilename, "w" );
    if ( edcuini == NULL )
    {
        sprintf( errMsg,
                 "DAQ FILE ERROR: FILE %s DOES NOT EXIST\n",
                 edcuinifilename );
        fprintf( stderr, "%s", errMsg );
        // logFileEntry( errMsg );
        goto done;
    }

    // Write standard header into .ini file
    fprintf( edcuini, "%s", "[default] \n" );
    fprintf( edcuini, "%s", "gain=1.00 \n" );
    fprintf( edcuini, "%s", "datatype=4 \n" );
    fprintf( edcuini, "%s", "ifoid=0 \n" );
    fprintf( edcuini, "%s", "slope=1 \n" );
    fprintf( edcuini, "%s", "acquire=3 \n" );
    fprintf( edcuini, "%s", "offset=0 \n" );
    fprintf( edcuini, "%s", "units=undef \n" );
    fprintf( edcuini, "%s%s%s", "dcuid=", dcuid, " \n" );
    fprintf( edcuini, "%s", "datarate=16 \n\n" );

    // Read the master file entries.
    while ( fgets( line, sizeof line, edcumaster ) != NULL )
    {
        newname = strtok( line, "\n" );
        if ( !newname )
        {
            continue;
        }
        newname = (char*)skip_whitespace( newname );
        remove_line_comments( newname );
        remove_trailing_whitespace( newname );

        if ( *newname == '\0' )
        {
            continue;
        }
        strcpy( edcufilename, fdir );
        strcat( edcufilename, newname );
        printf( "File in master = %s\n", edcufilename );
        daqfileptr = fopen( edcufilename, "r" );
        if ( daqfileptr == NULL )
        {
            fprintf(
                stderr,
                "DAQ FILE ERROR: FILE %s DOES NOT EXIST OR CANNOT BE READ!\n",
                edcufilename );
            goto done;
        }
        while ( fgets( line, sizeof line, daqfileptr ) != NULL )
        {
            fprintf( edcuini, "%s", line );
        }
        fclose( daqfileptr );
        daqfileptr = NULL;
    }
    ok = 1;
done:
    if ( daqfileptr )
        fclose( daqfileptr );
    if ( edcuini )
        fclose( edcuini );
    if ( edcumaster )
        fclose( edcumaster );
    if ( !ok )
    {
        exit( 1 );
    }
}

int
veto_line_due_to_datarate( const char* line )
{
    const int datarate_eq_len = 9;

    if ( !line )
    {
        return 0;
    }
    if ( strncmp( "datarate=", line, datarate_eq_len ) != 0 )
    {
        return 0;
    }

    if ( strcmp( "16\n", line + datarate_eq_len ) == 0 ||
         strcmp( "16", line + datarate_eq_len ) == 0 )
    {
        return 0;
    }
    return 1;
}

int
veto_line_due_to_datatype( const char* line )
{
    const int datatype_eq_len = 9;

    if ( !line )
    {
        return 0;
    }
    if ( strncmp( "datatype=", line, datatype_eq_len ) != 0 )
    {
        return 0;
    }

    if ( strcmp( "4\n", line + datatype_eq_len ) == 0 ||
         strcmp( "4", line + datatype_eq_len ) == 0 )
    {
        return 0;
    }
    return 1;
}

// **************************************************************************
void
edcuCreateChanList( const char* pref,
                    const char* daqfilename,
                    const char* edculogfilename )
{
    // **************************************************************************
    int   i;
    int   status;
    FILE* daqfileptr;
    FILE* edculog;
    char  errMsg[ 64 ];
    // char daqfile[64];
    char  line[ 128 ];
    char* newname;

    char eccname[ 256 ];
    sprintf( eccname, "%s%s", pref, "EDCU_CHAN_CONN" );
    char chcntname[ 256 ];
    sprintf( chcntname, "%s%s", pref, "EDCU_CHAN_CNT" );
    char cnfname[ 256 ];
    sprintf( cnfname, "%s%s", pref, "EDCU_CHAN_NOCON" );

    // sprintf(daqfile, "%s%s", fdir, "EDCU.ini");
    daqd_edcu1.num_chans = 0;
    daqfileptr = fopen( daqfilename, "r" );
    if ( daqfileptr == NULL )
    {
        fprintf(
            stderr, "DAQ FILE ERROR: FILE %s DOES NOT EXIST\n", daqfilename );
    }
    edculog = fopen( edculogfilename, "w" );
    if ( daqfileptr == NULL )
    {
        fprintf( stderr,
                 "DAQ FILE ERROR: FILE %s DOES NOT EXIST\n",
                 edculogfilename );
    }
    while ( fgets( line, sizeof line, daqfileptr ) != NULL )
    {
        fprintf( edculog, "%s", line );
        status = strlen( line );
        if ( strncmp( line, "[", 1 ) == 0 && status > 0 )
        {
            newname = strtok( line, "]" );
            // printf("status = %d New name = %s and %s\n",status,line,newname);
            newname = strtok( line, "[" );
            // printf("status = %d New name = %s and %s\n",status,line,newname);
            if ( strcmp( newname, "default" ) == 0 )
            {
                printf( "DEFAULT channel = %s\n", newname );
            }
            else
            {
                // printf("NEW channel = %s\n", newname);
                sprintf( daqd_edcu1.channel_name[ daqd_edcu1.num_chans ],
                         "%s",
                         newname );
                daqd_edcu1.num_chans++;
            }
        }
        else
        {
            if ( veto_line_due_to_datarate( line ) )
            {
                fprintf(
                    stderr,
                    "Invalid data rate found, all entries must be 16Hz\n" );
                exit( 1 );
            }
            if ( veto_line_due_to_datatype( line ) )
            {
                fprintf( stderr,
                         "Invalid data type found, all entries must be 4 "
                         "(float)\n" );
                exit( 1 );
            }
        }
    }
    fclose( daqfileptr );
    fclose( edculog );

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

    timeIndex = 0;
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

/// Routine for logging messages to ioc.log file.
/// 	@param[in] message Ptr to string containing message to be logged.
// void
// logFileEntry( char* message )
//{
//    FILE*  log;
//    char   timestring[ 256 ];
//    long   status;
//    dbAddr paddr;
//
//    getSdfTime( timestring );
//    log = fopen( logfilename, "a" );
//    if ( log == NULL )
//    {
//        status = dbNameToAddr( reloadtimechannel, &paddr );
//        status = dbPutField( &paddr, DBR_STRING, "ERR - NO LOG FILE FOUND", 1
//        );
//    }
//    else
//    {
//        fprintf( log, "%s\n%s\n", timestring, message );
//        fprintf( log, "***************************************************\n"
//        ); fclose( log );
//    }
//}

void
usage( const char* prog )
{
    std::cout << "Usage:\n\t" << prog << " <options>\n\n";
    std::cout
        << "-b <mbuf name> - The name of the mbuf to write to [edc_daq]\n";
    std::cout << "-l <log dir> - Directory to output logs to [logs]\n";
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
    const char* logdir = "logs";
    const char* daqFile = "edc.ini";
    const char* prefix = "";
    int         mydcuid = 52;
    char        logfilename[ 256 ] = "";
    char        edculogfilename[ 256 ] = "";

    int delay_multiplier = 0;

    int cur_arg = 0;
    while ( ( cur_arg = getopt( argc, argv, "b:l:d:i:w:p:h" ) ) != EOF )
    {
        switch ( cur_arg )
        {
        case 'b':
            daqsharedmemname = optarg;
            break;
        // case 't':
        //    syncsharedmemname = optarg;
        //    break;
        case 'l':
            logdir = optarg;
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

    if ( stat( logdir, &st ) == -1 )
        mkdir( logdir, 0777 );

    printf( "My dcuid is %d\n", mydcuid );
    sprintf( logfilename, "%s%s", logdir, "/ioc.log" );
    printf( "LOG FILE = %s\n", logfilename );
    sleep( 2 );
    // **********************************************
    //

    // EDCU STUFF
    // ********************************************************************************************************

    sprintf( edculogfilename, "%s%s", logdir, "/edcu.log" );
    for ( ii = 0; ii < EDCU_MAX_CHANS; ii++ )
        daqd_edcu1.channel_status[ ii ] = 0xbad;
    edcuInitialize( daqsharedmemname, "-" );
    // edcuCreateChanFile(daqDir,daqFile,pref);
    edcuCreateChanList( prefix, daqFile, edculogfilename );
    int datarate = daqd_edcu1.num_chans * 64 / 1000;

    // Start SPECT
    daqd_edcu1.gpsTime = symm_initialize( );
    daqd_edcu1.epicsSync = 0;

    // End SPECT

    int dropout = 0;
    int numDC = 0;
    int cycle = 0;
    int numReport = 0;

    // Initialize DAQ and COEFF file CRC checksums for later compares.
    daqFileCrc = checkFileCrc( daqFile );
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
        send_daq_reset = 0;
        //        status = dbPutField( &gpstimedisplayaddr,
        //                             DBR_LONG,
        //                             &daqd_edcu1.gpsTime,
        //                             1 ); // Init to zero.
        //        status = dbPutField( &daqbyteaddr, DBR_LONG, &datarate, 1 );
        int conChans = daqd_edcu1.con_chans;
        //        status = dbPutField( &eccaddr, DBR_LONG, &conChans, 1 );
        // Check unconnected channels once per second
        if ( daqd_edcu1.epicsSync == 0 )
        {
            //            status = dbGetField(
            //                &daqresetaddr, DBR_LONG, &daqreset, &ropts,
            //                &nvals, NULL );
            //            if ( daqreset )
            //            {
            //                status = dbPutField(
            //                    &daqresetaddr, DBR_LONG, &ropts, 1 ); // Init
            //                    to zero.
            //                send_daq_reset = 1;
            //            }
            numDC = edcuFindUnconnChannels( );
            if ( numDC < ( pageNumDisp * 40 ) )
                pageNumDisp--;
            //            numReport = edcuReportUnconnChannels( pref, numDC,
            //            pageNumDisp );
        }
        //        status = dbPutField( &chnotfoundaddr, DBR_LONG, &numDC, 1 );

        fivesectimer = ( fivesectimer + 1 ) %
            50; // Increment 5 second timer for triggering CRC checks.
        // Check file CRCs every 5 seconds.
        // DAQ and COEFF file checking was moved from skeleton.st to here RCG
        // V2.9.
        /*if(!fivesectimer) {
                status = checkFileCrc(daqFile);
                if(status != daqFileCrc) {
                        daqFileCrc = status;
                        status =
        dbPutField(&daqmsgaddr,DBR_STRING,modfilemsg,1); logFileEntry("Detected
        Change to DAQ Config file.");
                }
        }*/

        cycle = ( cycle + 1 ) % 16;
        transmit_time = transmit_time + time_step;
    }

    return ( 0 );
}
