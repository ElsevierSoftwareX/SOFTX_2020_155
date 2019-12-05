//
// Created by jonathan.hanks on 8/22/19.
//
#include "fe_stream_generator.hh"

bool
is_data_type_valid( int data_type )
{
    switch ( static_cast< daq_data_t >( data_type ) )
    {
    case _16bit_integer:
    case _32bit_integer:
    case _64bit_integer:
    case _32bit_float:
    case _64bit_double:
    case _32bit_uint:
        return true;
    case _32bit_complex:
    default:
        break;
    }
    return false;
}

GeneratorPtr
create_generator( const std::string& generator, const SimChannel& ch )
{
    if ( ch.data_type( ) != 2 )
        throw std::runtime_error(
            "Invalid/unsupported data type for a generator" );
    if ( generator == "gps_sec" )
    {
        return GeneratorPtr( new Generators::GPSSecondGenerator( ch ) );
    }
    throw std::runtime_error( "Unknown generator type" );
}

GeneratorPtr
create_generator( const std::string& channel_name )
{
    std::vector< std::string > parts = split( channel_name, "--" );
    if ( parts.size( ) < 4 )
    {
        std::cerr << "channel_name = " << channel_name << std::endl;
        throw std::runtime_error(
            "Generator name has too few parts, invalid input" );
    }
    int data_type = 0;
    {
        std::istringstream is( parts[ parts.size( ) - 2 ] );
        is >> data_type;
    }
    int rate = 0;
    {
        std::istringstream is( parts[ parts.size( ) - 1 ] );
        is >> rate;
    }
    if ( !is_data_type_valid( data_type ) || rate < 16 )
        throw std::runtime_error( "Invalid data type or rate found" );
    std::string& base = parts[ 0 ];
    std::string& name = parts[ 1 ];
    int          arg_count =
        parts.size( ) - 4; // ignore base channel name, data type, rate
    if ( name == "gpssoff1p" && arg_count == 1 )
    {
        std::istringstream is( parts[ 2 ] );
        int                offset = 0;
        is >> offset;
        return create_generic_generator< Generators::GPSSecondWithOffset >(
            data_type, SimChannel( base, data_type, rate, 0 ), offset );
    }
    else if ( name == "gpssmd100koff1p" && arg_count == 1 )
    {
        std::istringstream is( parts[ 2 ] );
        int                offset = 0;
        is >> offset;
        return create_generic_generator< Generators::GPSMod100kSecWithOffset >(
            data_type, SimChannel( base, data_type, rate, 0 ), offset );
    }
    else if ( name == "gpssmd100koffc1p" && arg_count == 1 )
    {
        std::istringstream is( parts[ 2 ] );
        int                offset = 0;
        is >> offset;
        return create_generic_generator<
            Generators::GPSMod100kSecWithOffsetAndCycle >(
            data_type, SimChannel( base, data_type, rate, 0 ), offset );
    }
    else if ( name == "gpssmd30koff1p" && arg_count == 1 )
    {
        std::istringstream is( parts[ 2 ] );
        int                offset = 0;
        is >> offset;
        return create_generic_generator< Generators::GPSMod30kSecWithOffset >(
            data_type, SimChannel( base, data_type, rate, 0 ), offset );
    }
    else if ( name == "gpssmd100offc1p" && arg_count == 1 )
    {
        std::istringstream is( parts[ 2 ] );
        int                offset = 0;
        is >> offset;
        return create_generic_generator<
            Generators::GPSMod100SecWithOffsetAndCycle >(
            data_type, SimChannel( base, data_type, rate, 0 ), offset );
    }
    else
    {
        throw std::runtime_error( "Unknown generator type" );
    }
}
