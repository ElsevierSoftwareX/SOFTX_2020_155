//
// Created by jonathan.hanks on 3/26/20.
//
#include "catch.hpp"

#include "run_number_internal.hh"
#include "run_number.hh"

TEST_CASE( "parse_connection_string parses address pairs" )
{
    auto result = daqd_run_number::parse_connection_string( "127.0.0.1" );
    REQUIRE( result.address == "127.0.0.1" );
    REQUIRE( result.port == "5556" );

    result = daqd_run_number::parse_connection_string( "127.0.0.1:" );
    REQUIRE( result.address == "127.0.0.1" );
    REQUIRE( result.port == "5556" );

    result = daqd_run_number::parse_connection_string( "127.0.0.1:9000" );
    REQUIRE( result.address == "127.0.0.1" );
    REQUIRE( result.port == "9000" );

    result = daqd_run_number::parse_connection_string( "*:9000" );
    REQUIRE( result.address == "0.0.0.0" );
    REQUIRE( result.port == "9000" );

    result = daqd_run_number::parse_connection_string( "tcp://*:9000" );
    REQUIRE( result.address == "0.0.0.0" );
    REQUIRE( result.port == "9000" );

    result = daqd_run_number::parse_connection_string( "tcp://1.2.3.4:9000" );
    REQUIRE( result.address == "1.2.3.4" );
    REQUIRE( result.port == "9000" );
}
