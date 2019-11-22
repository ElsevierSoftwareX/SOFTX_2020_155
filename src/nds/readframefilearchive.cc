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

#include <iostream>
#include <fstream>
#include <list>
#include <set>
#include <map>
#include <string>

#include "nds.hh"
#include "io.h"
#include "daqd_net.hh"

#include "framecpp/daqframe.hh"
#include "framecpp/daqreader.hh"

using namespace CDS_NDS;
using namespace std;

// order predicate
class cmp
{
public:
    int
    operator( )( pair< unsigned long, unsigned long > p1,
                 pair< unsigned long, unsigned long > p2 )
    {
        return p1.first < p2.first;
    }
};

// intersection predicate
class does_not_intersect
{
    pair< unsigned long, unsigned long > p;

public:
    does_not_intersect( const unsigned long gps1, unsigned long gps2 )
        : p( pair< unsigned long, unsigned long >( gps1, gps2 ) )
    {
    }
    bool
    operator( )( const pair< unsigned long, unsigned long >& p1 )
    {
        return !( p1.first <= p.second && p1.second > p.first );
    }
};

static void
error_watch( const string& msg )
{
    system_log( 1, "framecpp error: %s", msg.c_str( ) );
}

// KLUDGE to work over gcc 2.95.3 bug
// FrameCPP::Dictionary *dict = FrameCPP::library.getCurrentVersionDictionary();
// extern FrameCPP::Dictionary *dict;

// read files using DaqReader and update on DaqFrames.
bool
Nds::readFrameFileArchive( )
{

#if 0
  {
    FrameCPP::Dictionary *dict = FrameCPP::library.getCurrentVersionDictionary();
    // cerr << "Current version is " << dict->getVersion() << endl;
  }
#endif

    const vector< pair< unsigned long, unsigned long > >& archive_gps =
        mSpec.getArchiveGps( );
    vector< pair< unsigned long, unsigned long > > gps = archive_gps;
    unsigned long start_time = mSpec.getStartGpsTime( );
    unsigned long end_time = mSpec.getEndGpsTime( );
    sort( gps.begin( ), gps.end( ), cmp( ) ); // sort gps ranges
    // remove ranges that aren't needed
    gps.erase( remove_if( gps.begin( ),
                          gps.end( ),
                          does_not_intersect( start_time, end_time ) ),
               gps.end( ) );

    system_log( 5, "%d pertinent range(s)", gps.size( ) );
    for ( int i = 0; i < gps.size( ); i++ )
    {
        system_log( 5, "%d %d", gps[ i ].first, gps[ i ].second );
    }

    // Full frames are 1 second long and trend frame files are 60 second long
    unsigned int block_period = 0;

    // Adjust trend gps start start mod 60
    // This will allow to address trend frame files, which are all created
    // mod 60
    if ( mSpec.getDataType( ) == Spec::TrendData )
    {
        block_period = 60; // seconds
    }
    else
    {
        block_period = 1; // seconds
    }

    FrameCPP::DaqReader*    daq_reader = 0; // reference frame reader
    FrameCPP::DaqFrame*     daq_frame = 0; // reference frame
    const vector< string >& names = mSpec.getSignalNames( ); // ADC signal names
    const vector< unsigned int >& rates =
        mSpec.getSignalRates( ); // ADC signal rates in hertz
    unsigned int file_rates[ names.size( ) ]; // sampling rate on disk
    bool         decimate[ names.size( ) ]; // which signal to decimate
    const vector< unsigned int >& bps =
        mSpec.getSignalBps( ); // ADC signal bytes per second
    unsigned int nfiles_read = 0; // number of file daqread
    unsigned int nfiles_updated =
        0; // the number of file reads that were actually daqframe update
    unsigned int nfiles_open_failed = 0; // number of files that were missing
    unsigned int nfiles_failed = 0; // number of times daqread failed
    unsigned int nbad_failures = 0; // tracks the number of expensive failures
    const FrameCPP::AdcData*
        adc_ptr[ names.size( ) ]; // pointer to ADC structures in ref. frame

    unsigned long transmission_block_size =
        0; // summary block size for one second of time
    for ( int i = 0; i < bps.size( ); i++ )
        transmission_block_size += bps[ i ] * rates[ i ];
    unsigned long header[ 5 ]; // data transmission block header

    REAL_4 signal_offset[ names.size( ) ];
    REAL_4 signal_slope[ names.size( ) ];
    INT_2U signal_status[ names.size( ) ];
    bool first_time = true; // triggers reconfig block send in the beginning of
                            // the transmission

    for ( int i = 0; i < names.size( ); i++ )
    {
        signal_offset[ i ] = signal_slope[ i ] = .0;
        signal_status[ i ] = 0;
    }

    unsigned long t_start =
        time( 0 ); // mark the beginning of frame file read process

    // Iterate over gps ranges, ie. over data directories
    for ( int i = 0; i < gps.size( ); i++ )
    {
        // get directory number for the range `i'
        unsigned int dir_num =
            distance( archive_gps.begin( ),
                      find_first_of( archive_gps.begin( ),
                                     archive_gps.end( ),
                                     gps.begin( ) + i,
                                     gps.begin( ) + i + 1 ) );
        // Iterate over the frame files
        for ( unsigned long j =
                  max( ( block_period == 60 ? ( start_time - start_time % 60 )
                                            : start_time ),
                       gps[ i ].first );
              j < gps[ i ].second && j <= end_time;
              j += block_period )
        {

            // Abort this request if bad failures persist.
            // It might get too expensive if this was a request to get a day
            // worth of full resolution signal that was not present a minute ago
            //
            if ( nbad_failures > 100 )
            {
                system_log( 1,
                            "too many bad failures (over 100 already); "
                            "FATAL" ) return false;
            }

            char file_name[ 1024 ];
            sprintf( file_name,
                     "%s%d/%s%d%s",
                     mSpec.getArchiveDir( ).c_str( ),
                     dir_num,
                     mSpec.getArchivePrefix( ).c_str( ),
                     j,
                     mSpec.getArchiveSuffix( ).c_str( ) );

            // stat file here and see if its size changed
            // delete reference frame, if it changed
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
                if ( daq_frame && buf.st_size != fsize )
                { // if reference exists and file size changed
                    system_log( 2, "%s: file size changed", file_name );
                    delete ( daq_frame );
                    daq_frame = 0; // will read new reference frame
                }
                fsize = buf.st_size;
            }

            ifstream in( file_name );
            if ( !in )
            {
                system_log( 3, "%s: frame file open failed", file_name );
                nfiles_open_failed++;
                continue;
            }

            // Update template frame with the data from the new file
            if ( daq_frame )
            {
                //  frame update
                try
                {
                    daq_frame->daqUpdate( &in );
                    nfiles_updated++;
                }
                catch ( read_failure )
                {
                    system_log( 2, "%s: update read failure", file_name );
                    nfiles_failed++;
                    continue;
                }
                catch ( cannot_update )
                {
                    DEBUG1( cerr << file_name << ": can't update" );
                    delete ( daq_frame );
                    daq_frame = 0; // will read new reference frame
                }
            }

            // No reference frame exists
            // Need to create daq reader and read reference or template
            // frame in.
            if ( !daq_frame )
            {
                try
                {
                    daq_reader = new FrameCPP::DaqReader( in );
                    daq_reader->setErrorWatch( error_watch );
                }
                catch ( read_failure )
                {
                    system_log(
                        1, "%s: failed to new the DaqReader()", file_name );
                    nfiles_failed++;
                    daq_reader = 0;
                    continue;
                }

                // read reference frame
                assert( daq_reader );
                try
                {
                    daq_frame = daq_reader->readFrame( ); // template frame
                }
                catch ( runtime_error& e )
                {
                    system_log( 1,
                                "%s: failed to read reference frame; %s",
                                file_name,
                                e.what( ) );
                    delete ( daq_reader );
                    delete ( daq_frame );
                    daq_reader = 0;
                    daq_frame = 0;
                    nfiles_failed++;
                    nbad_failures++;
                    continue;
                }

                delete ( daq_reader );
                daq_reader = 0;

                // turn on update for the ADCS in the request
                unsigned int nadc_active = 0;
                daq_frame->getRawData( )
                    ->refAdc( )
                    .rehash( ); // rehash ADC hashtable
                FrameCPP::RawData::AdcDataContainer& adc(
                    daq_frame->getRawData( )->refAdc( ) );
                for ( int k = 0; k < names.size( ); k++ )
                {
                    try
                    {
                        daq_frame->daqTriggerADC( (char*)names[ k ].c_str( ),
                                                  true );
                        FrameCPP::RawData::adcData_iterator iter =
                            adc.find( names[ k ], adc.begin( ) );
                        if ( iter == adc.end( ) )
                            throw;

                        adc_ptr[ k ] = *iter;
                        file_rates[ k ] =
                            (unsigned int)adc_ptr[ k ]->getSampleRate( );
                        if ( ( decimate[ k ] =
                                   ( file_rates[ k ] != rates[ k ] ) ) )
                        {
                            if ( file_rates[ k ] < rates[ k ] )
                            {
                                system_log(
                                    1,
                                    "%s: rate mismatch for signal `%s' (FATAL)",
                                    file_name,
                                    names[ k ].c_str( ) );
                                return false;
                            }
                        }
                        nadc_active++;
                    }
                    catch ( ... )
                    {
                        system_log( 3,
                                    "%s: failed to activate ADC `%s'",
                                    file_name,
                                    names[ k ].c_str( ) );
                        adc_ptr[ k ] = 0;
                    }
                }

                if ( !nadc_active )
                {
                    // Failed to find any ADCs in the file -- go to next one
                    nbad_failures++;
                    system_log( 1,
                                "%s: failed to activate any of %d ADCs",
                                file_name,
                                names.size( ) );
                    delete ( daq_frame );
                    daq_frame = 0;
                    nfiles_failed++;
                    continue; // goto processing next frame file
                }
                nfiles_read++;
            }

            // Take care of the reconfiguration block
            bool config_changed = false;
            for ( int k = 0; k < names.size( ); k++ )
            {
                if ( signal_offset[ k ] != adc_ptr[ k ]->getBias( ) ||
                     signal_slope[ k ] != adc_ptr[ k ]->getSlope( ) ||
                     signal_status[ k ] != adc_ptr[ k ]->getDataValid( ) )
                {
                    config_changed = true;
                    signal_offset[ k ] = adc_ptr[ k ]->getBias( );
                    signal_slope[ k ] = adc_ptr[ k ]->getSlope( );
                    signal_status[ k ] = adc_ptr[ k ]->getDataValid( );
                }
            }

            if ( first_time || config_changed )
            {
                first_time = false;
                int           written;
                unsigned long rh[ 5 ];
                rh[ 0 ] = htonl( 4 * sizeof( unsigned long ) +
                                 names.size( ) *
                                     ( 2 * sizeof( REAL_4 ) + sizeof( int ) ) );
                rh[ 1 ] = rh[ 2 ] = rh[ 3 ] = rh[ 4 ] = htonl( 0xffffffff );

                written = basic_io::writen( mDataFd, rh, sizeof( rh ) );
                if ( written != sizeof( rh ) )
                {
                    system_log(
                        1,
                        "failed to send reconfig header; errno=%d (FATAL)",
                        errno );
                    return false;
                }
                for ( int k = 0; k < names.size( ); k++ )
                {
                    written = basic_io::writen(
                        mDataFd, (char*)&signal_offset[ k ], sizeof( REAL_4 ) );
                    if ( written != sizeof( REAL_4 ) )
                    {
                        system_log(
                            1,
                            "failed to send reconfig data; errno=%d (FATAL)",
                            errno );
                        return false;
                    }
                    written = basic_io::writen(
                        mDataFd, (char*)&signal_slope[ k ], sizeof( REAL_4 ) );
                    if ( written != sizeof( REAL_4 ) )
                    {
                        system_log(
                            1,
                            "failed to send reconfig data; errno=%d (FATAL)",
                            errno );
                        return false;
                    }
                    unsigned int status = (int)signal_status[ k ];
                    written = basic_io::writen(
                        mDataFd, (char*)&status, sizeof( int ) );
                    if ( written != sizeof( int ) )
                    {
                        system_log(
                            1,
                            "failed to send reconfig data; errno=%d (FATAL)",
                            errno );
                        return false;
                    }
                }
            }

            unsigned seconds_to_send = block_period;
            unsigned start_diff =
                0; // shift from the start of data in file (in seconds)

            // Incomplete 60 second blocks handling (first and last)
            if ( block_period == 60 )
            {
                if ( j < start_time )
                { // first transmission block is incomplete 60 seconds
                    start_diff = start_time - j;
                    if ( j + block_period > end_time )
                        seconds_to_send = end_time - start_time + 1;
                    else
                        seconds_to_send = 60 - start_diff;
                }
                else if ( j + block_period > end_time )
                { // last data transmission block could be truncated, if
                  // end_time says it must be
                    seconds_to_send = end_time - j + 1;
                }
            }

            header[ 0 ] =
                htonl( 4 * sizeof( unsigned long ) +
                       transmission_block_size * seconds_to_send ); // len
            header[ 1 ] = htonl( seconds_to_send ); // block time interval
            header[ 2 ] = htonl( daq_frame->getGTime( ).getSec( ) +
                                 start_diff ); // gps time has to be eq. to `j'
            header[ 3 ] = htonl(
                daq_frame->getGTime( )
                    .getNSec( ) ); // this must be zero the way DAQ system works
            header[ 4 ] = htonl( seq_num++ );

            // FIXME: need to buffer data in memory if block size is too small
            // and request period is comparatively long. This will eliminate
            // many write() system calls that slow the process down.

            int written = basic_io::writen( mDataFd, header, sizeof( header ) );
            if ( written != sizeof( header ) )
            {
                system_log(
                    1, "failed to send data header; errno=%d (FATAL)", errno );
                return false;
            }

            bool average = ( mSpec.getFilter( ) == "average" );

            // actually get the data and send to the client here
            for ( int k = 0; k < names.size( ); k++ )
            {
                int   written;
                int   s = bps[ k ] * rates[ k ] * seconds_to_send;
                char* sptr =
                    ( (char*)adc_ptr[ k ]->refData( )[ 0 ]->getData( ) ) +
                    start_diff * bps[ k ] * rates[ k ];

                if ( adc_ptr )
                {
                    if ( decimate[ k ] )
                    {
                        int samples_per_point = file_rates[ k ] / rates[ k ];
                        if ( average )
                        {
                            DEBUG( 1,
                                   cerr << "averaging, decimating "
                                        << names[ k ] << " x"
                                        << samples_per_point << endl );
                            if ( bps[ k ] == 2 )
                            {
                                for ( int l = 0; l < rates[ k ]; l++ )
                                    ( (short*)sptr )[ l ] = daqd_net::averaging(
                                        (short*)sptr + l * samples_per_point,
                                        samples_per_point );
                            }
                            else if ( bps[ k ] == 4 )
                            {
                                for ( int l = 0; l < rates[ k ]; l++ )
                                    ( (float*)sptr )[ l ] = daqd_net::averaging(
                                        (float*)sptr + l * samples_per_point,
                                        samples_per_point );
                            }
                            else if ( bps[ k ] == 8 )
                            {
                                for ( int l = 0; l < rates[ k ]; l++ )
                                    ( (double*)sptr )[ l ] =
                                        daqd_net::averaging(
                                            (double*)sptr +
                                                l * samples_per_point,
                                            samples_per_point );
                            }
                            else
                            {
                                abort( );
                            }
                        }
                        else
                        {
                            DEBUG( 1,
                                   cerr << "nofilter, decimating " << names[ k ]
                                        << " x" << samples_per_point << endl );
                            if ( bps[ k ] == 2 )
                            {
                                for ( int l = 0; l < rates[ k ]; l++ )
                                    ( (short*)sptr )[ l ] = ( (
                                        short*)sptr )[ l * samples_per_point ];
                            }
                            else if ( bps[ k ] == 4 )
                            {
                                for ( int l = 0; l < rates[ k ]; l++ )
                                    ( (float*)sptr )[ l ] = ( (
                                        float*)sptr )[ l * samples_per_point ];
                            }
                            else if ( bps[ k ] == 8 )
                            {
                                for ( int l = 0; l < rates[ k ]; l++ )
                                    ( (double*)sptr )[ l ] = ( (
                                        double*)sptr )[ l * samples_per_point ];
                            }
                            else
                            {
                                abort( );
                            }
                        }
                    }
                    written = basic_io::writen( mDataFd, sptr, s );
                }
                else
                {
                    // send zeros
                    char junk[ s ];
                    memset( junk, 0, s );
                    written = basic_io::writen( mDataFd, junk, s );
                }
                if ( written != s )
                {
                    system_log(
                        1, "failed to send data; errno=%d (FATAL)", errno );
                    return false;
                }
            }
        } // frame files
    } // data directories

    system_log( 1,
                "time=%d read=%d updated=%d missing=%d failed=%d",
                time( 0 ) - t_start,
                nfiles_read,
                nfiles_updated,
                nfiles_open_failed,
                nfiles_failed );
    return true;
}
