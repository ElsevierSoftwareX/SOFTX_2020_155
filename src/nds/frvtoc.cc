// Validate frame file's table of contents
//
#include <stdio.h>
#include "framecpp/frame.hh"
#include "framecpp/framereadplan.hh"
#include "mmstream.hh"

static void
error_watch( const string& msg )
{
    fprintf( stderr, "framecpp error: %s", msg.c_str( ) );
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
        std::cout << chname_iter->first.c_str( ) << std::endl;
        names.push_back( chname_iter->first.c_str( ) );
    }

    try
    {
        reader->daqTriggerADC( names );
    }
    catch ( ... )
    {
        fputs( "Failed to activate ADC channels\n", stderr );
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
            printf( "\"%s\"; dataValid=0x%x\n",
                    adc[ adccntr ]->getName( ).c_str( ),
                    adc[ adccntr ]->getDataValid( ) );
            std::cout << "\"" << adc[ adccntr ]->getName( ) << "\""
                      << "; dataValid=0x" << hex
                      << adc[ adccntr ]->getDataValid( ) << endl;
            ;
        }
    }
}
