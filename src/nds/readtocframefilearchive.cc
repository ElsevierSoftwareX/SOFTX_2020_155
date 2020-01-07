#include <stdio.h>
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
#include "daqd_net.hh"
#include "mmstream.hh"
#include "framecpp/Version8/FrameStreamPlan.hh"

using namespace CDS_NDS;
using namespace std;

// intersection predicate
class intersect : public unary_function< ulong_pair, bool >
{
    ulong_pair p;

public:
    intersect( const ulong_pair pr ) : p( pr )
    {
    }
    bool
    operator( )( const ulong_pair& p1 ) const
    {
        return p1.first <= p.second && p1.second > p.first;
    }
};

class falls_in : public unary_function< ulong_pair, bool >
{
    unsigned long a; // gps time (seconds)
public:
    falls_in( const unsigned long a1 ) : a( a1 )
    {
    }
    bool
    operator( )( const ulong_pair& p ) const
    {
        return p.first + p.second >
            a; // frame data span intersects with target gps time
    }
};

static void
error_watch( const string& msg )
{
    system_log( 1, "framecpp error: %s", msg.c_str( ) );
}

// extern FrameCPP::Dictionary *dict;

// read files of arbitrary length in seconds or frames
bool
Nds::readTocFrameFileArchive( )
{
    unsigned long t_start =
        time( 0 ); // mark the beginning of frame file read process
    bool                 archiveDirScanned = false;
    vector< ulong_pair > archive_gps;

    // Will have to read archive directory to determine GPS time ranges for each
    // data directory
    if ( mSpec.getArchiveGps( ).size( ) == 0 )
    {
        if ( !scanArchiveDir( &archive_gps ) )
            return false;
        archiveDirScanned = true;
    }
    else
        archive_gps = mSpec.getArchiveGps( );

    vector< ulong_pair > gps = archive_gps;
    unsigned long        start_time = mSpec.getStartGpsTime( );
    unsigned long        end_time = mSpec.getEndGpsTime( );
    sort( gps.begin( ), gps.end( ), cmp2( ) ); // sort gps ranges
    // remove ranges that aren't needed
    gps.erase(
        remove_if( gps.begin( ),
                   gps.end( ),
                   not1( intersect( ulong_pair( start_time, end_time ) ) ) ),
        gps.end( ) );

    system_log( 5, "%d pertinent range(s)", (int)gps.size( ) );
    for ( int i = 0; i < gps.size( ); i++ )
    {
        system_log( 5, "%ld %ld", gps[ i ].first, gps[ i ].second );
    }

    const vector< string >& names = mSpec.getSignalNames( ); // ADC signal names
    unsigned long           num_signals = names.size( );
    unsigned int            nfiles_read = 0; // number of files read
    unsigned int            nfiles_updated = 0; // number of files updated
    unsigned int nfiles_open_failed = 0; // number of files that were missing
    unsigned int nfiles_failed = 0; // number of times read failed
    unsigned int nbad_failures = 0; // tracks the number of expensive failures

    // Buffer for the actual Adc data.
    // It is desirable to write() as large buffer as possible.
    // Data gaps in time are indicated by starting new data blocks (sending new
    // header). Data gaps in channels are currently filled with zeros. Reconfig
    // data change triggers new blocks too.
    daqd_net daqd_net( mDataFd, mSpec );

    // Iterate over gps ranges, ie. over data directories
    for ( int cur_range = 0; cur_range < gps.size( ); cur_range++ )
    {
        // get directory number for the range `i'

        unsigned int dir_num = 0;

        if ( archiveDirScanned )
        {
            // directory number is based on GPS timestamp: all digits but the
            // last 6
            dir_num = gps[ cur_range ].first / 100000;
        }
        else
        {
            dir_num = distance( archive_gps.begin( ),
                                find_first_of( archive_gps.begin( ),
                                               archive_gps.end( ),
                                               gps.begin( ) + cur_range,
                                               gps.begin( ) + cur_range + 1 ) );
        }

        // read file names in directory `i' and parse
        DIR*           dirp;
        struct dirent* direntp;
        char           dirname[ filename_max + 1 ];
        sprintf( dirname, "%s%d", mSpec.getArchiveDir( ).c_str( ), dir_num );
        if ( !( dirp = opendir( dirname ) ) )
        {
            system_log( 1, "Couldn't open directory `%s'", dirname );
            return false;
        }
        char* buf = (char*)malloc( sizeof( struct dirent ) + filename_max + 1 );
        vector< ulong_pair >                         tstamps;
        typedef vector< ulong_pair >::const_iterator VITER;

        int prefix_len = mSpec.getArchivePrefix( ).size( );
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
            char* cfile = direntp->d_name + prefix_len;
            int   times, framedt;
            char  junk[ 1024 ];
            int   scanned = sscanf( cfile, "%d-%d%s", &times, &framedt, junk );
            if ( strcmp( ".gwf", junk ) )
                scanned = 0xff;
            switch ( scanned )
            {
#if 0
      case 1:
	tstamps.push_back(ulong_pair(times, 1));
	break;
#endif
            case 3:
                tstamps.push_back( ulong_pair( times, framedt ) );
                break;
            default:
                system_log( 1,
                            "scan(): `%s' is invalid filename -- skipped",
                            direntp->d_name );
            }
        }
        (void)closedir( dirp );
        free( (void*)buf );

        if ( tstamps.size( ) == 0 )
        {
            system_log( 1,
                        "FATAL: directory %s%d is empty",
                        mSpec.getArchiveDir( ).c_str( ),
                        dir_num );
            return false;
        }

        // sort the timestamps
        sort( tstamps.begin( ), tstamps.end( ), cmp2( ) );

        VITER i =
            find_if( tstamps.begin( ), tstamps.end( ), falls_in( start_time ) );
        if ( i == tstamps.end( ) )
        {
            system_log( 1,
                        "FATAL: data for time %ld not found in directory %s%d",
                        start_time,
                        mSpec.getArchiveDir( ).c_str( ),
                        dir_num );
            return false;
        }

        FrameCPP::Version_8::IFrameStreamPlan* first_reader = 0;
        FrameCPP::Version::FrameH*             new_frame = 0;

        const vector< string >& names =
            mSpec.getSignalNames( ); // ADC signal names we care about
        unsigned int  num_signals = names.size( );
        unsigned long prev_gps = 0;
        // Iterate over the frame files
        for ( ; i != tstamps.end( ) && end_time >= i->first; i++ )
        {
            unsigned long gps, dt;
            gps = i->first;
            dt = i->second;

            DEBUG1( cerr << "gps=" << gps << "; dt=" << dt << endl );
            //
            // Delete the first reader if there is a gap in the frame files
            if ( first_reader && gps != prev_gps + dt )
            {
                delete first_reader;
                first_reader = 0;
            }
            prev_gps = gps;

            char file_name[ filename_max + 1 ];
            if ( dt == 1 )
            {
                sprintf( file_name,
                         "%s%d/%s%ld%s",
                         mSpec.getArchiveDir( ).c_str( ),
                         dir_num,
                         mSpec.getArchivePrefix( ).c_str( ),
                         gps,
                         mSpec.getArchiveSuffix( ).c_str( ) );
            }
            else
            {
                sprintf( file_name,
                         "%s%d/%s%ld-%ld%s",
                         mSpec.getArchiveDir( ).c_str( ),
                         dir_num,
                         mSpec.getArchivePrefix( ).c_str( ),
                         gps,
                         dt,
                         mSpec.getArchiveSuffix( ).c_str( ) );
            }

            // stat file here and see if its size changed
            // delete frame read FrameCPP::Version_8::IFrameStreamPlan, if it
            // changed
            {
                static unsigned long
                            fsize; // this is set to current file size that should match
                struct stat buf;
                if ( stat( file_name, &buf ) )
                {
                    system_log( 3, "%s: frame file stat failed", file_name );
                    nfiles_open_failed++;
                    continue;
                }
                fsize = buf.st_size;
            }

            FrameCPP::Common::FrameBuffer< filebuf >* ibuf =
                new FrameCPP::Common::FrameBuffer< std::filebuf >(
                    std::ios::in );
            ibuf->open( file_name, std::ios::in | std::ios::binary );

            // FIXME: catch std::runtime_error here
            // framecpp-1.19.24/lib/framecpp/src/Common/FrameBuffer.cc source
            // file
#if 0
      if (! in ) {
	system_log(3, "%s: frame file open failed", file_name);
	nfiles_open_failed++;
	continue;
      }
#endif

            DEBUG( 1, cerr << "Begin ReadFrame()" << endl );
            time_t t = time( 0 );

            // FrameCPP::Version::IFrameStream  ifs(ibuf);
            if ( first_reader == 0 )
            {
                first_reader =
                    new FrameCPP::Version_8::IFrameStreamPlan( ibuf, 0 );
                new_frame = new FrameCPP::Version::FrameH(
                    *first_reader->ReadFrameH( 0, 0 ) );
                FrameCPP::Version::FrameH::rawData_type rawData =
                    FrameCPP::Version::FrameH::rawData_type(
                        new FrameCPP::Version::FrRawData );
                new_frame->SetRawData( rawData );

                for ( int i = 0; i < num_signals; i++ )
                {
                    FrameCPP::Version::FrAdcData* adc =
                        first_reader->ReadFrAdcData( 0, names[ i ] ).get( );
                    adc->RefData( )[ 0 ]->Uncompress( );
                    new_frame->GetRawData( )->RefFirstAdc( ).append( *adc );
                    DEBUG( 1,
                           printf( "Added first %s\n",
                                   adc->GetName( ).c_str( ) ) );
                }
            }
            else
            {
                FrameCPP::Version_8::IFrameStreamPlan ifs( ibuf, first_reader );
                for ( int i = 0; i < num_signals; i++ )
                {
                    FrameCPP::Version::FrAdcData* adc =
                        ifs.ReadFrAdcData( 0, names[ i ] ).get( );
                    adc->RefData( )[ 0 ]->Uncompress( );
                    if ( first_reader )
                        new_frame->GetRawData( )->RefFirstAdc( ).erase( 0 );
                    new_frame->GetRawData( )->RefFirstAdc( ).append( *adc );
                    DEBUG( 1,
                           printf( "Added %s\n", adc->GetName( ).c_str( ) ) );
                }
                // Set frame time
                new_frame->SetGTime( FrameCPP::Version::GPSTime( gps, 0 ) );
            }

            t = time( 0 ) - t;
            DEBUG( 1, cerr << "Done in " << t << " seconds" << endl );
            // ibuf->close();

            // decimate and send the data, taking care of DAQD network protocol
            if ( !daqd_net.send_data( *new_frame, file_name, 0, &seq_num ) )
                return false;
        }
    } // data directories
    if ( !daqd_net.finish( ) )
        return false;
    system_log( 1,
                "time=%ld read=%d updated=%d missing=%d failed=%d",
                time( 0 ) - t_start,
                nfiles_read,
                nfiles_updated,
                nfiles_open_failed,
                nfiles_failed );

    return true;
}
