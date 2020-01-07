#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "parameter_set.hh"

TEST_CASE( "Test parameter sets", "[param_sets]" )
{
    SECTION( "We can create and use parameter sets" )
    {
        parameter_set params;
        SECTION( "We can add values to a parameter set and get them back" )
        {
            params.set( "dog", "cat" );
            REQUIRE( params.get( "dog", "bat" ) == std::string( "cat" ) );
            params.set( "game", std::string( "ball" ) );
            REQUIRE( params.get( "game", "board" ) == std::string( "ball" ) );
            params.set( "number", "5" );
            REQUIRE( params.get< int >( "number", 6 ) == 5 );

            SECTION( "We can get default values back for items that are not in "
                     "the set" )
            {
                REQUIRE( params.get( "cat", "tiger" ) ==
                         std::string( "tiger" ) );
                REQUIRE( params.get( "num_val", 7 ) == 7 );
            }

            SECTION(
                "We can copy parameter sets and still get the right values" )
            {
                parameter_set other1( params );
                REQUIRE( other1.get( "dog", "bat" ) == std::string( "cat" ) );
            }
        }
    }
}
