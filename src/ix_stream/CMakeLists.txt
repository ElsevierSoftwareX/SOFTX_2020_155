
add_executable(ix_fb_rcv ix_fb_rcv.c)
target_link_libraries(ix_fb_rcv PUBLIC driver::shmem dolphin::sisci)

if (${LibZMQ_FOUND})
add_executable(zmq_rcv_ix_xmit zmq_rcv_ix_xmit.c)
target_link_libraries(zmq_rcv_ix_xmit PUBLIC zmq_transport driver::shmem dolphin::sisci zmq4::zmq ${CMAKE_THREAD_LIBS_INIT})
endif (${LibZMQ_FOUND})