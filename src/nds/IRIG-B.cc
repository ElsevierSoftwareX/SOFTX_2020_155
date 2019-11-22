//#############################################################################
//#  IRIG-B signal decoder program    v0.0
//#
//#  By:   Szabi Marka, 2001
//#
//#############################################################################

#include "IRIG-B.hh"

#ifndef __CINT__

#define PIDCVSHDR                                                              \
    "$Header: "                                                                \
    "/ldcg_server/common/repository_cds/cds/project/daq/nds/IRIG-B.cc,v 1.1 "  \
    "2001/11/13 16:21:47 aivanov Exp $"
#define PIDTITLE "IRIG-B Timing Signal Check"
#include "ProcIdent.hh"
#include <iostream.h>
#include <fstream.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "TSeries.hh"
#include "FSeries.hh"
#include "Dacc.hh"

//--------------------------------------

EXECDAT( IrigB )
#endif // !def(__CINT__)

char Ch[ 255 ] =
    "H0:GDS-IRIGB_LVEA"; // Name of input channel (default, or command line)
char ChFile[ 255 ] = "X"; // Name of input channel (default, or command line)
int  ct = 0;

IrigB::IrigB( int argc, const char* argv[] ) // IrigB  constructor
    : DatEnv( argc, argv ), MaxFrame( 3 )
{

#ifdef DEBUG_IRIGB
    cout << "Usage: Irig-B \n"
         << "        [optional -h provides this help \n"
         << "        [optional -c <channel name>] \n"
         << "        [optional -f (read from file) <filename> Don't forget the "
            "quotes!] \n"
         << "        [optional -n N maximum number of frames to read]" << endl;
#endif
    for ( int i = 1; i < argc; i++ )
    {
        if ( !strcmp( "-h", argv[ i ] ) )
        {
            cout << "Usage: Irig-B \n"
                 << "        [optional -h provides this help \n"
                 << "        [optional -c <channel name>] \n"
                 << "        [optional -f (read from file) <filename> NOT "
                    "IMPLEMENTED YET!] \n"
                 << "        [optional -n N maximum number of frames to read]"
                 << endl;
            exit( 0 );
        }
        if ( !strcmp( "-c", argv[ i ] ) )
        {
            strcpy( Ch, argv[ ++i ] );
#ifdef DEBUG_IRIGB
            cout << "Channel loaded from command line : " << Ch << endl;
#endif
        }
        if ( !strcmp( "-n", argv[ i ] ) )
        {
            MaxFrame = strtol( argv[ ++i ], 0, 0 );
#ifdef DEBUG_IRIGB
            cout << "Maximum number of frames loaded from command line : "
                 << MaxFrame << endl;
#endif
        }
        if ( !strcmp( "-f", argv[ i ] ) )
        {
            strcpy( ChFile, argv[ ++i ] );
            getDacc( ).close( );
            getDacc( ).addFile( ChFile );
#ifdef DEBUG_IRIGB
            cout << "Frames are loaded from : " << ChFile << endl;
#endif
        }
    }
#ifdef DEBUG_IRIGB
    cout << "Irig-B channel watched : " << Ch
         << "\n\n===========================\n\n"
         << endl;
#endif
    getDacc( ).setStride( 1.0 ); // Set the time step to 1 seconds
    getDacc( ).addChannel( Ch ); // Add Irig-B Channel
}

IrigB::~IrigB( )
{
    cout << "IrigB is finished" << endl;
}

void
IrigB::ProcessData( void )
{ // Decodes the time

    bool b;
    bool g[ 17000 ];
    int  j[ 17000 ];
    int  i = 0, t = 0, ii = 0, l = 0, h = 0;
    int  Err = 0;
    int  secs = 0, mins = 0, hours = 0, days = 0;
    int  sec = 0, min = 0, hour = 0, day = 0;
    char I[ 105 ];
    char UTCtime[ 64 ];
    char da[ 4 ], ho[ 2 ], mi[ 2 ], se[ 2 ];

    ct++;
    // system("date -u");
    TSeries* tx = getDacc( ).refData( Ch );
    // float tsd = tx->getInterval();

    double txAve = tx->getAverage( ); // Determine the zero offset
    double txMax = tx->getMaximum( ); // Determine the maximum
    double txMin = tx->getMinimum( ); // Determine the minimum
    double cut =
        ( txMax - txMin ) / ( 2.0 * 2.92 ); // Set cut threshold (10:3 -> 5)
    int txChS = (int)( 100.0 *
                       ( ( ( ( txMax + txMin ) / 2.0 ) - txAve ) /
                         txAve ) ); // Construct checksum
#ifdef DEBUG_IRIGB
    cout << "\n..................................."
         << "\nSeries average: " << (int)txAve
         << "\nSeries Maximum: " << (int)txMax
         << "\nSeries Minimum: " << (int)txMin
         << "\nSeries AveDiff: " << (int)( ( ( txMax + txMin ) / 2 ) - txAve )
         << "\nCheckSum:       " << (float)txChS
         << "\nCut level:      " << (int)cut
         << "\n..................................." << endl;
#endif

    int  tsn = tx->getNSample( ); // Number of samples in the segment
    Time GT = tx->getStartTime( ); // Start time of segment
    TimeStr( GT, UTCtime, "%D:%H:%N:%S" );
    TimeStr( GT, da, "%D" );
    day = atoi( da );
    TimeStr( GT, ho, "%H" );
    hour = atoi( ho );
    TimeStr( GT, mi, "%N" );
    min = atoi( mi );
    TimeStr( GT, se, "%S" );
    sec = atoi( se );
    tx->getData( tsn, j ); // Load time series into array...(max 2 second fits)

    for ( i = 0; i < tsn; i++ ) // Cut between the high sine and the low sine
        if ( abs( j[ i ] - txAve ) > cut )
            g[ i ] = true;
        else
            g[ i ] = false;

    for ( i = 0; i < tsn; i++ )
    { // Erase the little gaps where the sines go 0...
        if ( g[ i - 2 ] && g[ i + 2 ] && !g[ i ] )
            g[ i ] = true;
        if ( !g[ i - 2 ] && !g[ i + 2 ] && g[ i ] )
            g[ i ] = false;
    }
    for ( i = 0; i < tsn; i++ )
    { // Erase the little gaps where the sines go 0...
        if ( g[ i - 2 ] && g[ i + 2 ] && !g[ i ] )
            g[ i ] = true;
        if ( !g[ i - 2 ] && !g[ i + 2 ] && g[ i ] )
            g[ i ] = false;
    }

    h = 0;
    l = 0;
    b = false;
    for ( i = 0; i < tsn; i++ )
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
    // Check for decoding errors...
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
        cerr << "ERROR! The time code does not contain the standard separators"
             << Err << endl;
        ii = 0;
        goto end;
    }

#ifdef DEBUG_IRIGB
    if ( Err == 0 )
    {
        cerr << "Check Bits are Fine! " << endl;
    }
#endif

    for ( l = 0; l < ii; l++ )
        I[ l ] -= 48; // Primitive character to digit conversion...bad Szabi bad

    secs = I[ 1 ] + 2 * I[ 2 ] + 4 * I[ 3 ] + 8 * I[ 4 ] + 10 * I[ 6 ] +
        20 * I[ 7 ] + 40 * I[ 8 ];
    secs -= 13; // Leap second correction (-13)
    mins = I[ 10 ] + 2 * I[ 11 ] + 4 * I[ 12 ] + 8 * I[ 13 ] + 10 * I[ 15 ] +
        20 * I[ 16 ] + 40 * I[ 17 ];
    if ( secs < 0 )
    {
        secs += 60;
        mins--;
    }
    hours = I[ 20 ] + 2 * I[ 21 ] + 4 * I[ 22 ] + 8 * I[ 23 ] + 10 * I[ 25 ] +
        20 * I[ 26 ] + 40 * I[ 27 ];
    if ( mins == -1 )
    {
        mins = 59;
        hours--;
    }
    days = I[ 30 ] + 2 * I[ 31 ] + 4 * I[ 32 ] + 8 * I[ 33 ] + 10 * I[ 35 ] +
        20 * I[ 36 ] + 40 * I[ 37 ] + 80 * I[ 38 ] + 100 * I[ 40 ] +
        200 * I[ 41 ] - 1;
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
    cout << UTCtime << "=<" << ii << ">=" << days << ":" << hours << ":" << mins
         << ":" << secs << endl;

end:
    if ( ct >= MaxFrame )
        exit( Err );
}
