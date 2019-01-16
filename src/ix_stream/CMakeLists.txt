
add_executable(ix_fb_rcv ix_fb_rcv.c)
target_link_libraries(ix_fb_rcv PUBLIC driver::shmem dolphin::sisci)

if (${LibZMQ_FOUND})
#add_executable(zmq_rcv_ix_xmit zmq_rcv_ix_xmit.c ../zmq_stream/dc_utils.c)
#target_link_libraries(zmq_rcv_ix_xmit PUBLIC
#        zmq_transport
#        driver::shmem
#        dolphin::sisci
#        zmq4::zmq
#        zmq::simple_pv
#        ${CMAKE_THREAD_LIBS_INIT})

add_executable(zmq_rcv_ix_xmit_delay zmq_rcv_ix_xmit_delay.cc dolphin_common.cc ../zmq_stream/dc_utils.c)
# target_compile_options(zmq_rcv_ix_xmit_cxx PUBLIC -fsanitize=address)
# target_compile_options(zmq_rcv_ix_xmit_cxx PUBLIC -fsanitize=thread -no-pie)
target_link_libraries(zmq_rcv_ix_xmit_delay PUBLIC
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

endif (${LibZMQ_FOUND})

if (${MX_FOUND})

add_executable(mx_rcv mx_rcv.cc dolphin_common.cc)
target_compile_options(mx_rcv PRIVATE -O)
target_link_libraries(mx_rcv PUBLIC
    driver::shmem
    dolphin::sisci
    zmq::simple_pv
    mx::myriexpress
    ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(mx_rcv_simple mx_rcv_simple.cc dolphin_common.cc)
target_compile_options(mx_rcv_simple PRIVATE -O)
target_link_libraries(mx_rcv_simple PUBLIC
    driver::shmem
    dolphin::sisci
    zmq::simple_pv
    mx::myriexpress
    rt
    ${CMAKE_THREAD_LIBS_INIT}
)


endif (${MX_FOUND})