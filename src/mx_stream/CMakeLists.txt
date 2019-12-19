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
	target_link_libraries(omx_xmit PUBLIC openmx::openmx args ${MAKE_THREAD_LIBS_INIT})
	set_target_properties(omx_xmit PROPERTIES LINKER_LANGUAGE CXX)

    install(TARGETS omx_xmit DESTINATION bin)

	add_executable(omx_recv omx_recv.c)
	target_link_libraries(omx_recv PUBLIC
			driver::shmem
			pv::simple_pv
			openmx::openmx
			${CMAKE_THREAD_LIBS_INIT}
			args
			)

	add_executable(omx_recv_buffered omx_recv_buffered.cc recv_buffer.cc)
	target_link_libraries(omx_recv_buffered PUBLIC
			driver::shmem
			pv::simple_pv
			openmx::openmx
			${CMAKE_THREAD_LIBS_INIT}
			)
	target_requires_cpp11(omx_recv_buffered PUBLIC)

	if (${CXX_HAS_ATOMIC})
	add_executable(test_recv_buffer recv_buffer.cc
			tests/test_main.cc
			tests/test_recv_buffer.cc)
	target_include_directories(test_recv_buffer PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR}
			${CMAKE_CURRENT_SOURCE_DIR}/../include)
	target_link_libraries(test_recv_buffer PUBLIC
			${CMAKE_THREAD_LIBS_INIT})
	target_requires_cpp11(test_recv_buffer PUBLIC)

	add_test(NAME test_recv_buffer
			COMMAND test_recv_buffer
			WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
	endif (${CXX_HAS_ATOMIC})

	install(TARGETS omx_recv DESTINATION bin)
endif (OPENMX_FOUND)
