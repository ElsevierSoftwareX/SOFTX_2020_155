// Copy striped minute trend file; skip unreadable portions

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifdef __linux__
#define STRUCT_SIZE 40
#else
#define STRUCT_SIZE 48
#endif

/* raw minute data structure is as follows:

*/
typedef struct
{
    union
    {
        int    I;
        double D;
        float  F;
    } min;
    union
    {
        int    I;
        double D;
        float  F;
    } max;
    int n; // the number of valid points used in calculating min, max, rms and
           // mean
    double rms;
    double mean;
} trend_block_t;

typedef struct raw_trend_record_struct
{
    unsigned long gps;
    trend_block_t tb;
} raw_trend_record_struct;

int
makesSense( unsigned int i )
{
    return i % 60 == 0;
}

main( int argc, char* argv[] )
{
    int           ifd, ofd;
    int           fpos;
    unsigned long i;
    unsigned long ifsize = 0;
    unsigned int  gpsTime = 0;

    printf( "%d\n", sizeof( raw_trend_record_struct ) );

    if ( argc != 3 && argc != 4 )
    {
        fprintf( stderr, "Usage: cpstriped <infile> <outfile> [end gps]\n" );
        exit( 1 );
    }
    if ( argc == 4 )
        gpsTime = atoi( argv[ 3 ] );
    ifd = open( argv[ 1 ], O_RDONLY );
    if ( ifd < 0 )
    {
        fprintf( stderr, "Cannot open input file `%s'\n", argv[ 1 ] );
        exit( 1 );
    }
    {
        struct stat buf;
        int         res = fstat( ifd, &buf );
        if ( res < 0 )
        {
            fprintf( stderr, "Cannot do an fstat()\n" );
            exit( 1 );
        }
        ifsize = buf.st_size;
    }
    ofd = open( argv[ 2 ], O_WRONLY | O_CREAT | O_TRUNC );
    if ( ifd < 0 )
    {
        fprintf( stderr, "Cannot open output file `%s'\n", argv[ 2 ] );
        exit( 1 );
    }

    for ( i = gpsTime; i < ifsize; i += STRUCT_SIZE )
    {
        int           n;
        unsigned char buf[ STRUCT_SIZE ];
        off_t         offs = lseek( ifd, i, SEEK_SET );
        if ( offs != i )
        {
            fprintf( stderr, "Failed lseek(%d)\n", i );
            exit( 1 );
        }
        n = read( ifd, buf, STRUCT_SIZE );
        if ( n != STRUCT_SIZE )
        {
            fprintf( stderr, "read failed at %d\n", i );
        }
        else
        {
            unsigned int gt = *( (unsigned int*)buf );
#if 0
			if (gt < gpsTime) {
			  if (makesSense(gt)) {
			    n = write(ofd, buf, STRUCT_SIZE);
			    if (n !=  STRUCT_SIZE) {
				fprintf(stderr, "write failed; errno=%d\n", errno);
				exit(1);
			    }
			  } else {
#endif
            fprintf( stderr, "GPS time %u at 0x%x makes no sense\n", gt, i );
#if 0
			  }
		   	} else {
			  exit(0);
			}
#endif
        }
    }
    close( ofd );
    sync( );
    exit( 0 );
}
