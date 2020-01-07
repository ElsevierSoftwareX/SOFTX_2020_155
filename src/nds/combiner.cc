/*
  combiner -- Concatenate frame files and compress.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <vector>

#include <fstream>
#include <iostream>
#include <framecpp/frame.hh>
#include <framecpp/framewritertoc.hh>
#include <framecpp/framereader.hh>
#include <framecpp/tocreader.hh>
#include <framecpp/errors.hh>
#include "framedir/framedir.hh"

// The name of the program executable
char* programname = 0;

// Input file wilcard pattern
char* ifdir = 0;

// input frame directory
FrameDir* fdir = 0;

// output frame file name
char* ofname = 0;

// output frame file
std::ofstream* ofs = 0;

bool verbose = false;

// wipe the output file and exit
void
cleanup( int foo )
{
    unlink( ofname );
    exit( 1 );
}

void
fatal( std::string msg )
{
    std::cerr << msg << std::endl;
    cleanup( 0 );
}

void
error_watch( const std::string& msg )
{
    std::cerr << "error: " << msg << std::endl;
}

using namespace std;

void
usage( char* f )
{
    if ( !strncasecmp( f, "-h", 2 ) || !strncasecmp( f, "--h", 3 ) )
    {
        cout << "Combine and compress frame files." << endl;
        cout << "Usage: " << (string)programname
             << " [-d frame_wildcard] [ -o outframe ]" << endl;
        cout << "Where `frame_wildcard' is a frame file shell wildcard "
                "pattern,\n"
             << "for which the default is `*', ie. all files in current "
                "directory.\n"
             << "`outframe' is an output frame file name (frame.out is the "
                "dafault).\n";
        exit( 0 );
    }
}

int
main( int argc, char* argv[] )
{
    /* Determine program name (strip filesystem path) */
    for ( programname = argv[ 0 ] + strlen( argv[ 0 ] ) - 1;
          programname != argv[ 0 ] && *programname != '/';
          programname-- )
        ;

    if ( *programname == '/' )
        programname++;

    //  usage help
    if ( argc > 1 )
        usage( argv[ 1 ] );

    bool d_seen = false;

    // clean up the output file on the termination
    signal( SIGTERM, cleanup );
    signal( SIGINT, cleanup );

    // parse command line arguments
    for ( int i = 1; i < argc; i++ )
    {
        if ( !strcmp( argv[ i ], "-d" ) )
        {
            d_seen = true;
            i++;
            if ( i < argc )
                fdir = new FrameDir( ifdir = argv[ i ] );
        }
        else if ( !strcmp( argv[ i ], "-o" ) )
        {
            i++;
            if ( i < argc )
            {
                int ofd = strcmp( argv[ i ], "-" )
                    ? open( argv[ i ], O_CREAT | O_WRONLY | O_TRUNC, 0644 )
                    : 1;
                if ( ofd < 0 )
                    fatal( "could not open `" + (string)argv[ i ] +
                           "' for output" );
#if __GNUC__ >= 3
                FILE*                          cfile = fdopen( ofd, "w" );
                std::ofstream::__filebuf_type* outbuf =
                    new std::ofstream::__filebuf_type( cfile,
                                                       std::ios_base::out );
                std::ofstream::__streambuf_type* oldoutbuf =
                    ( (std::ofstream::__ios_type*)ofs )->rdbuf( outbuf );
                delete oldoutbuf;
#else
                ofs = new std::ofstream( ofd );
#endif
                ofname = argv[ i ];
            }
        }
        else if ( !strcmp( argv[ i ], "-v" ) )
        {
            verbose = true;
        }
        else
        {
            fatal( "invalid argument `" + std::string( argv[ i ] ) + "'" );
        }
    }

    // input file glob pattern default to the star
    if ( !d_seen )
        fdir = new FrameDir( ifdir = "*" );

    // bail out if nothing matched
    if ( !fdir || fdir->end( ) == fdir->begin( ) )
        fatal( "no input frame files matched `" + std::string( ifdir ) + "'" );

    // output frame file default
    if ( !ofs )
        ofs = new ofstream( ofname = "frame.out" );

    FrameCPP::FrameWriterTOC fw( *ofs );

    // cat all the frames in all the files
    for ( FrameDir::file_iterator iter = fdir->begin( ); iter != fdir->end( );
          iter++ )
    { // over all frame files
        if ( verbose )
            cout << iter->second.getFile( ) << endl;
        ifstream              in( iter->second.getFile( ) );
        FrameCPP::FrameReader fr( in );
        fr.setErrorWatch( error_watch );
        try
        {
            for ( ;; )
            { // over all frames in the file
                FrameCPP::Frame* frame = fr.readFrame( );
                if ( !frame )
                    break;
                fw.writeFrame( *frame );
                delete frame;
            }
        }
        catch ( read_failure& e )
        {
            if ( e.code( ) != fr.ENDOFFILE )
                fatal( e.what( ) );
        }
    }
    fw.close( ); // build TOC
    ofs->close( );
    return 0;
}
