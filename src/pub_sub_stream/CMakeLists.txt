Message("PUB_SUB_ROOT = ${PUB_SUB_ROOT}")
if (PUB_SUB_ROOT)
    add_subdirectory(${PUB_SUB_ROOT}
            ${CMAKE_CURRENT_BINARY_DIR}/pub_sub)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../mx_stream/atomic_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/atomic_config.h)

    add_executable(pub_xmit
            pub_xmit.cc
            )
    target_include_directories(pub_xmit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
    target_link_libraries(pub_xmit PUBLIC
            driver::shmem
            pv::simple_pv
            args
            pub_sub)

    add_executable(pub_xmit_asan
            pub_xmit.cc
            )
    target_include_directories(pub_xmit_asan PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
    target_link_libraries(pub_xmit_asan PUBLIC
            driver::shmem
            pv::simple_pv
            args
            pub_sub_asan)

    add_executable(sub_recv
            sub_recv_bufferred.cc
            recv_buffer.cc
            )
    target_include_directories(sub_recv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include
            ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(sub_recv PUBLIC
            driver::shmem
            pv::simple_pv
            args
            pub_sub)

    add_executable(sub_recv_asan
            sub_recv_bufferred.cc
            recv_buffer.cc
            )
    target_include_directories(sub_recv_asan PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include
            ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(sub_recv_asan PUBLIC
            driver::shmem
            pv::simple_pv
            args
            pub_sub_asan)

    configure_file(test_pub_sub_xmit_recv.sh.in test_pub_sub_xmit_recv.sh @ONLY)

    add_test(NAME "test_pub_sub_xmit_recv"
            COMMAND /bin/bash ./test_pub_sub_xmit_recv.sh
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")


endif (PUB_SUB_ROOT)