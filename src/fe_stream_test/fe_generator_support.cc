//
// Created by jonathan.hanks on 8/22/19.
//
#include <fstream>

#include "fe_generator_support.hh"

extern "C" {

#include "../drv/crc.c"
}

std::string
cleaned_system_name( const std::string& system_name )
{
    std::vector< char > buf;
    for ( int i = 0; i < system_name.size( ); ++i )
    {
        if ( system_name[ i ] == ':' )
            continue;
        buf.push_back( (char)tolower( system_name[ i ] ) );
    }
    buf.push_back( '\0' );
    return std::string( buf.data( ) );
}

std::string
generate_ini_filename( const std::string& ini_dir,
                       const std::string& system_name )
{
    std::ostringstream ss;
    ss << ini_dir << "/" << system_name << ".ini";
    return ss.str( );
}

std::string
generate_par_filename( const std::string& ini_dir,
                       const std::string& system_name )
{
    std::ostringstream ss;
    ss << ini_dir << "/tpchn_" << system_name << ".par";
    return ss.str( );
}

void
output_ini_files( const std::string&          ini_dir,
                  const std::string&          system_name,
                  std::vector< GeneratorPtr > channels,
                  std::vector< GeneratorPtr > tp_channels,
                  int                         dcuid,
                  int                         model_rate )
{
    using namespace std;

    string   clean_name = cleaned_system_name( system_name );
    string   fname_ini = generate_ini_filename( ini_dir, clean_name );
    string   fname_par = generate_par_filename( ini_dir, clean_name );
    ofstream os_ini( fname_ini.c_str( ) );
    ofstream os_par( fname_par.c_str( ) );
    os_ini << "[default]\ngain=1.0\nacquire=3\ndcuid=" << dcuid
           << "\nifoid=0\n";
    os_ini << "datatype=2\ndatarate=" << model_rate
           << "\noffset=0\nslope=1.0\nunits=undef\n\n";

    vector< GeneratorPtr >::iterator cur = channels.begin( );
    for ( ; cur != channels.end( ); ++cur )
    {
        Generator* gen = ( cur->get( ) );
        ( *cur )->output_ini_entry( os_ini );
    }

    for ( cur = tp_channels.begin( ); cur != tp_channels.end( ); ++cur )
    {
        Generator* gen = ( cur->get( ) );
        ( *cur )->output_par_entry( os_par );
    }
}

unsigned int
calculate_ini_crc( const std::string& ini_dir, const std::string& system_name )
{
    std::string fname_ini =
        generate_ini_filename( ini_dir, cleaned_system_name( system_name ) );
    std::ifstream is( fname_ini.c_str( ), std::ios::binary );

    std::vector< char > buffer( 64 * 1024 );

    size_t       file_size = 0;
    unsigned int file_crc = 0;
    while ( is.read( &buffer[ 0 ], buffer.size( ) ) )
    {
        file_crc = crc_ptr( &buffer[ 0 ], buffer.size( ), file_crc );
        file_size += buffer.size( );
    }
    if ( is.gcount( ) > 0 )
    {
        file_crc = crc_ptr( &buffer[ 0 ], is.gcount( ), file_crc );
        file_size += is.gcount( );
    }
    file_crc = crc_len( file_size, file_crc );
    return file_crc;
}

unsigned int
calculate_crc( const void* buffer, size_t len )
{
    if ( !buffer || len <= 0 )
    {
        return 0;
    }
    return crc_len(
        len,
        crc_ptr( reinterpret_cast< char* >( const_cast< void* >( buffer ) ),
                 len,
                 0 ) );
}