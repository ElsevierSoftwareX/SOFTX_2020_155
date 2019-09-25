
add_executable(ix_test ix_test.c)
target_link_libraries(ix_test PUBLIC dolphin::sisci)

add_executable(dix_xmit dix_xmit.c)
target_link_libraries(dix_xmit PUBLIC driver::shmem dolphin::sisci pv::simple_pv)
install(TARGETS dix_xmit DESTINATION bin)

add_executable(dix_recv dix_recv.c)
target_link_libraries(dix_recv PUBLIC driver::shmem dolphin::sisci)
install(TARGETS dix_recv DESTINATION bin)

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


