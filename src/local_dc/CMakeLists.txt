add_executable(local_dc local_dc.c ${CMAKE_CURRENT_SOURCE_DIR}/../drv/rfm.c)

target_link_libraries(local_dc PUBLIC
        args
        util)

configure_file(test_local_dc.sh.in test_local_dc.sh @ONLY)

add_executable(test_local_dc_unit_tests tests/test_local_dc_unit_tests.c)
target_include_directories(test_local_dc_unit_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME "test_local_dc"
        COMMAND /bin/bash ./test_local_dc.sh
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

add_test(NAME "test_local_dc_unit_tests"
        COMMAND ./test_local_dc_unit_tests
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS local_dc DESTINATION bin)

