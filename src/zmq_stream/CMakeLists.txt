project(zmq)
cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

include(Cpp11)
include(FindThreads)
find_package(ZMQ4 REQUIRED)


add_library(zmq_dc_recv STATIC zmq_dc_recv.cc)
target_link_libraries(zmq_dc_recv PUBLIC zmq4::zmq ${CMAKE_THREAD_LIBS_INIT})


add_executable(zmq_multi_stream zmq_multi_stream.c ../drv/rfm.c)
target_link_libraries(zmq_multi_stream PUBLIC zmq4::zmq)
target_compile_options(zmq_multi_stream PUBLIC -std=gnu99)


add_executable(zmq_threads zmq_threads.c)
target_link_libraries(zmq_threads PUBLIC zmq ${CMAKE_THREAD_LIBS_INIT})
target_compile_options(zmq_threads PUBLIC -std=gnu99)

add_executable(zmq_dc zmq_dc.cc)
target_link_libraries(zmq_dc PUBLIC zmq_dc_recv zmq ${CMAKE_THREAD_LIBS_INIT})
target_compile_options(zmq_dc PUBLIC)
target_requires_cpp11(zmq_dc PUBLIC)

add_executable(zmq_proxy zmq_proxy.c)
target_link_libraries(zmq_proxy PUBLIC zmq4::zmq)
target_compile_options(zmq_proxy PUBLIC -std=gnu99)

add_executable(zmq_proxy_client zmq_proxy_client.c)
target_link_libraries(zmq_proxy_client PUBLIC zmq4::zmq)
target_compile_options(zmq_proxy_client PUBLIC -std=gnu99)

add_executable(zmq_rcv_from_dc zmq_rcv_from_dc.c)
target_link_libraries(zmq_rcv_from_dc PUBLIC zmq4::zmq)
target_compile_options(zmq_rcv_from_dc PUBLIC -std=gnu99)

add_executable(zmq_daq_test_data_firehose tests/zmq_firehose_pub.cc tests/zmq_firehose_common.cc)
target_include_directories(zmq_daq_test_data_firehose PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(zmq_daq_test_data_firehose PUBLIC zmq4::zmq)
target_requires_cpp11(zmq_daq_test_data_firehose PUBLIC)
#target_compile_features(zmq_daq_test_data_firehose PUBLIC cxx_defaulted_functions)

add_executable(zmq_daq_test_data_firehose_recv tests/zmq_firehose_sub.cc tests/zmq_firehose_common.cc)
target_include_directories(zmq_daq_test_data_firehose_recv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(zmq_daq_test_data_firehose_recv PUBLIC zmq4::zmq)
target_requires_cpp11(zmq_daq_test_data_firehose_recv PUBLIC)
#target_compile_features(zmq_daq_test_data_firehose_recv PUBLIC cxx_defaulted_functions)

install(TARGETS zmq_multi_stream zmq_dc DESTINATION bin)