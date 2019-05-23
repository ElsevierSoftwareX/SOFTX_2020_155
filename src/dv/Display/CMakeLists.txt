add_executable(framer4 framer4.c)
target_link_libraries(framer4 PUBLIC
        th::datasrv
        tai::tai
        m
        ${CMAKE_THREAD_LIBS_INIT}
        grace::grace)


add_executable(frameMemRead frameMemRead.c)
target_link_libraries(frameMemRead PUBLIC
        th::datasrv
        tai::tai
        m
        ${CMAKE_THREAD_LIBS_INIT}
        grace::grace)

add_executable(framer_nds4 framer_nds_client.c)
target_link_libraries(framer_nds4 PUBLIC
        th::datasrv_nds
        tai::tai
        m
        ${CMAKE_THREAD_LIBS_INIT}
        grace::grace)

add_executable(frameMemRead_nds frameMemRead_nds_client.c)
target_link_libraries(frameMemRead_nds PUBLIC
        th::datasrv_nds
        tai::tai
        m
        ${CMAKE_THREAD_LIBS_INIT}
        grace::grace)

install(TARGETS framer4 frameMemRead DESTINATION bin)
install(TARGETS framer_nds4 frameMemRead_nds DESTINATION bin)
