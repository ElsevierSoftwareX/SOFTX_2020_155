#include <stdio.h>
#include "framecpp/frame.hh"
#include "framecpp/framereadplan.hh"
#include "mmstream.hh"

static void
error_watch( const string& msg )
{
    fprintf( stderr, "framecpp error: %s", msg.c_str( ) );
}

void
doAdc( FrameCPP::AdcData& adc, unsigned long gps )
{
    short* data = (short*)adc.refData( )[ 0 ]->getData( );
    int    tsn =
        adc.refData( )[ 0 ]->getNData( ); // Number of samples in the segment
    int samplingRate = (int)adc.getSampleRate( );
    int seconds =
        tsn / samplingRate; // How many seconds of data we have got here

    for ( int second = 0; second < seconds; second++ )
    {
        double txAve = (double)data[ 0 ];
        double txMax = (double)data[ 0 ];
        double txMin = (double)data[ 0 ];
        short* j = data + samplingRate * second;

        for ( unsigned int i = 1; i < samplingRate; i++ )
        {
            txAve += (double)j[ i ];
            if ( ( (double)j[ i ] ) < txMin )
                txMin = (double)j[ i ];
            if ( ( (double)j[ i ] ) > txMax )
                txMax = (double)j[ i ];
        }
        txAve /= samplingRate;
        double cut =
            ( txMax - txMin ) / ( 2.0 * 2.92 ); // Set cut threshold (10:3 -> 5)
        int txChS = (int)( 100.0 *
                           ( ( ( ( txMax + txMin ) / 2.0 ) - txAve ) /
                             txAve ) ); // Construct checksum
#ifdef DEBUG_IRIGB
        cout << "\n..................................."
             << "\nSeries average: " << (int)txAve
             << "\nSeries Maximum: " << (int)txMax
             << "\nSeries Minimum: " << (int)txMin << "\nSeries AveDiff: "
             << (int)( ( ( txMax + txMin ) / 2 ) - txAve )
             << "\nCheckSum:       " << (float)txChS
             << "\nCut level:      " << (int)cut
             << "\n..................................." << endl;
#endif

        bool g[ samplingRate ];
        bool b;
        int  i = 0, t = 0, ii = 0, l = 0, h = 0;
        int  Err = 0;
        int  secs = 0, mins = 0, hours = 0, days = 0;
        int  sec = 0, min = 0, hour = 0, day = 0;
        char I[ 105 ];
        char da[ 4 ], ho[ 2 ], mi[ 2 ], se[ 2 ];

        struct tm tms;
        time_t    utc = ( time_t )( gps + 315964819 - 32 ) + second;
        gmtime_r( &utc, &tms );
        tms.tm_mon++;
        char UTCtime[ 256 ];
        sprintf( UTCtime,
                 "%02d:%02d:%02d:%02d",
                 day = tms.tm_yday,
                 hour = tms.tm_hour,
                 min = tms.tm_min,
                 sec = tms.tm_sec );

        for ( i = 0; i < samplingRate;
              i++ ) // Cut between the high sine and the low sine
            if ( abs( j[ i ] - txAve ) > cut )
                g[ i ] = true;
            else
                g[ i ] = false;

        for ( i = 0; i < samplingRate; i++ )
        { // Erase the little gaps where the sines go 0...
            if ( g[ i - 2 ] && g[ i + 2 ] && !g[ i ] )
                g[ i ] = true;
            if ( !g[ i - 2 ] && !g[ i + 2 ] && g[ i ] )
                g[ i ] = false;
        }
        for ( i = 0; i < samplingRate; i++ )
        { // Erase the little gaps where the sines go 0...
            if ( g[ i - 2 ] && g[ i + 2 ] && !g[ i ] )
                g[ i ] = true;
            if ( !g[ i - 2 ] && !g[ i + 2 ] && g[ i ] )
                g[ i ] = false;
        }

        h = 0;
        l = 0;
        b = false;
        for ( i = 0; i < samplingRate; i++ )
        { // Pulse duty factor measurement and bit recognition
            if ( !g[ i ] )
                l++;
            if ( g[ i ] )
                h++;
            if ( !g[ i ] && b )
                b = false;
            if ( g[ i ] && !b )
            {
                t = (int)floor( 100.0 * ( (float)h / (float)l ) );
                I[ ii ] = 'E';
                if ( abs( t - 100 ) < 30 )
                    I[ ii ] = '1';
                if ( abs( t - 25 ) < 10 )
                    I[ ii ] = '0';
                if ( abs( t - 400 ) < 100 )
                    I[ ii ] = 'P';
#ifdef DEBUG_IRIGB
                    // cout << ii << " - " << h << "/" << l << "=> " << I[ii] <<
                    // endl;
#endif
                h = 0;
                l = 0;
                b = true;
                if ( I[ 0 ] == 'P' )
                    ii++;
                if ( ( ii == 0 ) && ( i > 100 ) )
                {
                    cerr << "ERROR! Start marker was not found" << endl;
                    ii = 0;
                    goto end;
                }
            }
        }

        Err = 0;
        for ( t = 9; t < 60; t += 10 )
            if ( I[ t ] != 'P' )
                Err = 1;
        if ( I[ 5 ] != '0' )
            Err = 2;
        for ( t = 14; t < 55; t += 10 )
            if ( I[ t ] != '0' )
                Err = 3;
#ifdef DEBUG_IRIGB
        cout << "Error code: " << Err << endl;
#endif
        if ( Err > 0 )
        {
            cerr << "ERROR! The time code does not contain the standard "
                    "separators"
                 << Err << endl;
            ii = 0;
            continue;
        }

#ifdef DEBUG_IRIGB
        if ( Err == 0 )
        {
            cerr << "Check Bits are Fine! " << endl;
        }
#endif

        for ( l = 0; l < ii; l++ )
            I[ l ] -=
                48; // Primitive character to digit conversion...bad Szabi bad

        secs = I[ 1 ] + 2 * I[ 2 ] + 4 * I[ 3 ] + 8 * I[ 4 ] + 10 * I[ 6 ] +
            20 * I[ 7 ] + 40 * I[ 8 ];
        secs -= 13; // Leap second correction (-13)
        mins = I[ 10 ] + 2 * I[ 11 ] + 4 * I[ 12 ] + 8 * I[ 13 ] +
            10 * I[ 15 ] + 20 * I[ 16 ] + 40 * I[ 17 ];
        if ( secs < 0 )
        {
            secs += 60;
            mins--;
        }
        hours = I[ 20 ] + 2 * I[ 21 ] + 4 * I[ 22 ] + 8 * I[ 23 ] +
            10 * I[ 25 ] + 20 * I[ 26 ] + 40 * I[ 27 ];
        if ( mins == -1 )
        {
            mins = 59;
            hours--;
        }
        days = I[ 30 ] + 2 * I[ 31 ] + 4 * I[ 32 ] + 8 * I[ 33 ] +
            10 * I[ 35 ] + 20 * I[ 36 ] + 40 * I[ 37 ] + 80 * I[ 38 ] +
            100 * I[ 40 ] + 200 * I[ 41 ] - 1;
        if ( hours == -1 )
        {
            hours = 23;
            days--;
            if ( days < 0 )
                days = 365 + days;
        }

        ii = ( day - days ) + ( hour - hours ) + ( min - mins ) +
            ( sec -
              secs ); // Check whether the IRIG-B code agrees with the DAQ stamp
        if ( ii == 0 )
            cout << "OK  ";
        else
            cout << "BAD!!!!!!  ";
        cout << UTCtime << "=<" << ii << ">=" << days << ":" << hours << ":"
             << mins << ":" << secs << endl;
    end:;
    }
}

main( int argc, char* argv[] )
{
    if ( argc < 2 )
    {
        fputs( "Usage: fririgb <frame file name>\n", stderr );
        exit( 1 );
    }
    mm_istream in( argv[ 1 ] );
    if ( !in )
    {
        fprintf( stderr, "Failed to mmap file `%s'\n", argv[ 1 ] );
        exit( 1 );
    }
    FrameCPP::FrameReadPlan* reader = 0; // frame reader
    try
    {
        reader = new FrameCPP::FrameReadPlan( in );
    }
    catch ( ... )
    {
        fputs( "Failed to create frame reader\n", stderr );
        exit( 1 );
    }
    reader->setErrorWatch( error_watch );
    vector< string > names;
    // find all IRIGB channels
    for ( FrameCPP::TOC::ANPI chname_iter =
              reader->getAdcNamePositionMap( ).begin( );
          chname_iter != reader->getAdcNamePositionMap( ).end( );
          chname_iter++ )
    {
        if ( strstr( chname_iter->first.c_str( ), "IRIGB" ) )
            names.push_back( chname_iter->first.c_str( ) );
    }
    // names.push_back("H0:GDS-IRIGB_LVEA");
    //    names.push_back("H1:GDS-IRIGB_EY");

    try
    {
        reader->daqTriggerADC( names );
    }
    catch ( ... )
    {
        fputs( "Failed to activate H0:GDS-IRIGB_LVEA and H1:GDS-IRIGB_EY\n",
               stderr );
        exit( 1 );
    }
    // Iterate over frames in the frame file
    int frames = reader->getFrame( ).size( );
    for ( unsigned long j = 0; j < frames; j++ )
    {
        FrameCPP::Frame* frame = 0;
        try
        {
            frame = &reader->readFrame( in, j );
        }
        catch ( ... )
        {
            fprintf( stderr, "Failed to read frame number %d\n", j );
            exit( 1 );
        }
        FrameCPP::RawData::AdcDataContainer& adc(
            frame->getRawData( )->refAdc( ) );
        const FrameCPP::Time& gps = frame->getGTime( );
        printf( "Frame %d; gps %d\n", j, gps.getSec( ) );
        for ( int adccntr = 0; adccntr < adc.getSize( ); adccntr++ )
        {
            printf( "%s; dataValid=0x%x\n",
                    adc[ adccntr ]->getName( ).c_str( ),
                    adc[ adccntr ]->getDataValid( ) );
            doAdc( *adc[ adccntr ], gps.getSec( ) );
        }
    }
}
