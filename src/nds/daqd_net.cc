#include <iostream>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <arpa/inet.h>

#include "debug.h"
#include "io.h"
#include "daqd_net.hh"

using namespace CDS_NDS;
using namespace std;

daqd_net::daqd_net( int fd, Spec& spec )
    : ndata( 0 ), buf( 0 ), num_signals( spec.getSignalNames( ).size( ) ),
      first_time( true ), mDataFd( fd ), mSpec( spec ),
      transmission_block_size( 0 ), subjobReadStateMap( ), seq_num( 0 ),
      mSignalBlockOffsets( )
{
#if 0
  buf = malloc (buf_size);
  if (! buf) {
    system_log(1, "data buffer malloc(%d) failed", buf_size);
    abort ();
  }
#endif

    int rsize = sizeof( reconfig_data_t ) * num_signals;
    reconfig_data = (reconfig_data_t*)malloc( rsize );
    if ( !reconfig_data )
    {
        system_log( 1, "malloc(%d) failed", rsize );
        abort( );
    }
    for ( int i = 0; i < num_signals; i++ )
    {
        reconfig_data[ i ].signal_offset = reconfig_data[ i ].signal_slope = .0;
        reconfig_data[ i ].signal_status = 0;
    }

    const vector< unsigned int >& bps = mSpec.getSignalBps( );
    const vector< unsigned int >& rates = mSpec.getSignalRates( );
    for ( int i = 0; i < num_signals; i++ )
        transmission_block_size += bps[ i ] * rates[ i ];
}

// Initialize for the concurrent combination processing.
daqd_net::daqd_net( int fd, Spec& spec, CPT& v )
    : ndata( 0 ), buf( 0 ), num_signals( spec.getSignalNames( ).size( ) ),
      first_time( true ), mDataFd( fd ), mSpec( spec ),
      transmission_block_size( 0 ), subjobReadStateMap( ), seq_num( 0 ),
      mSignalBlockOffsets( )
{
#if 0
  buf = malloc (buf_size);
  if (! buf) {
    system_log(1, "data buffer malloc(%d) failed", buf_size);
    abort ();
  }
#endif

    int rsize = sizeof( reconfig_data_t ) * num_signals;
    reconfig_data = (reconfig_data_t*)malloc( rsize );
    if ( !reconfig_data )
    {
        system_log( 1, "malloc(%d) failed", rsize );
        abort( );
    }
    for ( int i = 0; i < num_signals; i++ )
    {
        reconfig_data[ i ].signal_offset = reconfig_data[ i ].signal_slope = .0;
        reconfig_data[ i ].signal_status = 0;
    }

    const vector< unsigned int >& bps = mSpec.getSignalBps( );
    const vector< unsigned int >& rates = mSpec.getSignalRates( );
    for ( int i = 0; i < num_signals; i++ )
        transmission_block_size += bps[ i ] * rates[ i ];

#ifndef NDEBUG
    cerr << "Daqd_net Combination Processing Map:" << endl;
    for ( CPMI p = v.begin( ); p != v.end( ); p++ )
    {
        cerr << p->first << "->";
        for ( int i = 0; i < p->second.size( ); i++ )
            cerr << p->second[ i ] << " ";
        cerr << endl;
    }
#endif

    // Initialize subjob state map and the offset vector
    for ( CPMI p = v.begin( ); p != v.end( ); p++ )
    {
        subjobReadStateMap.insert( std::pair< int, SubjobReadState >(
            p->first, SubjobReadState( p->second ) ) );
    }

    unsigned long cur_offs = 0;
    for ( int i = 0; i < mSpec.getSignalBps( ).size( ); i++ )
    {
        mSignalBlockOffsets.push_back( cur_offs );
        cur_offs += mSpec.getSignalBps( )[ i ];
    }
}

daqd_net::~daqd_net( )
{
    //  free(buf);
    free( reconfig_data );
}

bool
daqd_net::send_reconfig_data( Spec& spec )
{
    for ( int i = 0; i < spec.getSignalNames( ).size( ); i++ )
    {
        reconfig_data[ i ].signal_offset = spec.getSignalOffsets( )[ i ];
        reconfig_data[ i ].signal_slope = spec.getSignalSlopes( )[ i ];
        reconfig_data[ i ].signal_status = 0;
    }
    return send_reconfig_block( );
}

bool
daqd_net::send_reconfig_block( )
{
    int          written;
    unsigned int rh[ 5 ];
    rh[ 0 ] = htonl( 4 * sizeof( unsigned int ) +
                     num_signals * ( 2 * sizeof( REAL_4 ) + sizeof( int ) ) );
    rh[ 1 ] = rh[ 2 ] = rh[ 3 ] = rh[ 4 ] = htonl( 0xffffffff );

    written = basic_io::writen( mDataFd, rh, sizeof( rh ) );
    if ( written != sizeof( rh ) )
    {
        system_log(
            1, "failed to send reconfig header; errno=%d (FATAL)", errno );
        return false;
    }
    for ( int k = 0; k < num_signals; k++ )
    {
        written = basic_io::writen( mDataFd,
                                    (char*)&reconfig_data[ k ].signal_offset,
                                    sizeof( REAL_4 ) );
        if ( written != sizeof( REAL_4 ) )
        {
            system_log(
                1, "failed to send reconfig data; errno=%d (FATAL)", errno );
            return false;
        }
        written = basic_io::writen( mDataFd,
                                    (char*)&reconfig_data[ k ].signal_slope,
                                    sizeof( REAL_4 ) );
        if ( written != sizeof( REAL_4 ) )
        {
            system_log(
                1, "failed to send reconfig data; errno=%d (FATAL)", errno );
            return false;
        }
        unsigned int status = (int)reconfig_data[ k ].signal_status;
        written = basic_io::writen( mDataFd, (char*)&status, sizeof( int ) );
        if ( written != sizeof( int ) )
        {
            system_log(
                1, "failed to send reconfig data; errno=%d (FATAL)", errno );
            return false;
        }
    }
    DEBUG1( cerr << "sent reconfig " << endl );
    return true;
}

bool
daqd_net::combine_send_data( )
{
    for ( ;; )
    {
        int nfinished =
            0; // The number of finished subjobs with and empty block lists
        bool reconfigBlockSeen = false;

        for ( CRMINC p = subjobReadStateMap.begin( );
              p != subjobReadStateMap.end( );
              p++ )
        {
            // Wait until received at least single block from all subjobs
            if ( p->second.block_list.empty( ) )
            {
                if ( p->second.finished )
                    nfinished++;
                else
                    return true;
            }
            else
            {
                if ( is_reconfig_block( p->second.block_list.front( ).data ) )
                    reconfigBlockSeen = true;
            }
            // :TODO: pop zero length blocks?
        }

        // If all subjobs are done we are done
        if ( nfinished == subjobReadStateMap.size( ) )
            return true;

        // If there is at least one reconfig block then need to send output
        // reconfig block
        if ( reconfigBlockSeen )
        {
            // Pop all configuration blocks and record the values
            for ( CRMINC p = subjobReadStateMap.begin( );
                  p != subjobReadStateMap.end( );
                  p++ )
            {
                if ( p->second.finished && p->second.block_list.empty( ) )
                    continue; // Skip finished jobs
                if ( is_reconfig_block( p->second.block_list.front( ).data ) )
                {
                    SubjobReadState::BT block = p->second.block_list.front( );
                    unsigned int        block_len =
                        block.length - 4 * sizeof( unsigned int );
                    char* data = block.data +
                        4 *
                            sizeof(
                                unsigned int ); // Skip remainder of the header
                    if ( p->second.signalIndices.size( ) *
                             sizeof( reconfig_data_t ) !=
                         block_len )
                    {
                        system_log( 1,
                                    "bad config block length; expected %d; "
                                    "received %d; fd=%d",
                                    (int)( p->second.signalIndices.size( ) *
                                           sizeof( reconfig_data_t ) ),
                                    block_len,
                                    p->first );
                        return false;
                    }
                    else
                    {

                        // Record the values
                        reconfig_data_t r;
                        for ( int i = 0; i < p->second.signalIndices.size( );
                              i++ )
                        {
                            memcpy( reconfig_data +
                                        p->second.signalIndices[ i ],
                                    data + i * sizeof( reconfig_data_t ),
                                    sizeof( reconfig_data_t ) );
                        }
                    }
                    free( block.data );
                    p->second.block_list.pop_front( );
                }
            }
            send_reconfig_block( );
            continue;
        }
        else
        {
            unsigned long start_t = 0xffffffff;
            unsigned long end_t = 0xffffffff;

            // Find minimum start and end times for the blocks in the front of
            // the receive queues
            for ( CRMINC p = subjobReadStateMap.begin( );
                  p != subjobReadStateMap.end( );
                  p++ )
            {
                if ( p->second.finished && p->second.block_list.empty( ) )
                    continue; // Skip finished jobs
                SubjobReadState::BT block = p->second.block_list.front( );
                unsigned int        block_len = block.length;
                char*               data = block.data;
                if ( block_len < 4 * sizeof( int ) )
                {
                    system_log(
                        1, "bad block length %d; fd=%d", block_len, p->first );
                    return false;
                }
                unsigned long dt, gps, gpsn;
                memcpy( &dt, data, sizeof( dt ) );
                memcpy( &gps, data + 4, sizeof( gps ) );
                memcpy( &gpsn, data + 8, sizeof( gpsn ) );
                if ( gpsn )
                {
                    system_log( 1, "gpsn!=0 is not supported by the merger" );
                    return false;
                }
                if ( gps + block.dt < start_t )
                    start_t = gps + block.dt;
                if ( gps + dt < end_t )
                    end_t = gps + dt;
            }
            DEBUG1( cerr << "start_t=" << start_t << "; end_t=" << end_t
                         << endl );

            // Length of the transmission block
            unsigned long seconds_to_send = end_t - start_t;
            unsigned long image_size =
                transmission_block_size * seconds_to_send / 60 +
                5 * sizeof( unsigned int );
            char* image = (char*)malloc( image_size );
            if ( !image )
            {
                system_log( 1, "malloc(%ld) failed", image_size );
                return false;
            }
            memset( image, 0, image_size );
            DEBUG1( cerr << "imageptr=" << hex << (unsigned long)image
                         << "; image_size=" << dec << image_size << endl );

            // Write header out to the client
            unsigned int* header = (unsigned int*)image;
            header[ 0 ] = htonl( image_size - sizeof( int ) ); // len
            header[ 1 ] = htonl( seconds_to_send ); // block time interval
            header[ 2 ] = htonl( start_t );
            header[ 3 ] = htonl( 0 );
            header[ 4 ] = htonl( seq_num++ );

            seconds_to_send /= 60;
            char* iptr = image + 5 * sizeof( unsigned int );

            // For each block: write the data to the image, write zeros or split
            // blocks
            for ( CRMINC p = subjobReadStateMap.begin( );
                  p != subjobReadStateMap.end( );
                  p++ )
            {
                if ( p->second.finished && p->second.block_list.empty( ) )
                    continue; // Skip finished jobs
                SubjobReadState::BT* block = &p->second.block_list.front( );

                DEBUG1( block->print_debug_info( ) );

                unsigned long block_len = block->length;
                char*         data = block->data;
                unsigned long o_dt =
                    block->dt; // Data output offset ('block->dt' worth of data
                               // was sent out already)
                unsigned long dt, gps;
                memcpy( &dt, data, sizeof( dt ) );
                memcpy( &gps, data + 4, sizeof( gps ) );
                data += 4 * sizeof( unsigned int ); // Skip the header vars to
                                                    // the start of data
                unsigned long i_end_t = gps + dt; // Input block's end time

                if ( gps + o_dt < end_t )
                { // This block has some data we need
                    // Minus data already sent
                    dt -= o_dt;

                    if ( i_end_t > end_t )
                    { // Partial block
                        // Subtract data length not fitting into the block
                        dt = dt - ( i_end_t - end_t ); // This many seconds fits
                                                       // into the current block

                        // Add length of data we are going to send now
                        block->dt += dt;
                    }
                    dt /= 60; // 60 seconds per data point
                    unsigned long inb_offs =
                        0; // offset into the input block's to the start of this
                           // signal's memory
                    for ( int i = 0; i < p->second.signalIndices.size( ); i++ )
                    {
                        unsigned long signal_offset = mSignalBlockOffsets
                            [ p->second.signalIndices[ i ] ]; // Signal offset
                                                              // in the output
                                                              // block
                        unsigned long signal_bps =
                            mSpec.getSignalBps( )[ p->second
                                                       .signalIndices[ i ] ];
                        unsigned long davail =
                            dt * signal_bps; // This much data is available in
                                             // the input block
                        unsigned long ins_offs =
                            o_dt / 60 * signal_bps; // offset in the input
                                                    // block's signal memory
                        unsigned long soffs = ( gps + o_dt - start_t ) / 60 *
                            signal_bps; // offset in the output block's signal
                                        // memory
                        unsigned long boffs = ( end_t - start_t ) / 60 *
                            signal_offset; // offset in the output block' to the
                                           // start of this signal's memory

                        DEBUG1( cerr << "memcpy(boffs=" << boffs << ";soffs="
                                     << soffs << ", inb_offs=" << inb_offs
                                     << ";ins_offs=" << ins_offs << ","
                                     << davail << ");" << endl );
                        memcpy( iptr + boffs + soffs,
                                data + inb_offs + ins_offs,
                                davail );
                        inb_offs += ( i_end_t - gps ) / 60 *
                            signal_bps; // Move to the next signal in the subjob
                                        // block (input block)
                    }

                    if ( i_end_t <= end_t )
                    { // All of this block's data fits into the output
                        // Delete the block from the queue (list).
                        p->second.block_list.pop_front( );
                        free( data );
                    }
                }
            }

            int written = basic_io::writen( mDataFd, image, image_size );
            free( image );
            if ( written != image_size )
            {
                system_log(
                    1, "failed to send data header; errno=%d (FATAL)", errno );
                return false;
            }
        }
    }

#if 0

  // Send all blocks as is, not doing any combination processing
  for (CRMINC p = subjobReadStateMap.begin (); p != subjobReadStateMap.end (); p++) {
    std::list<std::pair<unsigned long, char *> > &l = p -> second.block_list;
    for (SubjobReadState::BLINC p = l.begin (); p != l.end (); p++) {
      unsigned long len =  htonl (p -> first);
      int written = basic_io::writen(mDataFd, &len, sizeof(len));
      if (written != sizeof(len)) {
	system_log(1, "failed to send data block length; errno=%d (FATAL)", errno);
	return false;
      }
      
      written = basic_io::writen(mDataFd, p -> second,  p -> first);
      if (written != p -> first) {
	system_log(1, "failed to send data; errno=%d (FATAL)", errno);
	return false;
      }
      free(p -> second);
      l.erase(p); // erase sent block from the list
    }
  }

#endif

    return true;
}

// Read one data block from 'fildes' and do the combination processing
bool
daqd_net::comb_read( int fildes )
{
    CRMINC iter = subjobReadStateMap.find( fildes );
    if ( iter == subjobReadStateMap.end( ) )
    {
        system_log(
            1, "daqd_net::comb_read() unknown fildes passed: %d", fildes );
        return false;
    }

    long block_len;
    long seconds;

    if ( !( block_len = read_long( fildes ) ) ) // Read data block byte length
        return false;

    char* block = (char*)malloc( block_len );
    if ( !block )
    {
        system_log( 1, "malloc(%ld) failed", block_len );
        return false;
    }

    int nread = read_bytes( fildes, block, block_len );
    if ( nread != block_len )
        return false;

    // Put new block into the subjobs linked list of blocks
    iter->second.add_block( block_len, block );

    DEBUG1( cerr << "read one block from desc " << fildes << endl );

    return combine_send_data( );
}

// Finish one subjob processing
bool
daqd_net::comb_subjob_done( int fildes, int seq_num )
{
    DEBUG1( cerr << "combination processing finished with desc " << fildes
                 << endl );
    CRMINC iter = subjobReadStateMap.find( fildes );
    if ( iter == subjobReadStateMap.end( ) )
    {
        system_log(
            1, "daqd_net::comb_read() unknown fildes passed: %d", fildes );
        return false;
    }
    iter->second.seq_num = seq_num;
    iter->second.finished = true;
    return combine_send_data( );
}

/* Read 4 byte `long' */
unsigned long
daqd_net::read_long( int fd )
{
    int  bread;
    int  oread;
    char buf[ 4 ];

    for ( bread = oread = 0; oread < 4; oread += bread )
    {
        if ( ( bread = read( fd, buf + oread, 4 - oread ) ) < 0 )
        {
            system_log( 1, "read() failed; errno=%d (FATAL)", errno );
            return 0;
        }
        if ( !bread ) /* return zero on EOF -- might be not appropriate for all
                         usage cases */
            return 0;
    }
    return ntohl( *(unsigned long*)buf );
}

/*
   Read `numb' bytes.
   Returns number of bytes read or 0 on the error or EOF
*/
int
daqd_net::read_bytes( int fd, char* cptr, int numb )
{
    int bread;
    int oread;

    for ( bread = oread = 0; oread < numb; oread += bread )
    {
        if ( ( bread = read( fd, cptr + oread, numb - oread ) ) < 0 )
        {
            system_log( 1, "read_bytes failed; errno=%d (FATAL)", errno );
            return 0;
        }
        if ( !bread )
            break;
    }
    return oread;
}

inline double
htond( double in )
{
    double retVal;
    char*  p = (char*)&retVal;
    char*  i = (char*)&in;
    p[ 0 ] = i[ 7 ];
    p[ 1 ] = i[ 6 ];
    p[ 2 ] = i[ 5 ];
    p[ 3 ] = i[ 4 ];

    p[ 4 ] = i[ 3 ];
    p[ 5 ] = i[ 2 ];
    p[ 6 ] = i[ 1 ];
    p[ 7 ] = i[ 0 ];

    return retVal;
}

bool
daqd_net::send_data( FrameCPP::Version::FrameH& frame,
                     const char*                file_name,
                     unsigned                   frame_number,
                     int*                       seq_num )
{
    FrameCPP::Version::FrRawData::firstAdc_type adc(
        frame.GetRawData( )->RefFirstAdc( ) );

    // Take care of the reconfiguration block
    bool config_changed = false;
    for ( int k = 0; k < num_signals; k++ )
    {
        if ( reconfig_data[ k ].signal_offset != adc[ k ]->GetBias( ) ||
             reconfig_data[ k ].signal_slope != adc[ k ]->GetSlope( ) ||
             reconfig_data[ k ].signal_status != adc[ k ]->GetDataValid( ) )
        {
            config_changed = true;
            reconfig_data[ k ].signal_offset = adc[ k ]->GetBias( );
            reconfig_data[ k ].signal_slope = adc[ k ]->GetSlope( );
            reconfig_data[ k ].signal_status = adc[ k ]->GetDataValid( );
        }
    }

    if ( first_time || config_changed )
    {
        first_time = false;
        if ( !send_reconfig_block( ) )
            return false;
    }

    // Send actual data taking care of the decimation if needed
    DEBUG1( cerr << "sending data..." << endl );

    // Determine start time differential and how many seconds should be sent
    //
    // Shift from the start of data in frame (in seconds or in minutes for the
    // minute trend data)
    unsigned start_diff = 0;
    unsigned dt = (unsigned)frame.GetDt( );
    // How many seconds (or minutes for minute trend data) of data to send
    unsigned                          seconds_to_send = dt;
    const FrameCPP::Version::GPSTime& gps = frame.GetGTime( );
    unsigned long                     start_time = mSpec.getStartGpsTime( );
    unsigned long                     end_time = mSpec.getEndGpsTime( );
    if ( dt > 1 )
    {
        if ( gps.getSec( ) < start_time )
        { // first transmission block is incomplete
            start_diff = start_time - gps.getSec( );
        }
        if ( gps.getSec( ) + dt > end_time )
        { // last data transmission block could be truncated, if end_time says
          // it must be
            seconds_to_send = end_time - gps.getSec( ) + 1;
        }
        seconds_to_send -= start_diff;
    }
    else if ( dt < 1 )
    {
        system_log( 1, "%s: frame %d has dt=%d", file_name, frame_number, dt );
        return false;
    }

    DEBUG1( cerr << "start_diff=" << start_diff
                 << " seconds_to_send=" << seconds_to_send << endl );

    if ( mSpec.getDataType( ) == Spec::MinuteTrendData )
    {
        // devide all times by sixty for the minute trend data (at 1/60 Hz)
        seconds_to_send += start_diff %
            60; // Add the remainder of the difference to the total time
        start_diff /= 60;
        if ( seconds_to_send % 60 )
            seconds_to_send += 60 - seconds_to_send % 60;
        seconds_to_send /= 60;
        DEBUG1( cerr << "Adjusted: start_diff=" << start_diff
                     << " seconds_to_send=" << seconds_to_send << endl );
    }

    unsigned int header[ 5 ]; // data transmission block header
    header[ 0 ] = htonl( 4 * sizeof( unsigned int ) +
                         transmission_block_size * seconds_to_send ); // len
    if ( mSpec.getDataType( ) == Spec::MinuteTrendData )
    {
        header[ 1 ] = htonl( 60 * seconds_to_send ); // block time interval
        header[ 2 ] = htonl( gps.getSec( ) +
                             60 * start_diff ); // gps time has to be eq. to `j'
    }
    else
    {
        header[ 1 ] = htonl( seconds_to_send ); // block time interval
        header[ 2 ] = htonl( gps.getSec( ) +
                             start_diff ); // gps time has to be eq. to `j'
    }
    header[ 3 ] = htonl(
        gps.getNSec( ) ); // this must be zero the way our DAQ system works
    header[ 4 ] = htonl( *seq_num );
    ( *seq_num )++;

    int written = basic_io::writen( mDataFd, header, sizeof( header ) );
    if ( written != sizeof( header ) )
    {
        system_log( 1, "failed to send data header; errno=%d (FATAL)", errno );
        return false;
    }

    const vector< unsigned int >& bps = mSpec.getSignalBps( );
    const vector< unsigned int >& rates = mSpec.getSignalRates( );
    bool                          average = ( mSpec.getFilter( ) == "average" );

    // actually get the data and send to the client here
    for ( int k = 0; k < num_signals; k++ )
    {
        int written;
        int s = bps[ k ] * rates[ k ] * seconds_to_send;

        // :TODO: support 32 bit integer data type -- required changes to the
        // frmebuilder too.

        // Check data type
        unsigned int frameDataType =
            (unsigned int)adc[ k ]->RefData( )[ 0 ]->GetType( );
        unsigned int file_rate = (unsigned int)adc[ k ]->GetSampleRate( );

        switch ( frameDataType )
        {
        case FrameCPP::Version::FrVect::FR_VECT_2S:
        case FrameCPP::Version::FrVect::FR_VECT_4S:
        case FrameCPP::Version::FrVect::FR_VECT_2U:
        case FrameCPP::Version::FrVect::FR_VECT_4R:
        case FrameCPP::Version::FrVect::FR_VECT_8R:
            break;
        default:
            if ( file_rate != rates[ k ] )
            {
                system_log( 1,
                            "%s: frame %d has an ADC `%s' of unsupported "
                            "framecpp data type %d",
                            file_name,
                            frame_number,
                            adc[ k ]->GetName( ).c_str( ),
                            frameDataType );
                return false;
            }
            else
                break;
        }

        // See if conversion is required and if so do it in place
        if ( mSpec.getSignalTypes( )[ k ] == Spec::_16bit_integer )
        {
            if ( frameDataType == FrameCPP::Version::FrVect::FR_VECT_4R )
            {
                // Convert floats to shorts
                short* sptr =
                    (short*)adc[ k ]->RefData( )[ 0 ]->GetData( ).get( );
                float* fptr =
                    (float*)adc[ k ]->RefData( )[ 0 ]->GetData( ).get( );
                for ( int l = 0; l < file_rate * seconds_to_send; l++ )
                    *sptr++ = (short)*fptr++;
            }
            else if ( frameDataType == FrameCPP::Version::FrVect::FR_VECT_8R )
            {
                // Convert doubles to shorts
                // Convert floats to shorts
                short* sptr =
                    (short*)adc[ k ]->RefData( )[ 0 ]->GetData( ).get( );
                double* dptr =
                    (double*)adc[ k ]->RefData( )[ 0 ]->GetData( ).get( );
                for ( int l = 0; l < file_rate * seconds_to_send; l++ )
                    *sptr++ = (short)*dptr++;
            }
        }
        else if ( mSpec.getSignalTypes( )[ k ] == Spec::_32bit_float )
        {
            if ( frameDataType == FrameCPP::Version::FrVect::FR_VECT_8R )
            {
                // Convert doubles to floats
                float* fptr =
                    (float*)adc[ k ]->RefData( )[ 0 ]->GetData( ).get( );
                double* dptr =
                    (double*)adc[ k ]->RefData( )[ 0 ]->GetData( ).get( );
                for ( int l = 0; l < file_rate * seconds_to_send; l++ )
                    *fptr++ = (float)*dptr++;
            }
        }

        char* sptr = ( (char*)adc[ k ]->RefData( )[ 0 ]->GetData( ).get( ) ) +
            start_diff * bps[ k ] * rates[ k ];

        if ( mSpec.getDataType( ) != Spec::MinuteTrendData )
        {
            if ( file_rate != rates[ k ] )
            {
                int samples_per_point = file_rate / rates[ k ];
                DEBUG( 1,
                       cerr << "decimating " << adc[ k ]->GetName( )
                            << " file_rate=" << file_rate
                            << " rate=" << rates[ k ] << "samples_per_point="
                            << samples_per_point << endl );

                if ( average )
                {
                    DEBUG( 1,
                           cerr << "averaging" << samples_per_point << endl );
                    if ( bps[ k ] == 2 )
                    {
                        for ( int l = 0; l < rates[ k ] * seconds_to_send; l++ )
                            ( (short*)sptr )[ l ] =
                                averaging( (short*)sptr + l * samples_per_point,
                                           samples_per_point );
                    }
                    else if ( bps[ k ] == 4 )
                    {
                        for ( int l = 0; l < rates[ k ] * seconds_to_send; l++ )
                            ( (float*)sptr )[ l ] =
                                averaging( (float*)sptr + l * samples_per_point,
                                           samples_per_point );
                    }
                    else if ( bps[ k ] == 8 )
                    {
                        for ( int l = 0; l < rates[ k ] * seconds_to_send; l++ )
                            ( (double*)sptr )[ l ] = averaging(
                                (double*)sptr + l * samples_per_point,
                                samples_per_point );
                    }
                    else
                    {
                        abort( );
                    }
                }
                else
                {
                    DEBUG( 1, cerr << "nofilter" << samples_per_point << endl );
                    if ( bps[ k ] == 2 )
                    {
                        for ( int l = 0; l < rates[ k ] * seconds_to_send; l++ )
                            ( (short*)sptr )[ l ] =
                                ( (short*)sptr )[ l * samples_per_point ];
                    }
                    else if ( bps[ k ] == 4 )
                    {
                        for ( int l = 0; l < rates[ k ] * seconds_to_send; l++ )
                            ( (float*)sptr )[ l ] =
                                ( (float*)sptr )[ l * samples_per_point ];
                    }
                    else if ( bps[ k ] == 8 )
                    {
                        for ( int l = 0; l < rates[ k ] * seconds_to_send; l++ )
                            ( (double*)sptr )[ l ] =
                                ( (double*)sptr )[ l * samples_per_point ];
                    }
                    else
                    {
                        abort( );
                    }
                }
            }
        }
#ifdef __linux__
        if ( bps[ k ] == 2 )
        {
            for ( int j = 0; j < s / 2; j++ )
                ( (unsigned short*)sptr )[ j ] =
                    htons( ( (unsigned short*)sptr )[ j ] );
        }
        else if ( bps[ k ] == 4 )
        {
            for ( int j = 0; j < s / 4; j++ )
                ( (unsigned int*)sptr )[ j ] =
                    htonl( ( (unsigned int*)sptr )[ j ] );
        }
        else if ( bps[ k ] == 8 )
        {
            for ( int j = 0; j < s / 8; j++ )
                ( (double*)sptr )[ j ] = htond( ( (double*)sptr )[ j ] );
        }
#endif
        DEBUG1( cerr << "before doing writen(s=" << s << ")" << endl );

        written = basic_io::writen( mDataFd, sptr, s );
        if ( written != s )
        {
            system_log( 1, "failed to send data; errno=%d (FATAL)", errno );
            return false;
        }
    }

    return true;
}

bool
daqd_net::finish( )
{
    //  cerr << "sending " << ndata << " bytes out" << endl;
    return true;
}
