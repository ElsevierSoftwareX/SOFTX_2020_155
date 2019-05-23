cmake_minimum_required(VERSION 3.8)
project(transfer)

set(CMAKE_CXX_STANDARD 11)

SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/config" )

FIND_PACKAGE(EPICSv4 REQUIRED)

add_executable(epicsv4_stream
        epicsv4_stream.cc
        )
target_compile_options(epicsv4_stream PUBLIC
        -Wno-deprecated-declarations
        -O0
        -g3
        )
target_include_directories(epicsv4_stream PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        )
target_link_libraries(epicsv4_stream PUBLIC
        EPICSv4::pvDatabaseCPP
        EPICSv4::normativeTypesCPP
        epics::ca
        m
        readline
        pthread
        )

