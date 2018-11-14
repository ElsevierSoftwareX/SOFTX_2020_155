project(zmq)
cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

include(Cpp11)
include(Cpp11Atomic)
include(FindThreads)
find_package(ZMQ4 REQUIRED)

add_library(simple_pv STATIC simple_pv.c)
target_include_directories(simple_pv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_library(zmq::simple_pv ALIAS simple_pv)

add_library(zmq_transport STATIC zmq_transport.c)
target_include_directories(zmq_transport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(zmq_transport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(zmq_transport PRIVATE zmq4::zmq)

add_library(zmq_dc_recv STATIC zmq_dc_recv.c zmq_dc_recv.h)
target_include_directories(zmq_dc_recv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(zmq_dc_recv PUBLIC zmq4::zmq ${CMAKE_THREAD_LIBS_INIT})
add_library(zmq::dc_recv ALIAS zmq_dc_recv)

add_executable(zmq_fe zmq_fe.c dc_utils.c)
target_link_libraries(zmq_fe PUBLIC driver::shmem zmq_transport zmq4::zmq zmq::simple_pv ${CMAKE_THREAD_LIBS_INIT})

add_executable(zmq_multi_stream zmq_multi_stream.c ../drv/rfm.c ${CMAKE_CURRENT_SOURCE_DIR}/../include/daq_core.h)
target_include_directories(zmq_multi_stream PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(zmq_multi_stream PUBLIC zmq4::zmq)
target_compile_options(zmq_multi_stream PUBLIC -std=gnu99)


add_executable(zmq_threads zmq_threads.c dc_utils.c)
target_include_directories(zmq_threads PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(zmq_threads PUBLIC
        driver::shmem
        zmq_transport
        zmq4::zmq
        zmq::simple_pv
        ${CMAKE_THREAD_LIBS_INIT})
target_compile_options(zmq_threads PUBLIC -std=gnu99)

add_executable(zmq_dc zmq_dc.cc)
target_include_directories(zmq_dc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(zmq_dc PUBLIC zmq_dc_recv zmq4::zmq ${CMAKE_THREAD_LIBS_INIT})
target_compile_options(zmq_dc PUBLIC ${CPP11_FLAG})

add_executable(zmq_recv zmq_recv.cc ../drv/rfm.c ${CMAKE_CURRENT_SOURCE_DIR}/../include/daq_core.h)
target_include_directories(zmq_recv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(zmq_recv PUBLIC zmq_dc_recv zmq4::zmq driver::shmem ${CMAKE_THREAD_LIBS_INIT})
if (${CXX_HAS_ATOMIC})
else (${CXX_HAS_ATOMIC})
    target_compile_definitions(zmq_recv PUBLIC "NO_STD_ATOMIC=1")
endif (${CXX_HAS_ATOMIC})
target_compile_options(zmq_recv PUBLIC ${CPP11_FLAG})

add_executable(zmq_proxy zmq_proxy.c)
target_include_directories(zmq_proxy PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(zmq_proxy PUBLIC zmq4::zmq)
target_compile_options(zmq_proxy PUBLIC -std=gnu99)

add_executable(zmq_proxy_client zmq_proxy_client.c)
target_include_directories(zmq_proxy_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(zmq_proxy_client PUBLIC zmq4::zmq)
target_compile_options(zmq_proxy_client PUBLIC -std=gnu99)

add_executable(zmq_rcv_from_dc zmq_rcv_from_dc.c)
target_include_directories(zmq_rcv_from_dc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(zmq_rcv_from_dc PUBLIC zmq4::zmq)
target_compile_options(zmq_rcv_from_dc PUBLIC -std=gnu99)

if (${CXX_HAS_ATOMIC})
# don't even try building these on pre c++11 compilers.
add_executable(zmq_daq_test_data_firehose tests/zmq_firehose_pub.cc tests/zmq_firehose_common.cc)
target_include_directories(zmq_daq_test_data_firehose PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(zmq_daq_test_data_firehose PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(zmq_daq_test_data_firehose PUBLIC zmq4::zmq)
target_requires_cpp11(zmq_daq_test_data_firehose PUBLIC)
#target_compile_features(zmq_daq_test_data_firehose PUBLIC cxx_defaulted_functions)

add_executable(zmq_daq_test_data_firehose_recv tests/zmq_firehose_sub.cc tests/zmq_firehose_common.cc)
target_include_directories(zmq_daq_test_data_firehose_recv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(zmq_daq_test_data_firehose_recv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(zmq_daq_test_data_firehose_recv PUBLIC zmq4::zmq)
target_requires_cpp11(zmq_daq_test_data_firehose_recv PUBLIC)
#target_compile_features(zmq_daq_test_data_firehose_recv PUBLIC cxx_defaulted_functions)

endif (${CXX_HAS_ATOMIC})

install(TARGETS zmq_multi_stream zmq_dc DESTINATION bin)
