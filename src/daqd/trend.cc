#include <config.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <fstream>

#include "framecpp/Common/MD5SumFilter.hh"

using namespace std;

#include "circ.hh"
#include "daqd.hh"
#include "sing_list.hh"

#include "epics_pvs.hh"

extern daqd_c daqd;

#include "crc8.cc"
#include "raii.hh"

/* Define duration of trend quantities */
#define SECPERMIN 60 // # of seconds per minute
#define SECPERHOUR 3600 // # of seconds per hour
#define MINPERHOUR 60 // # of minutes per hour

// saves striped data
void*
trender_c::raw_minute_saver( )
{
    const int STATE_NORMAL = 0;
    const int STATE_WRITING = 1;

    // Set thread parameters
    daqd_c::set_thread_priority(
        "Raw minute trend saver", "dqmtraw", SAVER_THREAD_PRIORITY, 0 );

    long         minute_put_cntr;
    unsigned int rmp = raw_minute_trend_saving_period;

    // error message buffer
    char errmsgbuf[ 80 ];

    circ_buffer_block_prop_t* cur_prop = new circ_buffer_block_prop_t[ rmp ];
    trend_block_t*            cur_blk[ rmp ];
    for ( int i = 0; i < rmp; i++ )
        cur_blk[ i ] = new trend_block_t[ num_channels ];

    for ( minute_put_cntr = 0;; minute_put_cntr++ )
    {
        int eof_flag = 0;

        int nb = mtb->get( raw_msaver_cnum );
        DEBUG( 3, cerr << "raw minute trender saver; block " << nb << endl );
        {
            cur_prop[ minute_put_cntr % rmp ] = mtb->block_prop( nb )->prop;
            if ( !mtb->block_prop( nb )->bytes )
            {
                mtb->unlock( raw_msaver_cnum );
                eof_flag = 1;
                DEBUG1( cerr << "raw minute trender framer EOF" << endl );
                break; // out of the for() loop
            }
            memcpy( cur_blk[ minute_put_cntr % rmp ],
                    mtb->block_ptr( nb ),
                    num_channels * sizeof( trend_block_t ) );
        }
        mtb->unlock( raw_msaver_cnum );

        if ( minute_put_cntr % rmp == ( rmp - 1 ) )
        {
            // write to raw files. one for each channel
            // open/create file, lock file ???, write a record, close the file

            time_t t = time( 0 );
            DEBUG( 1, cerr << "Begin raw minute trend writing" << endl );
            PV::set_pv( PV::PV_RAW_MTREND_TW_STATE, STATE_WRITING );

            mt_stats.sample( );
            for ( int j = 0; mt_file_stats.sample( ), j < num_channels;
                  mt_file_stats.tick( ), j++ )
            {
                // Calculate combined status
                int status = 0;
                int k;
                for ( k = 0; k < rmp; k++ )
                    status |= cur_blk[ k ][ j ].n;
                if ( status )
                { // don't write bad data points
                    char tmpf[ filesys_c::filename_max + 10 ];
                    char crc8_name[ 3 ];
                    sprintf( crc8_name, "%x", crc8( channels[ j ].name ) );
                    strcpy( tmpf, raw_minute_fsd.get_path( ) );
                    strcat( strcat( tmpf, "/" ), crc8_name );
                    // Create the directory
                    mkdir( tmpf, 0777 );
                    strcat( strcat( tmpf, "/" ), channels[ j ].name );
                    int fd = open( tmpf, O_CREAT | O_WRONLY | O_APPEND, 0644 );
                    if ( fd < 0 )
                    {
                        strerror_r( errno, errmsgbuf, sizeof( errmsgbuf ) );
                        system_log( 1,
                                    "Couldn't open raw minute trend file `%s' "
                                    "for writing; %s",
                                    tmpf,
                                    errmsgbuf );
                        daqd.set_fault( );
                    }
                    else
                    {
                        struct stat fst;
                        int         check_close = 0;
                        if ( fstat( fd, &fst ) )
                        {
                            strerror_r( errno, errmsgbuf, sizeof( errmsgbuf ) );
                            system_log(
                                1,
                                "can't stat raw minute trend file `%s'; %s",
                                tmpf,
                                errmsgbuf );
                            daqd.set_fault( );
                        }
                        else
                        {
                            // Will not try writing to improperly sized file
                            if ( fst.st_size %
                                     sizeof( raw_trend_record_struct ) !=
                                 0 )
                            {
                                system_log( 1,
                                            "Improperly sized file `%s', must "
                                            "be modulo %u in size",
                                            tmpf,
                                            (unsigned int)sizeof(
                                                raw_trend_record_struct ) );
                                daqd.set_fault( );
                            }
                            else
                            {
                                raw_trend_record_struct rmtr[ rmp ];
                                for ( k = 0; k < rmp; k++ )
                                {
                                    rmtr[ k ].gps = cur_prop[ k ].gps;
                                    rmtr[ k ].tb = cur_blk[ k ][ j ];
                                }

                                int wsize =
                                    rmp * sizeof( raw_trend_record_struct );
                                int nw = write( fd, &rmtr, wsize );
                                if ( nw != wsize )
                                {
                                    int error = ftruncate(
                                        fd,
                                        fst.st_size ); // to keep data integrity
                                    strerror_r(
                                        errno, errmsgbuf, sizeof( errmsgbuf ) );
                                    system_log( 1,
                                                "failed to write raw minute "
                                                "trend file `%s' out; %s",
                                                tmpf,
                                                errmsgbuf );
                                    daqd.set_fault( );
                                }
                                check_close = 1; // verify success of close()
                            }
                        }
                        if ( check_close )
                        {
                            if ( daqd.do_fsync )
                            {
                                if ( fsync( fd ) != 0 )
                                {
                                    // No space left in the file system?
                                    int error = ftruncate(
                                        fd,
                                        fst.st_size ); // to keep data integrity
                                    strerror_r(
                                        errno, errmsgbuf, sizeof( errmsgbuf ) );
                                    system_log(
                                        1,
                                        "failed to write raw minute trend file "
                                        "`%s' out; in fsync() %s",
                                        tmpf,
                                        errmsgbuf );
                                    daqd.set_fault( );
                                }
                            }
                        }
                        close( fd );
                    }
                }
            } // for

            mt_stats.tick( );

            t = time( 0 ) - t;
            DEBUG( 1,
                   cerr << "Finished raw minute trend writing in " << t
                        << " seconds" << endl );

            PV::set_pv( PV::PV_RAW_MTREND_TW_STATE, STATE_NORMAL );
            PV::set_pv( PV::PV_RAW_MTREND_TW_WRITE_SEC, t );
        }

        if ( eof_flag )
            break; // out of the for() loop
    }

    // signal to the producer that we are done
    this->shutdown_trender( );
    return NULL;
}

void*
trender_c::minute_framer( )
{
    const int STATE_NORMAL = 0;
    const int STATE_WRITTING = 1;

    // Set thread parameters
    daqd_c::set_thread_priority( "Minute trend framer",
                                 "dqmtrfr",
                                 SAVER_THREAD_PRIORITY,
                                 MINUTE_SAVER_CPUAFFINITY );

    ldas_frame_h_type              frame;
    FrameCPP::FrameH::rawData_type rawData(
        new FrameCPP::FrameH::rawData_type::element_type( "" ) );

    int frame_length_seconds;
    int frame_length_blocks;

    /* Circular buffer length determines minute trend frame length */
    frame_length_blocks = mtb->blocks( );

    /* Length in seconds is the product of lengths of minute trend
     * buffers*seconds in minute */
    frame_length_seconds = frame_length_blocks * SECPERMIN;

    FrameCPP::Version::FrDetector detector = daqd.getDetector1( );

    // Create minute trend frame
    //
    try
    {
        frame = ldas_frame_h_type( new FrameCPP::Version::FrameH(
            "LIGO",
            configuration_number( ), // run number ??? buffptr -> block_prop
                                     // (nb) -> prop.run;
            1, // frame number
            FrameCPP::Version_6::GPSTime( 0, 0 ),
            0, // leap seconds
            frame_length_seconds // dt
            ) );
        frame->SetRawData( rawData );
        frame->RefDetectProc( ).append( detector );

        // Append second detector if it is defined
        if ( daqd.detector_name1.length( ) > 0 )
        {
            FrameCPP::Version::FrDetector detector1 = daqd.getDetector2( );
            frame->RefDetectProc( ).append( detector1 );
        }
    }
    catch ( ... )
    {
        system_log( 1, "Couldn't create minute trend frame" );
        this->shutdown_trender( );
        return NULL;
    }

    // Keep pointers to the data samples for each data channel
    unsigned char* adc_ptr[ num_trend_channels ];
    // INT_2U data_valid [num_trend_channels];
    INT_2U* data_valid_ptr[ num_trend_channels ];

    // error message buffer
    char errmsgbuf[ 80 ];

    // Create ADCs
    try
    {
        for ( int i = 0; i < num_trend_channels; i++ )
        {
            FrameCPP::Version::FrAdcData adc = FrameCPP::Version::FrAdcData(
                std::string( trend_channels[ i ].name ),
                trend_channels[ i ].group_num,
                i, // channel ???
                CHAR_BIT * trend_channels[ i ].bps,
                1. / frame_length_blocks,
                trend_channels[ i ].signal_offset,
                trend_channels[ i ].signal_slope,
                std::string( trend_channels[ i ].signal_units ),
                .0,
                0,
                0,
                .0 );
            FrameCPP::Version::Dimension dims[ 1 ] = {
                FrameCPP::Version::Dimension(
                    frame_length_blocks, frame_length_blocks, "" )
            };
            FrameCPP::Version::FrVect* vect;
            switch ( trend_channels[ i ].data_type )
            {
            case _64bit_double:
            {
                vect = new FrameCPP::Version::FrVect(
                    "", 1, dims, new REAL_8[ frame_length_blocks ], "" );
                break;
            }
            case _32bit_float:
            {
                vect = new FrameCPP::Version::FrVect(
                    "", 1, dims, new REAL_4[ frame_length_blocks ], "" );
                break;
            }
            case _32bit_integer:
            {
                vect = new FrameCPP::Version::FrVect(
                    "", 1, dims, new INT_4S[ frame_length_blocks ], "" );
                break;
            }
            case _32bit_uint:
            {
                vect = new FrameCPP::Version::FrVect(
                    "", 1, dims, new INT_4U[ frame_length_blocks ], "" );
                break;
            }
            default:
            {
                abort( );
                break;
            }
            }
            adc.RefData( ).append( *vect );
            frame->GetRawData( )->RefFirstAdc( ).append( adc );
            adc_ptr[ i ] = frame->GetRawData( )
                               ->RefFirstAdc( )[ i ]
                               ->RefData( )[ 0 ]
                               ->GetData( )
                               .get( );
        }
    }
    catch ( ... )
    {
        system_log( 1, "Couldn't create one or several adcs" );

        abort( );

        this->shutdown_trender( );
        return NULL;
    }

    system_log( 1, "Done creating ADC structures" );

    sem_post( &minute_frame_saver_sem );

    std::vector< trend_block_t > cur_blk_( num_channels );

    PV::set_pv( PV::PV_MTREND_FW_STATE, STATE_NORMAL );
    long frame_cntr;
    for ( frame_cntr = 0;; frame_cntr++ )
    {
        int                      eof_flag = 0;
        circ_buffer_block_prop_t file_prop;
        // trend_block_t cur_blk [num_channels];
        trend_block_t* cur_blk = cur_blk_.data( );

        // Accumulate frame adc data
        for ( int i = 0; i < frame_length_blocks; i++ )
        {
            int nb = mtb->get( msaver_cnum );
            DEBUG( 3, cerr << "minute trender saver; block " << nb << endl );
            {
                if ( !mtb->block_prop( nb )->bytes )
                {
                    mtb->unlock( msaver_cnum );
                    eof_flag = 1;
                    DEBUG1( cerr << "minute trender framer EOF" << endl );
                    break; // out of the for() loop
                }
                if ( !i )
                    file_prop = mtb->block_prop( nb )->prop;

                memcpy( cur_blk,
                        mtb->block_ptr( nb ),
                        num_channels * sizeof( trend_block_t ) );
            }
            mtb->unlock( msaver_cnum );

            // Check if the gps time isn't aligned on an hour boundary
            if ( !i )
            {
                unsigned long gps_mod = file_prop.gps % SECPERHOUR;
                if ( gps_mod )
                {
                    // adjust time to make files aligned on gps mod 3600
                    file_prop.gps -= gps_mod;
                    if ( file_prop.cycle > gps_mod * 16 )
                        file_prop.cycle -= gps_mod * 16;
                    else
                        file_prop.cycle = 0;
                    i += gps_mod / MINPERHOUR;
                }
            }

            for ( int j = 0; j < num_channels; j++ )
            {
                if ( channels[ j ].data_type == _64bit_double )
                {
                    memcpy( adc_ptr[ 5 * j ] + i * sizeof( REAL_8 ),
                            &cur_blk[ j ].min.D,
                            sizeof( REAL_8 ) );
                    memcpy( adc_ptr[ 5 * j + 1 ] + i * sizeof( REAL_8 ),
                            &cur_blk[ j ].max.D,
                            sizeof( REAL_8 ) );
                }
                else if ( channels[ j ].data_type == _32bit_float )
                {
                    memcpy( adc_ptr[ 5 * j ] + i * sizeof( REAL_4 ),
                            &cur_blk[ j ].min.F,
                            sizeof( REAL_4 ) );
                    memcpy( adc_ptr[ 5 * j + 1 ] + i * sizeof( REAL_4 ),
                            &cur_blk[ j ].max.F,
                            sizeof( REAL_4 ) );
                }
                else if ( channels[ j ].data_type == _32bit_uint )
                {
                    memcpy( adc_ptr[ 5 * j ] + i * sizeof( INT_4U ),
                            &cur_blk[ j ].min.U,
                            sizeof( INT_4U ) );
                    memcpy( adc_ptr[ 5 * j + 1 ] + i * sizeof( INT_4U ),
                            &cur_blk[ j ].max.U,
                            sizeof( INT_4U ) );
                }
                else
                {
                    memcpy( adc_ptr[ 5 * j ] + i * sizeof( INT_4S ),
                            &cur_blk[ j ].min.I,
                            sizeof( INT_4S ) );
                    memcpy( adc_ptr[ 5 * j + 1 ] + i * sizeof( INT_4S ),
                            &cur_blk[ j ].max.I,
                            sizeof( INT_4S ) );
                }
                // cerr << j << "\t" << i << hex << (void*)(adc_ptr[5*j+2]) <<
                // endl;
                memcpy( adc_ptr[ 5 * j + 2 ] + i * sizeof( INT_4S ),
                        &cur_blk[ j ].n,
                        sizeof( INT_4S ) );
                // adc_ptr [5*j+2][4*i]=0;
#ifdef not_def
                REAL_8 rms = sqrt( cur_blk[ j ].rms );
                memcpy( adc_ptr[ 5 * j + 3 ] + i * sizeof( REAL_8 ),
                        &rms,
                        sizeof( REAL_8 ) );
#endif
                memcpy( adc_ptr[ 5 * j + 3 ] + i * sizeof( REAL_8 ),
                        &cur_blk[ j ].rms,
                        sizeof( REAL_8 ) );
                memcpy( adc_ptr[ 5 * j + 4 ] + i * sizeof( REAL_8 ),
                        &cur_blk[ j ].mean,
                        sizeof( REAL_8 ) );
            }
        }

        if ( eof_flag )
            break; // out of the for() loop

        frame->SetGTime(
            FrameCPP::Version::GPSTime( file_prop.gps, file_prop.gps_n ) );

        time_t file_gps;
        time_t file_gps_n;
        int    dir_num = 0;

        char _tmpf[ filesys_c::filename_max + 10 ];
        char tmpf[ filesys_c::filename_max + 10 ];

        file_gps = file_prop.gps;
        file_gps_n = file_prop.gps_n;
        dir_num = minute_fsd.getDirFileNames(
            file_gps, _tmpf, tmpf, 1, frame_length_blocks * SECPERMIN );

        int fd = creat( _tmpf, 0644 );
        if ( fd < 0 )
        {
            strerror_r( errno, errmsgbuf, sizeof( errmsgbuf ) );
            system_log(
                1,
                "Couldn't open minute trend frame file `%s' for writing; %s",
                _tmpf,
                errmsgbuf );
            minute_fsd.report_lost_frame( );
            daqd.set_fault( );
        }
        else
        {
            DEBUG( 3, cerr << "`" << _tmpf << "' opened" << endl );
#if defined( DIRECTIO_ON ) && defined( DIRECTIO_OFF )
            if ( daqd.do_directio )
                directio( fd, DIRECTIO_ON );
#endif
            close( fd );
            // For framebuilder, add mutex to serialize second, minute trend
            // writing
            pthread_mutex_lock( &frame_write_lock );
            DEBUG( 3,
                   cerr << "Get trend frame write mutex for minute frame"
                        << endl );

            FrameCPP::Common::MD5SumFilter            md5filter;
            FrameCPP::Common::FrameBuffer< filebuf >* obuf =
                new FrameCPP::Common::FrameBuffer< std::filebuf >(
                    std::ios::out );
            obuf->open( _tmpf, std::ios::out | std::ios::binary );
            obuf->FilterAdd( &md5filter );
            FrameCPP::Common::OFrameStream ofs( obuf );
            ofs.SetCheckSumFile( FrameCPP::Common::CheckSum::CRC );
            DEBUG( 1, cerr << "Begin minute trend WriteFrame()" << endl );

            PV::set_pv( PV::PV_MTREND_FW_STATE, STATE_WRITTING );
            time_t t = time( 0 );

            ofs.WriteFrame(
                frame, // FrameCPP::Common::CheckSum::NONE,
                daqd.no_compression
                    ? FrameCPP::FrVect::RAW
                    : FrameCPP::FrVect::ZERO_SUPPRESS_OTHERWISE_GZIP,
                1,
                FrameCPP::Common::CheckSum::CRC );

            ofs.Close( );
            obuf->close( );
            md5filter.Finalize( );
            pthread_mutex_unlock( &frame_write_lock );

            t = time( 0 ) - t;
            PV::set_pv( PV::PV_MTREND_FW_STATE, STATE_NORMAL );

            daqd.queue_frame_checksum(
                tmpf, daqd_c::minute_trend_frame, md5filter );

            PV::set_pv( PV::PV_MINUTE_FRAME_CHECK_SUM_TRUNC,
                        *reinterpret_cast< const unsigned int* >(
                            md5filter.Value( ) ) );

            DEBUG( 1,
                   cerr << "Minute trend frame write done in " << t
                        << " seconds" << endl );
            if ( rename( _tmpf, tmpf ) )
            {
                strerror_r( errno, errmsgbuf, sizeof( errmsgbuf ) );
                system_log( 1,
                            "minute_framer(): failed to rename file; %s",
                            errmsgbuf );
                minute_fsd.report_lost_frame( );
                daqd.set_fault( );
            }
            else
            {
                DEBUG( 3,
                       cerr << "minute trend frame " << frame_cntr
                            << " is written out" << endl );
                // Successful frame write
                minute_fsd.update_dir(
                    file_gps, file_gps_n, frame_length_blocks, dir_num );
            }
#if EPICS_EDCU == 1
            /* Record frame write time */
            PV::set_pv( PV::PV_MINUTE_FRAME_WRITE_SEC, t );
#endif
        }

#if EPICS_EDCU == 1
        /* Epics display: minute trend data look back size in seconds */
        PV::set_pv( PV::PV_LOOKBACK_MTREND,
                    minute_fsd.get_max( ) - minute_fsd.get_min( ) );

        /* Epics display: current trend frame directory */
        PV::set_pv( PV::PV_LOOKBACK_MTREND_DIR, minute_fsd.get_cur_dir( ) );
#endif

        // get the frame size as well
        {
            unsigned int      fsize = 0;
            raii::file_handle fd( open( tmpf, O_RDONLY ) );
            if ( fd.get( ) >= 0 )
            {
                struct stat finfo;
                if ( fstat( fd.get( ), &finfo ) >= 0 )
                {
                    fsize = finfo.st_size;
                }
            }
            PV::set_pv( PV::PV_MINUTE_FRAME_SIZE, fsize );
        }
    }

    // signal to the producer that we are done
    this->shutdown_trender( );
    return NULL;
}

// Minute trender accumulates trend samples into the local storage
// until the number of samples equal to the length of trend buffer
// is accumulated. At this point minute trender puts a block into
// the minute trend buffer.

// FIXME: shutdown is messed up

void*
trender_c::minute_trend( )
{
    // Set thread parameters
    daqd_c::set_thread_priority( "Minute trender",
                                 "dqmtrco",
                                 SAVER_THREAD_PRIORITY,
                                 MINUTE_TRENDER_CPUAFFINITY );

    int           tblen = mtb->blocks( ); // trend buffer length
    int           nc; // number of trend samples accumulated
    int           nb; // trend buffer block number
    trend_block_t ttb[ num_channels ]; // minute trend local storage
    unsigned long npoints[ num_channels ]; // number of data points processed

    for ( nc = 0;; )
    {
        circ_buffer_block_prop_t prop;

        // Request to shut us down received from consumer
        //
        if ( stopping() )
        {
            // FIXME -- synchronize with the demise of the trender
            //	  shutdown_buffer ();
            continue;
        }

        nb = tb->get( mcnum );
        {
            trend_block_t* btr = (trend_block_t*)tb->block_ptr( nb );
            int            bytes;

            // Shutdown request from trend circ buffer
            if ( !( bytes = tb->block_prop( nb )->bytes ) )
            {
                char pbuf[ 1 ];
                mtb->put( pbuf, 0 );
                return NULL;
            }

            // Start of new minute trend block
            if ( !nc )
            {
                prop =
                    tb->block_prop( nb )->prop; // remember initial properties
                memset( ttb, 0, sizeof( ttb[ 0 ] ) * num_channels );
                memset( npoints, 0, sizeof( npoints[ 0 ] ) * num_channels );
                // Check GPS time; it must be aligned on 60 secs boundary
                // correct if wrong
                if ( prop.gps % tblen )
                {
                    nc = prop.gps % tblen;
                    system_log( 1,
                                "Minute trender made GPS time correction; "
                                "gps=%d; gps%%%d=%d",
                                (int)prop.gps,
                                tblen,
                                (int)( prop.gps % tblen ) );
                    prop.gps -=
                        prop.gps % tblen; // This minute trend point has skips
                                          // (gap in the beginning)
                }
            }

            for ( register int j = 0; j < num_channels; j++ )
            {
                if ( btr[ j ].n )
                {
                    if ( !npoints[ j ] )
                    { // first good data point for this channel
                        ttb[ j ] =
                            btr[ j ]; // set initial trend for this channel
                        ttb[ j ].rms *=
                            ttb[ j ].rms; // prepare squares for RMS channel
                    }
                    else
                    { // not the first data point for this channel
#ifndef NO_RMS
                        ttb[ j ].rms += (double)( btr[ j ].rms * btr[ j ].rms );
#endif
                        ttb[ j ].mean += btr[ j ].mean;
                        ttb[ j ].n += btr[ j ].n;
                        if ( channels[ j ].data_type == _64bit_double )
                        {
                            if ( btr[ j ].min.D < ttb[ j ].min.D )
                                ttb[ j ].min.D = btr[ j ].min.D;
                            if ( btr[ j ].max.D > ttb[ j ].max.D )
                                ttb[ j ].max.D = btr[ j ].max.D;
                        }
                        else if ( channels[ j ].data_type == _32bit_float )
                        {
                            if ( btr[ j ].min.F < ttb[ j ].min.F )
                                ttb[ j ].min.F = btr[ j ].min.F;
                            if ( btr[ j ].max.F > ttb[ j ].max.F )
                                ttb[ j ].max.F = btr[ j ].max.F;
                        }
                        else if ( channels[ j ].data_type == _32bit_uint )
                        {
                            if ( btr[ j ].min.U < ttb[ j ].min.U )
                                ttb[ j ].min.U = btr[ j ].min.U;
                            if ( btr[ j ].max.U > ttb[ j ].max.U )
                                ttb[ j ].max.U = btr[ j ].max.U;
                        }
                        else
                        {
                            if ( btr[ j ].min.I < ttb[ j ].min.I )
                                ttb[ j ].min.I = btr[ j ].min.I;
                            if ( btr[ j ].max.I > ttb[ j ].max.I )
                                ttb[ j ].max.I = btr[ j ].max.I;
                        }
                    }
                    npoints[ j ]++; // count this data point in, there is good
                                    // data available
                }
            }
        }
        tb->unlock( mcnum );

        nc++;
        if ( nc == tblen )
        {
#ifndef NO_RMS
            for ( register int j = 0; j < num_channels; j++ )
            {
                if ( npoints[ j ] )
                {
                    ttb[ j ].rms = sqrt( ttb[ j ].rms / (double)npoints[ j ] );
                    ttb[ j ].mean = ttb[ j ].mean / (double)npoints[ j ];
                }
            }
#endif
            nc = 0;
            DEBUG( 3, cerr << "minute trender consumer " << prop.gps << endl );
            mtb->put( (char*)ttb, block_size, &prop );
        }
    }
    return NULL;
}

void*
trender_c::saver( )
{
    int nb;

    for ( ;; )
    {
        nb = tb->get( saver_cnum );
        {
            if ( !tb->block_prop( nb )->bytes )
                break;

            trend_block_t* blk;

            blk = (trend_block_t*)tb->block_ptr( nb );

            *fout << "block " << nb << "gps=" << tb->block_prop( nb )->prop.gps
                  << endl;
            for ( int j = 0; j < num_channels; j++ )
            {
                *fout << channels[ j ].name << endl;
                *fout << "min=" << blk[ j ].min.I << endl;
                *fout << "max=" << blk[ j ].max.I << endl;
#ifndef NO_RMS
                *fout << "rms=" << blk[ j ].rms << endl;
#endif
            }
        }
        tb->unlock( saver_cnum );
    }
    DEBUG1( cerr << "trender saver EOF" << endl );
    this->shutdown_trender( );
    return NULL;
}

void*
trender_c::framer( )
{
    const int STATE_NORMAL = 0;
    const int STATE_WRITTING = 1;

    // Set thread parameters
    daqd_c::set_thread_priority( "Second trend framer",
                                 "dqstrfr",
                                 SAVER_THREAD_PRIORITY,
                                 SECOND_SAVER_CPUAFFINITY );

    ldas_frame_h_type              frame;
    FrameCPP::FrameH::rawData_type rawData(
        new FrameCPP::FrameH::rawData_type::element_type( "" ) );

    int frame_length_blocks;

    /* Circular buffer length determines trend frame length */
    frame_length_blocks = tb->blocks( );

    /* Detector data */
    FrameCPP::Version::FrDetector detector = daqd.getDetector1( );

    // Create trend frame
    //
    try
    {
        frame = ldas_frame_h_type( new FrameCPP::Version::FrameH(
            "LIGO",
            configuration_number( ), // run number ??? buffptr -> block_prop
                                     // (nb) -> prop.run;
            1, // frame number
            FrameCPP::Version_6::GPSTime( 0, 0 ),
            0, // localTime
            frame_length_blocks // dt
            ) );
        frame->SetRawData( rawData );
        frame->RefDetectProc( ).append( detector );
        // Append second detector if it is defined
        if ( daqd.detector_name1.length( ) > 0 )
        {
            FrameCPP::Version::FrDetector detector1 = daqd.getDetector2( );
            frame->RefDetectProc( ).append( detector1 );
        }
    }
    catch ( ... )
    {
        system_log( 1, "Couldn't create second trend frame" );
        this->shutdown_trender( );
        return NULL;
    }

    // Keep pointers to the data samples for each data channel
    unsigned char* adc_ptr[ num_trend_channels ];
    // INT_2U data_valid [num_trend_channels];
    INT_2U* data_valid_ptr[ num_trend_channels ];

    // error message buffer
    char errmsgbuf[ 80 ];

    // Create  ADCs
    try
    {
        for ( int i = 0; i < num_trend_channels; i++ )
        {
            FrameCPP::Version::FrAdcData adc = FrameCPP::Version::FrAdcData(
                std::string( trend_channels[ i ].name ),
                trend_channels[ i ].group_num,
                i, // channel ???
                CHAR_BIT * trend_channels[ i ].bps,
                trend_channels[ i ].sample_rate,
                trend_channels[ i ].signal_offset,
                trend_channels[ i ].signal_slope,
                std::string( trend_channels[ i ].signal_units ),
                .0,
                0,
                0,
                .0 );

            FrameCPP::Version::Dimension dims[ 1 ] = {
                FrameCPP::Version::Dimension(
                    frame_length_blocks,
                    1. / trend_channels[ i ].sample_rate,
                    "" )
            };
            FrameCPP::Version::FrVect* vect;
            switch ( trend_channels[ i ].data_type )
            {
            case _64bit_double:
            {
                vect = new FrameCPP::Version::FrVect(
                    "", 1, dims, new REAL_8[ frame_length_blocks ], "" );
                break;
            }
            case _32bit_float:
            {
                vect = new FrameCPP::Version::FrVect(
                    "", 1, dims, new REAL_4[ frame_length_blocks ], "" );
                break;
            }
            case _32bit_integer:
            {
                vect = new FrameCPP::Version::FrVect(
                    "", 1, dims, new INT_4S[ frame_length_blocks ], "" );
                break;
            }
            default:
            {
                abort( );
                break;
            }
            }
            adc.RefData( ).append( *vect );
            frame->GetRawData( )->RefFirstAdc( ).append( adc );
            adc_ptr[ i ] = frame->GetRawData( )
                               ->RefFirstAdc( )[ i ]
                               ->RefData( )[ 0 ]
                               ->GetData( )
                               .get( );
        }
    }
    catch ( ... )
    {
        system_log( 1, "Couldn't create one or several adcs" );

        abort( );

        this->shutdown_trender( );
        return NULL;
    }

    sem_post( &frame_saver_sem );

    std::vector< trend_block_t > cur_blk_( num_channels );

    PV::set_pv( PV::PV_STREND_FW_STATE, STATE_NORMAL );
    long frame_cntr;
    for ( frame_cntr = 0;; frame_cntr++ )
    {
        int                      eof_flag = 0;
        circ_buffer_block_prop_t file_prop;
        trend_block_t*           cur_blk = cur_blk_.data( );
        // trend_block_t cur_blk [num_channels];

        // Accumulate frame adc data
        for ( int i = 0; i < frame_length_blocks; i++ )
        {
            int nb = tb->get( saver_cnum );
            DEBUG( 3, cerr << "second trender saver; block " << nb << endl );
            {
                if ( !tb->block_prop( nb )->bytes )
                {
                    tb->unlock( saver_cnum );
                    eof_flag = 1;
                    DEBUG1( cerr << "second trender framer EOF" << endl );
                    break; // out of the for() loop
                }
                if ( !i )
                    file_prop = tb->block_prop( nb )->prop;

                memcpy( cur_blk,
                        tb->block_ptr( nb ),
                        num_channels * sizeof( trend_block_t ) );
            }
            tb->unlock( saver_cnum );

            // Check if the gps time isn't aligned on a frame start time
            //   Second trend frames align at gps=0 and every
            //   frame-length-blocks (multiples of 60)
            if ( !i )
            {
                unsigned long gps_mod = file_prop.gps % frame_length_blocks;
                if ( gps_mod )
                {
                    // adjust time to make files aligned on gps mod
                    // frame_length_blocks
                    file_prop.gps -= gps_mod;
                    i += gps_mod;

                    if ( file_prop.cycle > gps_mod * 16 )
                        file_prop.cycle -= gps_mod * 16;
                    else
                        file_prop.cycle = 0;

                    // zero out some data that's missing
                    for ( int j = 0; j < num_trend_channels; j++ )
                        memset( adc_ptr[ j ],
                                0,
                                gps_mod * trend_channels[ j ].bps );
                }
            }

            for ( int j = 0; j < num_channels; j++ )
            {
                if ( channels[ j ].data_type == _64bit_double )
                {
                    memcpy( adc_ptr[ 5 * j ] + i * sizeof( REAL_8 ),
                            &cur_blk[ j ].min.D,
                            sizeof( REAL_8 ) );
                    memcpy( adc_ptr[ 5 * j + 1 ] + i * sizeof( REAL_8 ),
                            &cur_blk[ j ].max.D,
                            sizeof( REAL_8 ) );
                }
                else if ( channels[ j ].data_type == _32bit_float )
                {
                    memcpy( adc_ptr[ 5 * j ] + i * sizeof( REAL_4 ),
                            &cur_blk[ j ].min.F,
                            sizeof( REAL_4 ) );
                    memcpy( adc_ptr[ 5 * j + 1 ] + i * sizeof( REAL_4 ),
                            &cur_blk[ j ].max.F,
                            sizeof( REAL_4 ) );
                }
                else if ( channels[ j ].data_type == _32bit_uint )
                {
                    memcpy( adc_ptr[ 5 * j ] + i * sizeof( INT_4U ),
                            &cur_blk[ j ].min.U,
                            sizeof( INT_4U ) );
                    memcpy( adc_ptr[ 5 * j + 1 ] + i * sizeof( INT_4U ),
                            &cur_blk[ j ].max.U,
                            sizeof( INT_4U ) );
                }
                else
                {
                    memcpy( adc_ptr[ 5 * j ] + i * sizeof( INT_4S ),
                            &cur_blk[ j ].min.I,
                            sizeof( INT_4S ) );
                    memcpy( adc_ptr[ 5 * j + 1 ] + i * sizeof( INT_4S ),
                            &cur_blk[ j ].max.I,
                            sizeof( INT_4S ) );
                }
                // cerr << j << "\t" << i << hex << (void*)(adc_ptr[5*j+2]) <<
                // endl;
                memcpy( adc_ptr[ 5 * j + 2 ] + i * sizeof( INT_4S ),
                        &cur_blk[ j ].n,
                        sizeof( INT_4S ) );
                // adc_ptr [5*j+2][4*i]=0;
#ifdef not_def
                REAL_8 rms = sqrt( cur_blk[ j ].rms );
                memcpy( adc_ptr[ 5 * j + 3 ] + i * sizeof( REAL_8 ),
                        &rms,
                        sizeof( REAL_8 ) );
#endif
                memcpy( adc_ptr[ 5 * j + 3 ] + i * sizeof( REAL_8 ),
                        &cur_blk[ j ].rms,
                        sizeof( REAL_8 ) );
                memcpy( adc_ptr[ 5 * j + 4 ] + i * sizeof( REAL_8 ),
                        &cur_blk[ j ].mean,
                        sizeof( REAL_8 ) );
            }
        }

        if ( eof_flag )
            break; // out of the for() loop

        // fw -> setFrameFileAttributes (file_prop.run, file_prop.cycle / 16 /
        // 60, 0, file_prop.gps, 60, file_prop.gps_n, file_prop.leap_seconds,
        // file_prop.altzone);

        frame->SetGTime(
            FrameCPP::Version::GPSTime( file_prop.gps, file_prop.gps_n ) );

        DEBUG( 1,
               cerr << "about to write second trend frame @ " << file_prop.gps
                    << endl );

        time_t file_gps;
        time_t file_gps_n;
        int    dir_num = 0;

        char tmpf[ filesys_c::filename_max + 10 ];
        char _tmpf[ filesys_c::filename_max + 10 ];

        file_gps = file_prop.gps;
        file_gps_n = file_prop.gps_n;
        dir_num = fsd.getDirFileNames(
            file_gps, _tmpf, tmpf, 1, frame_length_blocks );

        int fd = creat( _tmpf, 0644 );
        if ( fd < 0 )
        {
            strerror_r( errno, errmsgbuf, sizeof( errmsgbuf ) );
            system_log(
                1,
                "Couldn't open full trend frame file `%s' for writing; %s",
                tmpf,
                errmsgbuf );
            fsd.report_lost_frame( );
            daqd.set_fault( );
        }
        else
        {
            DEBUG( 3, cerr << "`" << _tmpf << "' opened" << endl );
            close( fd );
            // For framebuilder, add mutex to serialize second, minute trend
            // writing
            pthread_mutex_lock( &frame_write_lock );
            DEBUG( 3,
                   cerr << "Get trend frame write mutex for second frame"
                        << endl );

            FrameCPP::Common::MD5SumFilter            md5filter;
            FrameCPP::Common::FrameBuffer< filebuf >* obuf =
                new FrameCPP::Common::FrameBuffer< std::filebuf >(
                    std::ios::out );
            obuf->FilterAdd( &md5filter );
            obuf->open( _tmpf, std::ios::out | std::ios::binary );
            FrameCPP::Common::OFrameStream ofs( obuf );
            ofs.SetCheckSumFile( FrameCPP::Common::CheckSum::CRC );
            DEBUG( 1, cerr << "Begin second trend WriteFrame()" << endl );
            PV::set_pv( PV::PV_STREND_FW_STATE, STATE_WRITTING );
            time_t t = time( 0 );

            ofs.WriteFrame(
                frame, // FrameCPP::Common::CheckSum::NONE,
                daqd.no_compression
                    ? FrameCPP::FrVect::RAW
                    : FrameCPP::FrVect::ZERO_SUPPRESS_OTHERWISE_GZIP,
                1,
                FrameCPP::Common::CheckSum::CRC );

            ofs.Close( );
            obuf->close( );
            md5filter.Finalize( );

            pthread_mutex_unlock( &frame_write_lock );

            t = time( 0 ) - t;
            PV::set_pv( PV::PV_STREND_FW_STATE, STATE_NORMAL );

            daqd.queue_frame_checksum(
                tmpf, daqd_c::second_trend_frame, md5filter );

            PV::set_pv( PV::PV_SECOND_FRAME_CHECK_SUM_TRUNC,
                        *reinterpret_cast< const unsigned int* >(
                            md5filter.Value( ) ) );

            DEBUG( 1,
                   cerr << "Second trend frame write done in " << t
                        << " seconds" << endl );
            if ( rename( _tmpf, tmpf ) )
            {
                strerror_r( errno, errmsgbuf, sizeof( errmsgbuf ) );
                system_log(
                    1, "framer(): failed to rename file; %s", errmsgbuf );
                fsd.report_lost_frame( );
                daqd.set_fault( );
            }
            else
            {
                DEBUG( 3,
                       cerr << "trend frame " << frame_cntr << " is written out"
                            << endl );
                // Successful frame write
                fsd.update_dir(
                    file_gps, file_gps_n, frame_length_blocks, dir_num );
            }
#if EPICS_EDCU == 1
            /* Record frame write time */
            PV::set_pv( PV::PV_SECOND_FRAME_WRITE_SEC, t );
#endif
        }

#if EPICS_EDCU == 1
        /* Epics display: second trend data look back size in seconds */
        PV::set_pv( PV::PV_LOOKBACK_STREND, fsd.get_max( ) - fsd.get_min( ) );

        /* Epics display: current trend frame directory */
        PV::set_pv( PV::PV_LOOKBACK_STREND_DIR, fsd.get_cur_dir( ) );

        // get the frame size as well
        {
            unsigned int      fsize = 0;
            raii::file_handle fd( open( tmpf, O_RDONLY ) );
            if ( fd.get( ) >= 0 )
            {
                struct stat finfo;
                if ( fstat( fd.get( ), &finfo ) >= 0 )
                {
                    fsize = finfo.st_size;
                }
            }
            PV::set_pv( PV::PV_SECOND_FRAME_SIZE, fsize );
        }
#endif
    }

    // signal to the producer that we are done
    this->shutdown_trender( );
    return NULL;
}

inline void
trender_c::trend_loop_func( int j, int* status_ptr, char* block_ptr )
{
    int bad = *( status_ptr + channels[ j ].seq_num * 17 );
    if ( bad && daqd.zero_bad_data )
    {
        // No data is available for this channels for this second.
        // Mark trend for this channels as bad setting number of points
        // available to zero. Reset all the rest of the data also.
        memset( &ttb[ j ], 0, sizeof( trend_block_t ) );
    }
    else
    {
        ttb[ j ].n = channels[ j ].sample_rate;
#ifndef NO_SLOW_CHANNELS
        if ( channels[ j ].slow )
        {
#ifndef NO_RMS
            ttb[ j ].rms =
#endif
                ttb[ j ].mean = ttb[ j ].min.F = ttb[ j ].max.F =
                    *(float*)( block_ptr + channels[ j ].offset );
        }
        else
#endif // NO_SLOW_CHANNELS
        {
            int rate = channels[ j ].sample_rate;
            if ( channels[ j ].data_type == _64bit_double )
            {
                register double* data =
                    (double*)( block_ptr + channels[ j ].offset );

#ifndef NO_RMS
                register double rms = (double)( *data * *data );
#endif
                register double mean = *data;
                register double min = *data;
                register double max = *data;

                for ( register int i = 1; i < rate; i++ )
                {
                    register double test = data[ i ];
#ifndef NO_RMS
                    rms += ( test * test );
#endif
                    mean += test;
                    if ( test < min )
                        min = test;
                    if ( test > max )
                        max = test;
                }
#ifndef NO_RMS
                ttb[ j ].rms = sqrt( rms / (double)rate );
#endif
                ttb[ j ].mean = mean / (double)rate;
                ttb[ j ].min.D = min;
                ttb[ j ].max.D = max;
            }
            else if ( channels[ j ].data_type == _32bit_float &&
                      channels[ j ].bps == 4 )
            {
                int*  data = (int*)( block_ptr + channels[ j ].offset );
                float d;

                // is not compiled correctly with the new compiler
                //*((int *)&d) = *data;
                // Replacing the above with the float assignment fixes the
                // problem!
                d = *( (float*)( block_ptr + channels[ j ].offset ) );

#ifndef NO_RMS
                double rms = (double)( d * d );
#endif
                double mean = d;
                float  min = d;
                float  max = d;

                for ( int i = 1; i < rate; i++ )
                {
                    float test;
                    //*((int *)&test) = data [i];
                    test = *( (float*)( data + i ) );
#ifndef NO_RMS
                    rms += ( test * test );
#endif
                    mean += test;
                    if ( test < min )
                        min = test;
                    if ( test > max )
                        max = test;
                }
#ifndef NO_RMS
                ttb[ j ].rms = sqrt( rms / (double)rate );
#endif
                ttb[ j ].mean = mean / (double)rate;
                ttb[ j ].min.F = min;
                ttb[ j ].max.F = max;
            }
            else if ( channels[ j ].data_type == _32bit_float &&
                      channels[ j ].bps == 8 )
            { /* Complex data */
                register float* data =
                    (float*)( block_ptr + channels[ j ].offset );
                double test =
                    sqrt( data[ 0 ] * data[ 0 ] + data[ 1 ] * data[ 1 ] );
                data += 2;
#ifndef NO_RMS
                register double rms = test * test;
#endif
                register double mean = test;
                register float  min = test;
                register float  max = test;

                /* Complex data trend are calculated on magnitude */
                for ( register int i = 1; i < rate; i++ )
                {
                    test = data[ 0 ] * data[ 0 ] +
                        data[ 1 ] * data[ 1 ]; /* Magnitude squared */
                    data += 2;
#ifndef NO_RMS
                    rms += test;
#endif
                    test = sqrt( test ); /* Magnitude */
                    mean += test;
                    if ( test < min )
                        min = test;
                    if ( test > max )
                        max = test;
                }
#ifndef NO_RMS
                ttb[ j ].rms = sqrt( rms / (double)rate );
#endif
                ttb[ j ].mean = mean / (double)rate;
                ttb[ j ].min.F = min;
                ttb[ j ].max.F = max;
            }
            else if ( channels[ j ].data_type == _32bit_uint )
            { // Treat as unsigned, because it is...
                register unsigned int* data =
                    (unsigned int*)( block_ptr + channels[ j ].offset );

#ifndef NO_RMS
                register double rms = (double)( *data * *data );
#endif
                register double       mean = *data;
                register unsigned int min = *data;
                register unsigned int max = *data;

                for ( register int i = 1; i < rate; i++ )
                {
                    register unsigned int test = data[ i ];
#ifndef NO_RMS
                    rms += (double)( test * test );
#endif
                    mean += test;
                    if ( test < min )
                        min = test;
                    if ( test > max )
                        max = test;
                }
#ifndef NO_RMS
                ttb[ j ].rms = sqrt( rms / (double)rate );
#endif
                ttb[ j ].mean = mean / (double)rate;
                ttb[ j ].min.U = min;
                ttb[ j ].max.U = max;
            }
            else if ( channels[ j ].data_type == _32bit_integer )
            { // Treat this data as signed
                register int* data = (int*)( block_ptr + channels[ j ].offset );

#ifndef NO_RMS
                register double rms = (double)( *data * *data );
#endif
                register double mean = *data;
                register int    min = *data;
                register int    max = *data;

                for ( register int i = 1; i < rate; i++ )
                {
                    register int test = data[ i ];
#ifndef NO_RMS
                    rms += (double)( test * test );
#endif
                    mean += test;
                    if ( test < min )
                        min = test;
                    if ( test > max )
                        max = test;
                }
#ifndef NO_RMS
                ttb[ j ].rms = sqrt( rms / (double)rate );
#endif
                ttb[ j ].mean = mean / (double)rate;
                ttb[ j ].min.I = min;
                ttb[ j ].max.I = max;
            }
            else
            {
                register short* data =
                    (short*)( block_ptr + channels[ j ].offset );

#ifndef NO_RMS
                register double rms = (double)( *data * *data );
#endif
                register double mean = *data;
                register short  min = *data;
                register short  max = *data;

                for ( register int i = 1; i < rate; i++ )
                {
                    register short test = data[ i ];
#ifndef NO_RMS
                    rms += (double)( test * test );
#endif
                    mean += test;
                    if ( test < min )
                        min = test;
                    if ( test > max )
                        max = test;
                }
#ifndef NO_RMS
                ttb[ j ].rms = sqrt( rms / (double)rate );
#endif
                ttb[ j ].mean = mean / (double)rate;
                ttb[ j ].min.I = min;
                ttb[ j ].max.I = max;
            }
        }
    }
}

#define WORKPILE_INC 10

void*
trender_c::trend( )
{
    // Set thread parameters
    daqd_c::set_thread_priority( "Second trend consumer",
                                 "dqstrco",
                                 SAVER_THREAD_PRIORITY,
                                 SECOND_TRENDER_CPUAFFINITY );
    int          nb;
    circ_buffer* ltb = this->tb;
    sem_post( &trender_sem );

#ifdef not_def
    // Find out how to split the work with worker thread
    {
        unsigned long trend_load_size = 0;
        // Calculate summary trender data load
        for ( unsigned int i = 0; i < num_channels; i++ )
            trend_load_size += channels[ i ].bytes;

        system_log( 1, "Trend's work load is %d bytes", trend_load_size );
        unsigned long bsize = trend_load_size / 2;
        unsigned long load_size = 0;
        for ( worker_first_channel = 0; worker_first_channel < num_channels;
              worker_first_channel++ )
        {
            load_size += channels[ worker_first_channel ].bytes;
            if ( load_size > bsize )
                break;
        }
        system_log( 1,
                    "Trend Worker's first channel is #%d (%s)",
                    worker_first_channel,
                    channels[ worker_first_channel ].name );
    }
#endif

    for ( ;; )
    {
        circ_buffer_block_prop_t prop;

#ifdef not_def
        pthread_mutex_lock( &lock );
        ltb = this->tb;
        pthread_mutex_unlock( &lock );

        if ( ltb ) /* There is a trend buffer, so we need to process data */
#endif

        {
            // Request to shut us down received from consumer
            //
            if ( stopping() )
            {
                // FIXME -- synchronize with the demise of the minute trender
                shutdown_buffer( );
                continue;
            }

            nb = daqd.b1->get( cnum );
            {
                char*         block_ptr = daqd.b1->block_ptr( nb );
                unsigned long block_bytes = daqd.b1->block_prop( nb )->bytes;

                // Shutdown request from the main circ buffer
                if ( !block_bytes )
                {
                    char pbuf[ 1 ];
                    ltb->put( pbuf, 0 );
                    return NULL;
                }

                pthread_mutex_lock( &worker_lock );
                trend_worker_nb = nb;
                next_trender_block = 0;
                next_worker_block = num_channels;
                worker_busy = 1;
                pthread_mutex_unlock( &worker_lock );
                pthread_cond_signal( &worker_notempty );

                // This points to the start of status word area
                int* status_ptr = (int*)( block_ptr + daqd.block_size ) -
                    17 * daqd.num_channels;

                for ( bool finished = false;; )
                {
                    pthread_mutex_lock( &worker_lock );
                    int first_block = next_trender_block;
                    if ( next_trender_block >= next_worker_block )
                    {
                        finished = true;
                    }
                    else
                    {
                        next_trender_block += WORKPILE_INC;
                        if ( next_trender_block > num_channels )
                            next_trender_block = num_channels;

                        if ( next_trender_block > next_worker_block )
                            next_trender_block = next_worker_block;
                    }
                    pthread_mutex_unlock( &worker_lock );

                    if ( finished )
                    {
                        break;
                    }

                    // Calculate MIN, MAX and RMS for all channels
                    for ( register int j = first_block; j < next_trender_block;
                          j++ )
                        trend_loop_func( j, status_ptr, block_ptr );
                }

                prop = daqd.b1->block_prop( nb )->prop;

                // wait for worker thread
                pthread_mutex_lock( &worker_lock );
                while ( worker_busy )
                    pthread_cond_wait( &worker_done, &worker_lock );
                pthread_mutex_unlock( &worker_lock );

                DEBUG( 3,
                       cerr << "trender finished; next_trender_block="
                            << next_trender_block << "; next_worker_block="
                            << next_worker_block << endl );
            }
            daqd.b1->unlock( cnum );

            DEBUG( 3, cerr << "trender consumer " << prop.gps << endl );

#ifdef not_def
            for ( register int j = 0; j < num_channels; j++ )
            {
                ttb[ j ].min.F = ttb[ j ].max.F = j;
                ttb[ j ].rms = j;
            }
#endif
            ltb->put( (char*)ttb, block_size, &prop );
        }
#ifdef not_def
        else /* Do stuff required for the synchronization */
            daqd.b1->noop( cnum );
#endif
    }

    return NULL;
}

void*
trender_c::trend_worker( )
{
    // Set thread parameters
    daqd_c::set_thread_priority(
        "Second trend worker", "dqstrwk", 0, SECOND_TREND_WORKER_CPUAFFINITY );

    for ( ;; )
    {
        // get control from the trender thread
        pthread_mutex_lock( &worker_lock );
        while ( !worker_busy )
            pthread_cond_wait( &worker_notempty, &worker_lock );
        int nb = trend_worker_nb;
        pthread_mutex_unlock( &worker_lock );

        char* block_ptr = daqd.b1->block_ptr( nb );

        // This points to the start of status word area
        int* status_ptr =
            (int*)( block_ptr + daqd.block_size ) - 17 * daqd.num_channels;

        // Calculate MIN, MAX and RMS for all channels
        for ( bool finished = false;; )
        {
            pthread_mutex_lock( &worker_lock );
            int last_block = next_worker_block;
            if ( next_trender_block >= next_worker_block )
                finished = true;
            else
            {
                if ( next_worker_block < WORKPILE_INC )
                    next_worker_block = 0;
                else
                    next_worker_block -= WORKPILE_INC;
                if ( next_worker_block < next_trender_block )
                    next_worker_block = next_trender_block;
            }
            pthread_mutex_unlock( &worker_lock );

            if ( finished )
                break;

            // Calculate MIN, MAX and RMS for all channels
            for ( register int j = next_worker_block; j < last_block; j++ )
                trend_loop_func( j, status_ptr, block_ptr );

            if ( next_worker_block == 0 ) // finished
                break;
        }

        // signal trender thread that we are finished
        pthread_mutex_lock( &worker_lock );
        worker_busy = 0;
        pthread_mutex_unlock( &worker_lock );
        pthread_cond_signal( &worker_done );
    }

    return NULL;
}

int
trender_c::start_trend( ostream* yyout,
                        int      pframes_per_file,
                        int      pminute_frames_per_file,
                        int      ptrend_buffer_blocks,
                        int      pminute_trend_buffer_blocks )
{
    if ( this->tb )
    {
        *yyout << "trend is already running" << endl;
        return 1;
    }

    if ( pminute_trend_buffer_blocks != SECPERMIN )
    {
        *yyout << "Minute trend buffer blocks must be " << SECPERMIN
               << " aborting process." << endl;
        exit( 1 );
    }
    /*
       Set trender operational parameters.
    */
    frames_per_file = pframes_per_file;
    minute_frames_per_file = pminute_frames_per_file;
    trend_buffer_blocks = ptrend_buffer_blocks;
    minute_trend_buffer_blocks = SECPERMIN;

    // error message buffer
    char errmsgbuf[ 80 ];

    // Allocate trend circular buffer
    {
        void* mptr = malloc( sizeof( circ_buffer ) );
        if ( !mptr )
        {
            *yyout
                << "couldn't construct trend circular buffer, memory exhausted"
                << endl;
            return 1;
        }

#ifdef not_def
        if ( !( tb =
                    new circ_buffer( 0, trend_buffer_blocks, block_size, 1 ) ) )
        {
            *yyout << "couldn't construct trender circular buffer, memory "
                      "exhausted"
                   << endl;
            return 1;
        }
#endif

        // check that we don't exceed limit
        if ( trend_buffer_blocks > MAX_BLOCKS )
        {
            system_log( 1,
                        "FATAL: second trend frame length exceeds MAX_BLOCKS "
                        "limit of %d",
                        MAX_BLOCKS );
            return 1;
        }

        // check that second trend frames are in multiples of whole minutes
        int blkresid = trend_buffer_blocks % SECPERMIN;
        if ( blkresid > 0 )
        {
            system_log( 1,
                        "FATAL: second trend frame length must be whole "
                        "minutes - multiple of %d",
                        SECPERMIN );
            return 1;
        }

        tb = new ( mptr ) circ_buffer( 0, trend_buffer_blocks, block_size, 1 );
        if ( !( tb->buffer_ptr( ) ) )
        {
            tb->~circ_buffer( );
            free( (void*)tb );
            tb = 0;
            *yyout << "couldn't allocate second trender buffer data blocks, "
                      "memory exhausted"
                   << endl;
            return 1;
        }
    }

    // Allocate minute trend cicrular buffer
    {
        void* mptr = malloc( sizeof( circ_buffer ) );
        if ( !mptr )
        {
            *yyout << "couldn't construct minute trend circular buffer, memory "
                      "exhausted"
                   << endl;
            return 1;
        }

        // check that we don't exceed limit
        if ( minute_trend_buffer_blocks > MAX_BLOCKS )
        {
            system_log( 1,
                        "FATAL: minute trend frame length exceeds MAX_BLOCKS "
                        "limit of %d",
                        MAX_BLOCKS );
            return 1;
        }

        // check that minute trend frames are in multiples of whole hours
        int blkresid = minute_trend_buffer_blocks % MINPERHOUR;
        if ( blkresid > 0 )
        {
            system_log( 1,
                        "FATAL: minute trend frame length must be whole hours "
                        "- multiple of %d",
                        MINPERHOUR );
            return 1;
        }

        // SECPERMIN gives minute trend buffer data block period
        mtb = new ( mptr )
            circ_buffer( 0, minute_trend_buffer_blocks, block_size, SECPERMIN );
        if ( !( mtb->buffer_ptr( ) ) )
        {
            mtb->~circ_buffer( );
            free( (void*)mtb );
            mtb = 0;
            *yyout << "couldn't allocate minute trender buffer data blocks, "
                      "memory exhausted"
                   << endl;
            return 1;
        }
    }

    // Start minute trender
    if ( ( mcnum = tb->add_consumer( ) ) >= 0 )
    {
        pthread_attr_t attr;
        pthread_attr_init( &attr );
        pthread_attr_setstacksize( &attr, daqd.thread_stack_size );
        pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM );
        pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
        int err_no;
        if ( err_no = pthread_create( &mconsumer,
                                      &attr,
                                      (void* (*)(void*))minute_trend_static,
                                      (void*)this ) )
        {
            strerror_r( err_no, errmsgbuf, sizeof( errmsgbuf ) );
            system_log( 1,
                        "couldn't create minute trend consumer thread; "
                        "pthread_create() err=%s",
                        errmsgbuf );
            tb->delete_consumer( mcnum );
            mcnum = 0;
            tb->~circ_buffer( );
            free( (void*)tb );
            tb = 0;
            mtb->~circ_buffer( );
            free( (void*)mtb );
            mtb = 0;
            pthread_attr_destroy( &attr );
            return 1;
        }
        pthread_attr_destroy( &attr );
        DEBUG( 2,
               cerr << "minute trend consumer created; tid=" << mconsumer
                    << endl );
    }
    else
    {
        *yyout << "couldn't add minute trend consumer" << endl;
        tb->~circ_buffer( );
        free( (void*)tb );
        tb = 0;
        mtb->~circ_buffer( );
        free( (void*)mtb );
        mtb = 0;
        return 1;
    }

    // Start second trender
    if ( ( cnum = daqd.b1->add_consumer( ) ) >= 0 )
    {
        sem_wait( &trender_sem );

        pthread_attr_t attr;
        pthread_attr_init( &attr );
        pthread_attr_setstacksize( &attr, daqd.thread_stack_size );
        pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM );
        pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
        int err_no;

        if ( err_no = pthread_create( &worker_tid,
                                      &attr,
                                      (void* (*)(void*))trend_worker_static,
                                      (void*)this ) )
        {
            strerror_r( err_no, errmsgbuf, sizeof( errmsgbuf ) );
            system_log( 1,
                        "couldn't create second trend worker thread; "
                        "pthread_create() err=%s",
                        errmsgbuf );
            abort( );
        }

        if ( err_no = pthread_create( &consumer,
                                      &attr,
                                      (void* (*)(void*))trend_static,
                                      (void*)this ) )
        {
            strerror_r( err_no, errmsgbuf, sizeof( errmsgbuf ) );
            system_log( 1,
                        "couldn't create second trend consumer thread; "
                        "pthread_create() err=%s",
                        errmsgbuf );

            // FIXME: have to cancel minute trend thread here first !!!
            abort( );

            daqd.b1->delete_consumer( cnum );
            cnum = 0;
            tb->~circ_buffer( );
            free( (void*)tb );
            tb = 0;
            mtb->~circ_buffer( );
            free( (void*)mtb );
            mtb = 0;
            pthread_attr_destroy( &attr );
            return 1;
        }
        pthread_attr_destroy( &attr );
        DEBUG( 2,
               cerr << "second trend consumer created; tid=" << consumer
                    << endl );
    }
    else
    {
        *yyout << "couldn't add trend consumer" << endl;

        // FIXME: have to cancel minute trend thread here first !!!
        abort( );

        tb->~circ_buffer( );
        free( (void*)tb );
        tb = 0;
        mtb->~circ_buffer( );
        free( (void*)mtb );
        mtb = 0;
        return 1;
    }

    return 0;
}

int
trender_c::start_raw_minute_trend_saver( ostream* yyout )
{
    if ( !this->tb )
    {
        *yyout << "please start trend first" << endl;
        return 1;
    }

    // Start raw minute trend saver
    assert( mtb );
    assert( tb );
    if ( ( raw_msaver_cnum = mtb->add_consumer( ) ) < 0 )
    {
        *yyout << "start_raw_trend_saver: too many minute trend consumers, "
                  "saver was not started"
               << endl;
        return 1;
    }
    // error message buffer
    char errmsgbuf[ 80 ];

    pthread_attr_t attr;
    pthread_attr_init( &attr );
    pthread_attr_setstacksize( &attr, daqd.thread_stack_size );
    pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

    /* Start raw minute trend file saver thread */
    int err_no;
    if ( err_no = pthread_create(
             &mtraw,
             &attr,
             (void* (*)(void*))daqd.trender.raw_minute_trend_saver_static,
             (void*)this ) )
    {
        strerror_r( err_no, errmsgbuf, sizeof( errmsgbuf ) );
        system_log( 1,
                    "couldn't create raw minute trend saver thread; "
                    "pthread_create() err=%s",
                    errmsgbuf );

        // FIXME: have to cancel frame saver thread here
        abort( );

        mtb->delete_consumer( raw_msaver_cnum );
        raw_msaver_cnum = 0;
        pthread_attr_destroy( &attr );
        return 1;
    }
    pthread_attr_destroy( &attr );
    DEBUG( 2,
           cerr << "raw minute trend saver thread started; tid=" << mtraw
                << endl );
    return 0;
}

int
trender_c::start_minute_trend_saver( ostream* yyout )
{
    if ( !this->tb )
    {
        *yyout << "please start trend first" << endl;
        return 1;
    }

    // Start minute trend saver
    {
        assert( mtb );
        assert( tb );
        if ( ( msaver_cnum = mtb->add_consumer( ) ) < 0 )
        {
            *yyout << "start_trend_saver: too many minute trend consumers, "
                      "saver was not started"
                   << endl;
            return 1;
        }

        // error message buffer
        char errmsgbuf[ 80 ];

        sem_wait( &minute_frame_saver_sem );
        pthread_attr_t attr;
        pthread_attr_init( &attr );
        pthread_attr_setstacksize( &attr, daqd.thread_stack_size );
        pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM );
        pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

        /* Start minute trend frame saver thread */
        int err_no;
        if ( err_no = pthread_create(
                 &mtsaver,
                 &attr,
                 (void* (*)(void*))daqd.trender.minute_trend_framer_static,
                 (void*)this ) )
        {
            strerror_r( err_no, errmsgbuf, sizeof( errmsgbuf ) );
            system_log( 1,
                        "couldn't create minute trend framer thread; "
                        "pthread_create() err=%s",
                        errmsgbuf );

            // FIXME: have to cancel frame saver thread here
            abort( );

            mtb->delete_consumer( msaver_cnum );
            msaver_cnum = 0;
            pthread_attr_destroy( &attr );
            return 1;
        }
        pthread_attr_destroy( &attr );
        DEBUG( 2,
               cerr << "minute trend framer thread started; tid=" << mtsaver
                    << endl );
    }

    return 0;
}

int
trender_c::start_trend_saver( ostream* yyout )
{
    if ( !this->tb )
    {
        *yyout << "please start trend first" << endl;
        return 1;
    }

    // Start trend saver
    assert( tb );
    if ( ( saver_cnum = tb->add_consumer( ) ) < 0 )
    {
        *yyout << "start_trend_saver: too many trend consumers, saver was not "
                  "started"
               << endl;
        return 1;
    }

    // error message buffer
    char errmsgbuf[ 80 ];

    pthread_attr_t attr;
    pthread_attr_init( &attr );
    pthread_attr_setstacksize( &attr, daqd.thread_stack_size );
    pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

    if ( ascii_output )
    {
        fout = new ofstream( "trend.data", ios::out );
        if ( !fout )
        {
            *yyout << "failed to create file `trend.data'" << endl;
            return 1;
        }

        /* Start trend saver consumer thread */
        int err_no;
        if ( err_no =
                 pthread_create( &tsaver,
                                 &attr,
                                 (void* (*)(void*))daqd.trender.saver_static,
                                 (void*)this ) )
        {
            strerror_r( err_no, errmsgbuf, sizeof( errmsgbuf ) );
            system_log( 1,
                        "couldn't create ascii trend saver thread; "
                        "pthread_create() err=%s",
                        errmsgbuf );
            tb->delete_consumer( saver_cnum );
            pthread_attr_destroy( &attr );
            return 1;
        }
        pthread_attr_destroy( &attr );
        DEBUG( 2,
               cerr << "ASCII trend saver thread started; tid=" << tsaver
                    << endl );
    }
    else
    {
        sem_wait( &frame_saver_sem );

        /* Start second trend framer thread */
        int err_no;
        if ( err_no = pthread_create(
                 &tsaver,
                 &attr,
                 (void* (*)(void*))daqd.trender.trend_framer_static,
                 (void*)this ) )
        {
            strerror_r( err_no, errmsgbuf, sizeof( errmsgbuf ) );
            system_log( 1,
                        "couldn't create second trend framer thread; "
                        "pthread_create() err=%s",
                        errmsgbuf );
            tb->delete_consumer( saver_cnum );
            pthread_attr_destroy( &attr );
            return 1;
        }
        pthread_attr_destroy( &attr );
        DEBUG( 2,
               cerr << "second trend framer thread started; tid=" << tsaver
                    << endl );
    }

    return 0;
}

int
trender_c::kill_trend( ostream* yyout )
{
    char         buf[ 1 ];
    circ_buffer* oldb;

#ifdef not_def

    if ( !tb )
    {
        *yyout << "trend is not running" << endl;
        return 1;
    }

    /* Put zero-length block in the queue
       and wait till the saver is done */
    tb->put( buf, 0 );
    pthread_join( tsaver, NULL );
    return 0;

#endif

    *yyout << "Not implemented" << endl;
    return 1;
}
