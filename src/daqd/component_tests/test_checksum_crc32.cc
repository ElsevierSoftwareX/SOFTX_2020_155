//
// Created by jonathan.hanks on 5/21/18.
//
#include <iostream>
#include <string>
#include <vector>
#include <utility>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "checksum_crc32.hh"

typedef std::pair< std::string, uint32_t > test_type;
typedef std::vector< test_type >           test_list;

std::string
sanitize( const std::string& input )
{
    std::string safe( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123"
                      "456789!@#$%^&*()_+~`" );
    std::vector< char > tmp( input.size( ) );
    for ( int i = 0; i < input.size( ); ++i )
    {
        char ch = input[ i ];
        if ( safe.find( ch ) == std::string::npos )
        {
            ch = ' ';
        }
        tmp[ i ] = ch;
    }
    return std::string( tmp.data( ), tmp.size( ) );
}

test_list
generate_inputs( )
{
    test_list inputs;

    inputs.push_back( std::make_pair< std::string, uint32_t >(
        "abcdefghijklmnopqrstuvwxyz", 0xa1b937a8 ) );
    inputs.push_back( std::make_pair< std::string, uint32_t >(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", 0x7073ed5a ) );
    inputs.push_back( std::make_pair< std::string, uint32_t >(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890",
        0x207fc642 ) );
    inputs.push_back(
        std::make_pair< std::string, uint32_t >( "1234", 0xd5868303 ) );

    {
        std::vector< char > tmp( 256 );
        for ( int i = 0; i < 256; ++i )
        {
            tmp.push_back( static_cast< char >( i ) );
        }
        inputs.push_back( std::make_pair< std::string, uint32_t >(
            std::string( tmp.data( ), tmp.size( ) ), 0x430ee578 ) );
    }
    return inputs;
}

TEST_CASE( "Generate the crc32 class data" )
{
    test_list inputs = generate_inputs( );

    for ( int i = 0; i < inputs.size( ); ++i )
    {
        test_type      input = inputs[ i ];
        checksum_crc32 sum;
        sum.add( input.first );
        std::cout << sanitize( input.first ) << " = 0x" << std::hex
                  << sum.result( ) << std::endl;
    }
}

TEST_CASE( "Test the crc32 class" )
{
    test_list inputs = generate_inputs( );

    for ( int i = 0; i < inputs.size( ); ++i )
    {
        test_type      input = inputs[ i ];
        checksum_crc32 sum;
        sum.add( input.first );
        REQUIRE( sum.result( ) == input.second );
    }
}