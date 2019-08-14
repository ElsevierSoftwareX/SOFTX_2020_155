find_package(OpenMX)
include(FindThreads)

if (OPENMX_FOUND)

    # this is a weird area, these are .c files but need to be built as C++
    # this should be fixed after we stop the manual makefile build
#    set (CMAKE_C_COMPILER ${CMAKE_CXX_COMPILER})
#    add_executable(mx_stream mx_stream.c ${CMAKE_CURRENT_SOURCE_DIR}}/../drv/rfm.c)
#    target_compile_definitions(mx_stream PUBLIC -DNO_RTL -D_XOPEN_SOURCE -D_BSD_SOURCE)
#    target_include_directories(mx_stream PUBLIC
#            ${CMAKE_CURRENT_SOURCE_DIR}
#            ${CMAKE_CURRENT_SOURCE_DIR}/../common
#            ${CMAKE_CURRENT_SOURCE_DIR}/../common/bsd)
#    target_link_libraries(mx_stream PUBLIC openmx::openmx ${MAKE_THREAD_LIBS_INIT})
#    set_target_properties(mx_stream PROPERTIES LINKER_LANGUAGE CXX)

    add_executable(omx_xmit mx_fe.c ${CMAKE_CURRENT_SOURCE_DIR}/../drv/rfm.c)
	target_compile_definitions(omx_xmit PUBLIC -DNO_RTL -D_XOPEN_SOURCE -D_BSD_SOURCE)
	target_include_directories(omx_xmit PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/../common
            ${CMAKE_CURRENT_SOURCE_DIR}/../common/bsd)
	target_link_libraries(omx_xmit PUBLIC openmx::openmx ${MAKE_THREAD_LIBS_INIT})
	set_target_properties(omx_xmit PROPERTIES LINKER_LANGUAGE CXX)

    install(TARGETS omx_xmit DESTINATION bin)

	add_executable(omx_recv omc_recv.c)
	target_link_libraries(omx_recv PUBLIC
			driver::shmem
			dolphin::sisci
			zmq::simple_pv
			openmx::openmx
			${CMAKE_THREAD_LIBS_INIT}
			)
	install(TARGETS omx_recv DESTINATION bin)
endif (OPENMX_FOUND)
