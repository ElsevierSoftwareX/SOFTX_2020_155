find_package(OpenMX)
include(FindThreads)

if (OPENMX_FOUND)

    add_executable(mx_stream mx_stream.cc ${CMAKE_CURRENT_SOURCE_DIR}}/../drv/rfm.c)
    target_compile_definitions(mx_stream PUBLIC -DNO_RTL -D_XOPEN_SOURCE -D_BSD_SOURCE)
    target_include_directories(mx_stream PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/../common
            ${CMAKE_CURRENT_SOURCE_DIR}/../common/bsd)
    target_link_libraries(mx_stream PUBLIC openmx::openmx ${MAKE_THREAD_LIBS_INIT})
endif (OPENMX_FOUND)
