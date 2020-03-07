Message("libcds-pubsub_FOUND = ${libcds-pubsub_FOUND}")
if (libcds-pubsub_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../mx_stream/atomic_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/atomic_config.h)

    add_executable(pub_xmit
            pub_xmit.cc
            )
    target_include_directories(pub_xmit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
    target_link_libraries(pub_xmit PUBLIC
            driver::shmem
            pv::simple_pv
            args
            cds::pub_sub
            )
    target_requires_cpp11(pub_xmit PUBLIC)

    add_executable(pub_xmit_asan
            pub_xmit.cc
            )
    target_include_directories(pub_xmit_asan PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
    target_link_libraries(pub_xmit_asan PUBLIC
            driver::shmem
            pv::simple_pv
            args
            cds::pub_sub_asan
    )
    target_requires_cpp11(pub_xmit_asan PUBLIC)


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
            cds::pub_sub)
    target_requires_cpp11(sub_recv PUBLIC)

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
            cds::pub_sub_asan)
    target_requires_cpp11(sub_recv_asan PUBLIC)

    configure_file(test_pub_sub_xmit_recv.sh.in test_pub_sub_xmit_recv.sh @ONLY)

    add_test(NAME "test_pub_sub_xmit_recv"
            COMMAND /bin/bash ./test_pub_sub_xmit_recv.sh
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")


endif (libcds-pubsub_FOUND)