add_executable(local_dc local_dc.c ${CMAKE_CURRENT_SOURCE_DIR}/../drv/rfm.c)

target_link_libraries(local_dc PUBLIC util)

configure_file(test_local_dc.sh.in test_local_dc.sh @ONLY)

add_test(NAME "test_local_dc"
        COMMAND /bin/bash ./test_local_dc.sh
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS local_dc DESTINATION bin)

