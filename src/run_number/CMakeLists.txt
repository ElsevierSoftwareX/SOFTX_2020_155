project(run_number)
cmake_minimum_required(VERSION 3.0.0)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../config/cmake")

INCLUDE(CheckCXXCompilerFlag)
include(FindThreads)
find_package(ZMQ4 REQUIRED)

find_path(CATCH_PATH catch.hpp
        HINTS /usr/include /usr/local/include ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

CHECK_CXX_COMPILER_FLAG(-std=c++0x HAS_CXX_0X)
CHECK_CXX_COMPILER_FLAG(-std=c++11 HAS_CXX_11)
if (${HAS_CXX_11})
    set(_RN_CHECK_FLAGS "-std=c++11")
elseif(${HAS_CXX_0X})
    set(_RN_CHECK_FLAGS "-std=c++0x")
else(${HAS_CXX_11})
    set(_RN_CHECK_FLAGS "")
endif (${HAS_CXX_11})

add_executable(run_number_server run_number_main.cc run_number.cc run_number.hh)
target_link_libraries(run_number_server PUBLIC zmq)
target_compile_options(run_number_server PRIVATE ${_RN_CHECK_FLAGS})

add_library(run_number_client STATIC run_number_client.cc run_number_client.hh)
target_link_libraries(run_number_client PUBLIC zmq)
target_include_directories(run_number_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_library(run_number::run_number ALIAS run_number_client)

add_executable(test_run_number tests/test_main.cc tests/test_run_number.cc tests/test_run_number_structs.cc run_number.cc run_number.hh)
target_include_directories(test_run_number PRIVATE ${CATCH_PATH})
target_compile_options(test_run_number PRIVATE ${_RN_CHECK_FLAGS})

add_executable(test_run_number_client tests/test_client_main.cc)
target_link_libraries(test_run_number_client PUBLIC run_number::run_number)

add_executable(run_number_test_client run_number_test_client.cc)
target_link_libraries(run_number_test_client PUBLIC run_number_client)

add_test(_rn_basic_run_number_tests_ test_run_number)

install(TARGETS run_number_server
        DESTINATION bin
        )