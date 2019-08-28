
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "../drv/crc.c"
#include <time.h>
#include "../include/daqmap.h"
#include "../include/daq_core.h"

#include "sisci_types.h"
#include "sisci_api.h"
#include "sisci_error.h"
#include "sisci_demolib.h"
#include "testlib.h"

#include "./dolphin_common.c"

volatile int done = 0;

void
intHandler( int dummy )
{
    done = 1;
}

void
do_server( char* buffer, int buffer_size )
{
    int i = 0;

    for ( i = 0; !done; ++i )
    {
        if ( i > 255 )
        {
            i = 0;
        }
        memset( buffer, i, buffer_size );
        SCIMemCpy(
            sequence, buffer, remoteMap, 0, buffer_size, memcpyFlag, &error );
        SCIFlush( sequence, SCI_FLAG_FLUSH_CPU_BUFFERS_ONLY );
        printf( "%d\n", i );
        sleep( 1 );
    }
}

char
to_hex_nibble( int ch )
{
    ch = ch & 0x0f;
    if ( ch < 10 )
    {
        return '0' + ch;
    }
    return 'a' + ( ch - 10 );
}

void
print_hex( int ch )
{
    printf( "%c%c",
            to_hex_nibble( ( ch & 0x0f0 ) >> 4 ),
            to_hex_nibble( ch & 0x0f ) );
}

void
do_client( char* buffer, int buffer_size, int do_dump )
{
    int i = 0;
    while ( !done )
    {
        memcpy( buffer, readAddr, buffer_size );

        if ( do_dump )
        {
            printf( "\n" );
            for ( i = 0; i < 64; ++i )
            {
                if ( i % 4 == 0 && ( i != 0 && i != 32 ) )
                {
                    printf( " " );
                }
                if ( i == 32 )
                {
                    printf( "\n" );
                }
                printf( " " );
                print_hex( (int)( buffer[ i ] ) );
            }
            printf( "\n" );
        }
        else
        {
            printf( "Found %d\n", (int)buffer[ 0 ] );
            for ( i = 1; i < buffer_size; ++i )
            {
                if ( buffer[ i ] != buffer[ 0 ] )
                {
                    printf( "Mismatch mid buffer\n" );
                    break;
                }
            }
        }
        usleep( 1000 * 250 );
    }
}

void
usage( const char* prog_name )
{
    printf( "Usage:\n%s [options]\n\nOptions:\n", prog_name );
    printf( "\t-g <num> - dolphin group number\n" );
    printf( "\t-s - server side of the test\n" );
    printf( "\t-c - client side of the test\n" );
    printf( "\t-d - client side, dump data instead of checking buffer\n" );
    printf( "\n" );
}

int
main( int argc, char* argv[] )
{
    int segmentId = 0;
    int is_server = 0;
    int dump_data = 0;
    int c = 0;

    char* buffer = 0;
    int   buffer_size = 0;

    while ( ( c = getopt( argc, argv, "g:scdh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'g':
            segmentId = atoi( optarg );
            break;
        case 's':
            is_server = 1;
            dump_data = 0;
            break;
        case 'c':
            is_server = 0;
            dump_data = 0;
            break;
        case 'd':
            is_server = 0;
            dump_data = 1;
            break;
        case 'h':
        default:
            usage( argv[ 0 ] );
            exit( 1 );
            break;
        }
    }

    buffer_size = 6 * 1024 * 1024;
    buffer = malloc( buffer_size );
    if ( !buffer )
    {
        printf( "Unable to create temporary buffer\n" );
        exit( 1 );
    }

    signal( SIGINT, intHandler );

    error = dolphin_init( );
    fprintf( stderr,
             "Read = 0x%lx \n Write = 0x%lx \n",
             (long)readAddr,
             (long)writeAddr );

    if ( is_server )
    {
        do_server( buffer, buffer_size );
    }
    else
    {
        do_client( buffer, buffer_size, dump_data );
    }

    free( buffer );

    dolphin_closeout( );
    return 0;
}
