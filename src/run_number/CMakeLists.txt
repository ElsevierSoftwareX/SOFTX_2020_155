configure_file(tests/test_run_number_server.sh.in "${CMAKE_CURRENT_BINARY_DIR}/test_run_number_server.sh" @ONLY)

add_executable(run_number_server run_number_main.cc run_number.cc run_number.hh)
target_link_libraries(run_number_server PRIVATE
        args
        ${Boost_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(run_number_server PUBLIC ${Boost_INCLUDE_DIRS})
target_requires_cpp11(run_number_server PUBLIC)

add_library(run_number_client STATIC run_number_client.cc run_number_client.hh)
target_link_libraries(run_number_client PUBLIC ${Boost_LIBRARIES})
target_include_directories(run_number_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(run_number_client PRIVATE ${Boost_INCLUDE_DIRS})
target_requires_cpp11(run_number_client PRIVATE)
add_library(run_number::run_number ALIAS run_number_client)


add_executable(test_run_number
        tests/test_main.cc
        tests/test_run_number.cc
        tests/test_run_number_structs.cc
        tests/test_run_number_internals.cc
        run_number.cc
        run_number.hh)
target_include_directories(test_run_number PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_run_number PRIVATE
        ${Boost_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT})


add_executable(test_run_number_client tests/test_client_main.cc)
target_link_libraries(test_run_number_client
        PUBLIC run_number::run_number
        ${CMAKE_THREAD_LIBS_INIT})

add_executable(run_number_test_client run_number_test_client.cc)
target_link_libraries(run_number_test_client PUBLIC
        args
        run_number_client
        ${CMAKE_THREAD_LIBS_INIT})

add_test(_rn_basic_run_number_tests_ test_run_number)

add_test(NAME test_run_number_server
        COMMAND /bin/bash ./test_run_number_server.sh
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS run_number_server
        DESTINATION bin
        )
