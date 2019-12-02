FIND_PROGRAM(UNAME_PROG uname)
FIND_PROGRAM(DATE_PROG date)

INCLUDE(Cpp11)
INCLUDE(CheckCXXSourceCompiles)

add_subdirectory(component_tests)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(__daqd_tmp_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_FLAGS ${CPP11_FLAG})
check_cxx_source_compiles("#include<array>
int main() { return 0; }" DAQD_CAN_BUILD_DAQD_SHMEM)
set(CMAKE_REQUIRED_FLAGS ${__daqd_tmp_CMAKE_REQUIRED_FLAGS})

set_source_files_properties(listener.cc daqd.cc
	PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/y.tab.h)

execute_process(COMMAND uname -a
        OUTPUT_VARIABLE DAQD_UNAME)
execute_process(COMMAND date
        OUTPUT_VARIABLE DAQD_DATE)

string(REPLACE "#" "" DAQD_UNAME ${DAQD_UNAME})
string(REPLACE "\n" "" DAQD_UNAME ${DAQD_UNAME})

string(REPLACE "\n" "" DAQD_DATE ${DAQD_DATE})
set(DAQD_BUILD_INFO_DEFINES -DSERVER_VERSION=\"2.0\"
        -DPRODUCTION_MACHINE=\"${DAQD_UNAME}\"
        -DPRODUCTION_DATE=\"${DAQD_DATE}\")

set(CDS_GDSFLAGS "-DGDS_TESTPOINT_SUPPORT")

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/comm-lex.cc
        COMMAND ${FLEX_PROG} -+ -o ${CMAKE_CURRENT_BINARY_DIR}/comm-lex.cc ${CMAKE_CURRENT_SOURCE_DIR}/comm-lex.l
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/comm-lex.l
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/comm-lex.l
        )

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/comm.cc ${CMAKE_CURRENT_BINARY_DIR}/y.tab.h
        COMMAND ${BISON_PROG} -o ${CMAKE_CURRENT_BINARY_DIR}/comm.cc -d ${CMAKE_CURRENT_SOURCE_DIR}/comm.y
        COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/comm.hh ${CMAKE_CURRENT_BINARY_DIR}/y.tab.h
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/comm.y
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/comm.y
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )

########
# The daqd build is made from several static libraries
# the libraries are used to group items with similar build settings together
# and are not meant to be exported.
#######
# Common source lists
#######

set(DAQD_GDS_SRC
        gds.cc
        )

if (${DAQD_CAN_BUILD_DAQD_SHMEM})
##########
## generic daq with shmem (new standiop)
####
add_library(dc_gds_shmem STATIC gds.cc)
target_compile_definitions(dc_gds_shmem PRIVATE
		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux -DUNIX -Dlinux
		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
		-DLIGO_GDS
		-D_TP_DAQD
		-DARCHIVE=\"${CMAKE_CURRENT_SOURCE_DIR}/../gds\"
		-DDAQD_BUILD_SHMEM)
target_include_directories(dc_gds_shmem PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../include
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
		${RPC_INCLUDE_DIRS}
		)
target_compile_options(dc_gds_shmem PUBLIC
		-fno-common
		-Wno-deprecated
		-Wno-write-strings
		)
target_link_libraries(dc_gds_shmem PRIVATE
		gds::daqd
		ldastools::framecpp
		${RPC_LIBRARIES}
		)

add_executable(daqd_shmem
		archive.cc
		profiler.cc
		filesys.cc
		epics_pvs.cc
		trend.cc
		net_writer.cc
		circ.cc
		framesend.cc
		gdsmutex.cc
		epicsServer.cc
		exServer.cc
		exScalarPV.cc
		exVectorPV.cc
		exPV.cc
		exChannel.cc
		framerecv.cc
		listener.cc
		daqd.cc
		comm_impl.cc
		../include/daq_core.h
		${CMAKE_CURRENT_BINARY_DIR}/comm.cc
		${CMAKE_CURRENT_BINARY_DIR}/comm-lex.cc
		producer_shmem.cc)

target_compile_definitions(daqd_shmem PUBLIC
		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux -DUNIX -Dlinux
		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
		-DDAQD_BUILD_SHMEM
		)
target_include_directories(daqd_shmem PUBLIC
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../include
		${CMAKE_CURRENT_SOURCE_DIR}/../zmq_stream
		${RPC_INCLUDE_DIRS}
		)
target_link_libraries(daqd_shmem PUBLIC
		gds::daqd
		dc_gds_shmem
		run_number::run_number
		# ca Com cas gdd
		epics::gdd
		epics::ca epics::cas
		#${EPICS_BASE_CA_LIBS} ${EPICS_BASE_CAS_LIBS}
		ldastools::framecpp
		driver::shmem
		driver::ini_parsing
		${CMAKE_THREAD_LIBS_INIT}
		${RPC_LIBRARIES}
		)

install(TARGETS daqd_shmem DESTINATION bin)


add_executable(test_daqd_cmask_t tests/test_main.cc
		tests/test_daqd_cmask_t.cc)
target_include_directories(test_daqd_cmask_t PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME test_daqd_cmask_t
		COMMAND test_daqd_cmask_t
		WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

if (libNDS2Client_FOUND)

	configure_file(tests/daqdrc_live_test ${CMAKE_CURRENT_BINARY_DIR}/daqdrc_live_test COPYONLY)
	configure_file(tests/test_daqd_live_nds.sh.in ${CMAKE_CURRENT_BINARY_DIR}/test_daqd_live_nds.sh @ONLY)

	add_test(NAME test_daqd_live_nds
			COMMAND /bin/bash ./test_daqd_live_nds.sh
			WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

	configure_file(tests/daqdrc_nds_test ${CMAKE_CURRENT_BINARY_DIR}/daqdrc_nds_test COPYONLY)
	configure_file(tests/test_daqd_nds.sh.in ${CMAKE_CURRENT_BINARY_DIR}/test_daqd_nds.sh @ONLY)

	add_test(NAME test_daqd_nds
			COMMAND /bin/bash ./test_daqd_nds.sh
			WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")


	add_executable(test_nds1_connections tests/test_nds1_connections.cc)
	target_link_libraries(test_nds1_connections PUBLIC nds2client::cxx
		${CMAKE_THREAD_LIBS_INIT})

	configure_file(tests/test_daqd_nds_connections.sh.in ${CMAKE_CURRENT_BINARY_DIR}/test_daqd_nds_connections.sh @ONLY)

	add_test(NAME test_daqd_nds_connections
			COMMAND /bin/bash ./test_daqd_nds_connections.sh
			WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

endif (libNDS2Client_FOUND)

endif(${DAQD_CAN_BUILD_DAQD_SHMEM})
