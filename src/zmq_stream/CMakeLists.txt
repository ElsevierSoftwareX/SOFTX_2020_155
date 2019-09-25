project(zmq)
cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

include(Cpp11)
include(Cpp11Atomic)
include(FindThreads)
find_package(ZMQ4 REQUIRED)

add_library(zmq_transport STATIC zmq_transport.c)
target_include_directories(zmq_transport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(zmq_transport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(zmq_transport PRIVATE zmq4::zmq)

add_executable(zmq_xmit zmq_xmit.c dc_utils.c)
target_link_libraries(zmq_xmit PUBLIC driver::shmem zmq_transport zmq4::zmq ${CMAKE_THREAD_LIBS_INIT})

add_executable(zmq_recv zmq_recv.cc dc_utils.c)
# target_compile_options(zmq_rcv_ix_xmit_cxx PUBLIC -fsanitize=address)
# target_compile_options(zmq_rcv_ix_xmit_cxx PUBLIC -fsanitize=thread -no-pie)
target_link_libraries(zmq_recv PUBLIC
        # asan
        # tsan
        zmq_transport
        driver::shmem
        zmq4::zmq
        pv::simple_pv
        ${CMAKE_THREAD_LIBS_INIT}
        # -no-pie
        )

configure_file(test_zmq_xmit_recv.sh.in test_zmq_xmit_recv.sh @ONLY)

add_test(NAME "test_zmq_xmit_recv"
        COMMAND /bin/bash ./test_zmq_xmit_recv.sh
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS zmq_recv DESTINATION bin)
install(TARGETS zmq_xmit DESTINATION bin)
