Message("libcds-pubsub_FOUND = ${libcds-pubsub_FOUND}")
if (libcds-pubsub_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../mx_stream/atomic_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/atomic_config.h)

    add_subdirectory(plugins)

    add_executable(cps_xmit
            pub_xmit.cc
            )
    target_include_directories(cps_xmit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
    target_link_libraries(cps_xmit PUBLIC
            driver::shmem
            pv::simple_pv
            args
            cds::pub_sub
            )
    target_requires_cpp11(cps_xmit PUBLIC)

    add_executable(cps_xmit_asan
            pub_xmit.cc
            )
    target_include_directories(cps_xmit_asan PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
    target_link_libraries(cps_xmit_asan PUBLIC
            driver::shmem
            pv::simple_pv
            args
            cds::pub_sub_asan
    )
    target_requires_cpp11(cps_xmit_asan PUBLIC)


    add_executable(cps_recv
            sub_recv_bufferred.cc
            recv_buffer.cc
            )
    target_include_directories(cps_recv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include
            ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(cps_recv PUBLIC
            driver::shmem
            pv::simple_pv
            args
            cds::pub_sub)
    target_requires_cpp11(cps_recv PUBLIC)

    add_executable(cps_recv_asan
            sub_recv_bufferred.cc
            recv_buffer.cc
            )
    target_include_directories(cps_recv_asan PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include
            ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(cps_recv_asan PUBLIC
            driver::shmem
            pv::simple_pv
            args
            cds::pub_sub_asan)
    target_requires_cpp11(cps_recv_asan PUBLIC)

    add_executable(cds_pub_sub
            cds_pub_sub.cc
            recv_buffer.cc
            )
    target_include_directories(cds_pub_sub PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../src/include
            ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(cds_pub_sub PUBLIC
            pv::simple_pv
            args
            cds::pub_sub
            pub_sub_plugins)
    target_requires_cpp11(cds_pub_sub PUBLIC)

#    add_executable(cds_pub_sub_asan
#            cds_pub_sub.cc
#            recv_buffer.cc
#            )
#    target_include_directories(cds_pub_sub_asan PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../asan/include
#            ${CMAKE_CURRENT_BINARY_DIR})
#    target_link_libraries(cds_pub_sub_asan PUBLIC
#            pv::simple_pv
#            args
#            cds::pub_sub_asan
#            sub_plugin_rmipc)
#    target_requires_cpp11(cds_pub_sub_asan PUBLIC)

    add_executable(test_buffer_entry tests/test_buffer_entry.cc)
    target_include_directories(test_buffer_entry PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_requires_cpp11(test_buffer_entry PUBLIC)

    configure_file(test_pub_sub_xmit_recv.sh.in test_pub_sub_xmit_recv.sh @ONLY)
    configure_file(test_pub_sub.sh.in test_pub_sub.sh @ONLY)
    configure_file(test_pub_sub_rmipc_to_daqm.sh.in test_pub_sub_rmipc_to_daqm.sh @ONLY)
    configure_file(test_pub_sub_daqm_to_daqm.sh.in test_pub_sub_daqm_to_daqm.sh @ONLY)

    add_test(NAME "test_pub_sub_xmit_recv"
            COMMAND /bin/bash ./test_pub_sub_xmit_recv.sh
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

    add_test(NAME "test_pub_sub"
        COMMAND /bin/bash ./test_pub_sub.sh
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

    add_test(NAME "test_pub_sub_rmipc_to_daqm"
        COMMAND /bin/bash ./test_pub_sub_rmipc_to_daqm.sh
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

    add_test(NAME "test_pub_sub_daqm_to_daqm"
            COMMAND /bin/bash ./test_pub_sub_daqm_to_daqm.sh
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

    add_test(NAME "test_buffer_entry"
            COMMAND test_buffer_entry)


    install(TARGETS cps_xmit DESTINATION bin)
    install(TARGETS cps_recv DESTINATION bin)
    install(TARGETS cds_pub_sub DESTINATION bin)

endif (libcds-pubsub_FOUND)