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
        th::datasrv
        tai::tai
        m
        ${CMAKE_THREAD_LIBS_INIT}
        grace::grace)

install(TARGETS framer4 frameMemRead DESTINATION bin)