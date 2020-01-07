#include <stdio.h>
#include "framecpp/dump.hh"
#include "framecpp/frame.hh"
#include "framecpp/tocreader.hh"
#include "mmstream.hh"

static void
error_watch( const string& msg )
{
    fprintf( stderr, "framecpp error: %s", msg.c_str( ) );
}

main( int argc, char* argv[] )
{
    if ( argc < 3 )
    {
        fputs( "Usage: frdiff file1 file2\n", stderr );
        exit( 1 );
    }
    mm_istream in1( argv[ 1 ] );
    if ( !in1 )
    {
        fprintf( stderr, "Failed to mmap file `%s'\n", argv[ 1 ] );
        exit( 1 );
    }
    FrameCPP::TOCReader* reader1 = 0; // frame reader
    try
    {
        reader1 = new FrameCPP::TOCReader( in1 );
    }
    catch ( ... )
    {
        fputs( "Failed to create frame reader\n", stderr );
        exit( 1 );
    }
    reader1->setErrorWatch( error_watch );

    mm_istream in2( argv[ 2 ] );
    if ( !in1 )
    {
        fprintf( stderr, "Failed to mmap file `%s'\n", argv[ 1 ] );
        exit( 1 );
    }
    FrameCPP::TOCReader* reader2 = 0; // frame reader
    try
    {
        reader2 = new FrameCPP::TOCReader( in2 );
    }
    catch ( ... )
    {
        fputs( "Failed to create frame reader\n", stderr );
        exit( 1 );
    }
    reader2->setErrorWatch( error_watch );
    int frames1 = reader1->getFrame( ).size( );
    int frames2 = reader2->getFrame( ).size( );

    // See if the number of frames matches
    if ( frames1 != frames2 )
    {
        fprintf( stderr,
                 "%s has %d frames, %s has %d frames\n",
                 argv[ 1 ],
                 frames1,
                 argv[ 2 ],
                 frames2 );
        exit( 1 );
    }
    //  printf("%d frames\n", frames1);
    // Iterate over frames in the frame file
    unsigned int adcs_processed = 0;
    unsigned int mismatch = 0;
    unsigned int bytes_diff = 0;
    for ( unsigned long j = 0; j < frames1; j++ )
    {
        for ( FrameCPP::TOC::ANPI chname_iter =
                  reader1->getAdcNamePositionMap( ).begin( );
              chname_iter != reader1->getAdcNamePositionMap( ).end( );
              chname_iter++, adcs_processed++ )
        {
            FrameCPP::AdcData* adc1 = 0;
            FrameCPP::AdcData* adc2 = 0;
            try
            {
                adc1 = reader1->readADC( j, chname_iter->first );
            }
            catch ( ... )
            {
                fprintf( stderr,
                         "Failed to read frame %d from `%s'\n",
                         j,
                         argv[ 1 ] );
                exit( 1 );
            }
            try
            {
                adc2 = reader2->readADC( j, chname_iter->first );
            }
            catch ( ... )
            {
                fprintf( stderr,
                         "Only in `%s': %s\n",
                         j,
                         argv[ 1 ],
                         chname_iter->first.c_str( ) );
                exit( 1 );
            }
            unsigned char* c1 =
                (unsigned char*)adc1->refData( )[ 0 ]->getData( );
            unsigned char* c2 =
                (unsigned char*)adc2->refData( )[ 0 ]->getData( );
            unsigned int nbytes = adc1->refData( )[ 0 ]->getNBytes( );
            if ( *adc1 == *adc2 /*&& 0==memcmp(c1, c2, nbytes)*/ )
            {
                ;
            }
            else
            {
                unsigned int ndiff = 0;
                for ( int i = 0; i < nbytes; i++ )
                    ndiff += c1[ i ] != c2[ i ];
                fprintf(
                    stderr, "%s %d\n", chname_iter->first.c_str( ), ndiff );
                bytes_diff += ndiff;
                mismatch++;
            }
            delete adc1;
            delete adc2;
        }
    }
    printf( "%d ADCs total, %d ADCs mismatched, %d bytes differ\n",
            adcs_processed,
            mismatch,
            bytes_diff );
}
