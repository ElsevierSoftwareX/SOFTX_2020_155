
add_executable(ix_test ix_test.c)
target_link_libraries(ix_test PUBLIC dolphin::sisci)

add_executable(dix_xmit dix_xmit.c)
target_link_libraries(dix_xmit PUBLIC
        args
        driver::shmem
        dolphin::sisci
        pv::simple_pv)
install(TARGETS dix_xmit DESTINATION bin)

add_executable(dix_recv dix_recv.c)
target_link_libraries(dix_recv PUBLIC
        args
        driver::shmem
        dolphin::sisci)
install(TARGETS dix_recv DESTINATION bin)
