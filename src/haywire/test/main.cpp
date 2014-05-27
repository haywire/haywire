#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE( "Tests are running", "[meta]" ) {
    REQUIRE( 1 == 1 );
}
