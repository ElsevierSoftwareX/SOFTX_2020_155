
add_executable(ix_xmit ix_xmit.c)
target_link_libraries(ix_xmit PUBLIC driver::shmem dolphin::sisci)
install(TARGETS ix_xmit DESTINATION bin)

add_executable(ix_recv ix_fb_rcv.c)
target_link_libraries(ix_recv PUBLIC driver::shmem dolphin::sisci)
install(TARGETS ix_recv DESTINATION bin)

if (${LibZMQ_FOUND})
#add_executable(zmq_rcv_ix_xmit zmq_rcv_ix_xmit.c ../zmq_stream/dc_utils.c)
#target_link_libraries(zmq_rcv_ix_xmit PUBLIC
#        zmq_transport
#        driver::shmem
#        dolphin::sisci
#        zmq4::zmq
#        zmq::simple_pv
#        ${CMAKE_THREAD_LIBS_INIT})

add_executable(zmq_recv zmq_rcv_ix_xmit_delay.cc dolphin_common.cc ../zmq_stream/dc_utils.c)
# target_compile_options(zmq_rcv_ix_xmit_cxx PUBLIC -fsanitize=address)
# target_compile_options(zmq_rcv_ix_xmit_cxx PUBLIC -fsanitize=thread -no-pie)
target_link_libraries(zmq_recv PUBLIC
        # asan
        # tsan
        zmq_transport
        driver::shmem
        dolphin::sisci
        zmq4::zmq
        zmq::simple_pv
        ${CMAKE_THREAD_LIBS_INIT}
        # -no-pie
        )
install(TARGETS zmq_recv DESTINATION bin)

endif (${LibZMQ_FOUND})

#if (${MX_FOUND})
#
#add_executable(mx_rcv mx_rcv.cc dolphin_common.cc)
#target_compile_options(mx_rcv PRIVATE -O2)
#target_link_libraries(mx_rcv PUBLIC
#    driver::shmem
#    dolphin::sisci
#    zmq::simple_pv
#    mx::myriexpress
#    ${CMAKE_THREAD_LIBS_INIT}
#)
#
#add_executable(mx_rcv_simple mx_rcv_simple.cc dolphin_common.cc)
#target_compile_options(mx_rcv_simple PRIVATE -O2)
#target_link_libraries(mx_rcv_simple PUBLIC
#    driver::shmem
#    dolphin::sisci
#    zmq::simple_pv
#    mx::myriexpress
#    rt
#    ${CMAKE_THREAD_LIBS_INIT}
#)
#
#
#endif (${MX_FOUND})

if (${OPENMX_FOUND})
    add_executable(omx_recv mx2ix.c)
    target_link_libraries(omx_recv PUBLIC
            driver::shmem
            dolphin::sisci
            zmq::simple_pv
            openmx::openmx
            ${CMAKE_THREAD_LIBS_INIT}
            )
    install(TARGETS omx_recv DESTINATION bin)
endif (${OPENMX_FOUND})
