#include "catch.hpp"

#include "run_number_structs.h"

TEST_CASE("Check run_number_struct sizes") {
    REQUIRE(sizeof(::daqd_run_number_req_v1_t) == (64+4));
    REQUIRE(sizeof(::daqd_run_number_resp_v1_t) == 8);
}