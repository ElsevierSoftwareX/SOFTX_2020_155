// Scan archive directory to figure out GPS time ranges for data directories

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <dirent.h>

#include <iostream>
#include <fstream>
#include <list>
#include <set>
#include <map>
#include <string>

#include "nds.hh"

using namespace CDS_NDS;
using namespace std;

bool
Nds::scanArchiveDir( vector< ulong_pair >* tstamps )
{
    DIR*           dirp;
    struct dirent* direntp;
    char           dirname[ filename_max + 1 ];
    strcpy( dirname, mSpec.getArchiveDir( ).c_str( ) );

    int prefix_len = 0;

    // Skip the last part of the path
    int dirlen = strlen( dirname ) - 1;
    for ( int i = dirlen; i > 0; i-- )
    {
        if ( dirname[ i ] == '/' )
        {
            dirname[ i ] = 0;
            prefix_len = dirlen - i;
            break;
        }
    }

    if ( !( dirp = opendir( dirname ) ) )
    {
        system_log( 1, "Couldn't open archive directory `%s'", dirname );
        return false;
    }
    char* buf = (char*)malloc( sizeof( struct dirent ) + filename_max + 1 );
    typedef vector< ulong_pair >::const_iterator VITER;

#if defined( _POSIX_C_SOURCE )
    while ( !readdir_r( dirp, (struct dirent*)buf, &direntp ) )
#else
    while ( direntp = readdir_r( dirp, (struct dirent*)buf ) )
#endif
    {
        if ( !direntp )
            break;
        if ( strlen( direntp->d_name ) <= prefix_len )
            continue;
        system_log( 5, "scan(): `%s'", direntp->d_name );
        char* cfile = direntp->d_name + prefix_len;
        int   times;
        int   scanned = sscanf( cfile, "%d", &times );
        switch ( scanned )
        {
        case 1:
            tstamps->push_back(
                ulong_pair( times * 100000, ( times + 1 ) * 100000 ) );
            DEBUG( 5,
                   std::cerr << "scanned file:"
                             << "\t" << times * 100000 << std::endl );
            break;
        default:
            system_log( 5,
                        "scan(): `%s/%s' is invalid filename: skipped",
                        dirname,
                        direntp->d_name );
        }
    }
    (void)closedir( dirp );
    free( (void*)buf );

    if ( tstamps->size( ) == 0 )
    {
        system_log( 1,
                    "FATAL: archive `%s' is empty",
                    mSpec.getArchiveDir( ).c_str( ) );
        return false;
    }

    return true;
}
