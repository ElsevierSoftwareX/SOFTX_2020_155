project(zmq)
cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

include(FindThreads)
find_package(ZMQ4 REQUIRED)


add_executable(zmq_multi_stream zmq_multi_stream.c ../drv/rfm.c)
target_link_libraries(zmq_multi_stream PUBLIC zmq4::zmq)
target_compile_options(zmq_multi_stream PUBLIC -std=gnu99)


add_executable(zmq_threads zmq_threads.c)
target_link_libraries(zmq_threads PUBLIC zmq Threads::Threads)
target_compile_options(zmq_threads PUBLIC -std=gnu99)

add_executable(zmq_proxy zmq_proxy.c)
target_link_libraries(zmq_proxy PUBLIC zmq4::zmq)
target_compile_options(zmq_proxy PUBLIC -std=gnu99)

add_executable(zmq_proxy_client zmq_proxy_client.c)
target_link_libraries(zmq_proxy_client PUBLIC zmq4::zmq)
target_compile_options(zmq_proxy_client PUBLIC -std=gnu99)

add_executable(zmq_rcv_from_dc zmq_rcv_from_dc.c)
target_link_libraries(zmq_rcv_from_dc PUBLIC zmq4::zmq)
target_compile_options(zmq_rcv_from_dc PUBLIC -std=gnu99)

