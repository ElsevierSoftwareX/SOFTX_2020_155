add_library(java_widgets STATIC
        java_widgets/lib/
        java_widgets/lib/Border.c
        java_widgets/lib/Card.c
        java_widgets/lib/Flow.c
        java_widgets/lib/Grid.c
        java_widgets/lib/GridBag.c
        java_widgets/lib/Layout.c
        )
target_include_directories(java_widgets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/java_widgets/h)
target_link_libraries(java_widgets PUBLIC motif::motif)

add_executable(dv dc3.c TabBook.c main.c dc3_stubs.c)
target_include_directories(dv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(dv PUBLIC MOTIF)
target_link_libraries(dv PUBLIC
        th::datasrv
        java_widgets
        motif::motif
        dv::X11
        m
        tai::tai
        ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS dv DESTINATION bin)