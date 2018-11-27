FIND_PROGRAM(UNAME_PROG uname)
FIND_PROGRAM(DATE_PROG date)

INCLUDE(Cpp11)
INCLUDE(CheckCXXSourceCompiles)

add_subdirectory(component_tests)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(__daqd_tmp_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_FLAGS ${CPP11_FLAG})
check_cxx_source_compiles("#include<array> int main() { return 0; }" DAQD_CAN_BUILD_DAQD_SHMEM)
set(CMAKE_REQUIRED_FLAGS ${__daqd_tmp_CMAKE_REQUIRED_FLAGS})

# These sources are common to all daqd variants
set(MAIN_DAQD_SRC
        archive.cc
        profiler.cc
        filesys.cc
        #producer.cc
        trend.cc
        net_writer.cc
        #${CMAKE_CURRENT_BINARY_DIR}/comm.cc
        daqd.cc
		comm_impl.cc
        circ.cc
        edcu.cc
        framerecv.cc
        ${CMAKE_CURRENT_BINARY_DIR}/comm-lex.cc
        listener.cc
        )

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
# Each daqd build is made from several static libraries
# the libraries are used to group items with similar build settings together
# and are not meant to be exported.
#######
# Common source lists
#######

set(DAQD_GDS_SRC
        gds.cc
        )

set (DAQD_COMMON_SRC
        archive.cc
        profiler.cc
        filesys.cc
        epics_pvs.cc
        producer_fw.cc
		checksum_crc32.cc
        net_writer.cc
        trend.cc
        circ.cc
        edcu.cc
        epicsServer.cc
        exServer.cc
        exPV.cc
        exChannel.cc
        framerecv.cc
        listener.cc
        daqd.cc
        exScalarPV.cc
        exVectorPV.cc
		comm_impl.cc
        )

########
# FW Build
####

add_library(fw_daqd_gds STATIC ${DAQD_GDS_SRC})
target_compile_definitions(fw_daqd_gds PRIVATE
        ${DAQD_BUILD_INFO_DEFINES}
        -D_GNU_SOURCE -D_DEFAULT_SOURCE
        -D_X86_64_ -DUNIX -Dlinux
        -D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11
        -DLIGO_GDS -D_TP_DAQD
        -DARCHIVE=\"${CMAKE_CURRENT_SOURCE_DIR}/../gds\"
        )
target_compile_options(fw_daqd_gds PRIVATE
        -mtune=generic -m64
        -Wno-deprecated -Wno-write-strings
        )
target_include_directories(fw_daqd_gds PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(fw_daqd_gds PUBLIC ldastools::framecpp)
#c++ -c -DSERVER_VERSION=\"2.0\" -DPRODUCTION_DATE=\"now\" -DPRODUCTION_MACHINE=\"this\" -DGDS_TESTPOINT_SUPPORT
#  -D_GNU_SOURCE -D_DEFAULT_SOURCE
#  -D_X86_64_ -DUNIX -Dlinux
#  -mtune=generic -m64
#  -I/opt/epics/base-3.15.4/include -I/opt/epics/base-3.15.4/include/os/Linux -I/opt/epics/base-3.15.4/include/compiler/gcc -I/opt/gm/include -I/opt/mx/include
#   -g -Wno-deprecated -Wno-write-strings
#  -DDAQD_BUILD_FW -I/home/jonathan.hanks/Documents/Programming/aligo-rts-scratch-build/build/fw -I/home/jonathan.hanks/Documents/Programming/aligo-rts-scratch/src/daqd -I/home/jonathan.hanks/Documents/Programming/aligo-rts-scratch/src/include
#  -D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -g -O
#  -DLIGO_GDS -D_TP_DAQD -DARCHIVE=\"/home/jonathan.hanks/Documents/Programming/aligo-rts-scratch/src/daqd/../gds\" -I/home/jonathan.hanks/Documents/Programming/aligo-rts-scratch/src/gds

add_library(fw_common STATIC ${DAQD_COMMON_SRC})
target_include_directories(fw_common PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        )
target_compile_definitions(fw_common PRIVATE
        ${DAQD_BUILD_INFO_DEFINES}
        ${CDS_GDSFLAGS}
        -D_GNU_SOURCE -D_DEFAULT_SOURCE
        -D_X86_64_ -DUNIX -Dlinux -DUNIX
        -D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11
        -DDAQD_BUILD_FW)
target_compile_options(fw_common PRIVATE
        -mtune=generic -m64
        -Wno-deprecated -Wno-write-strings
        )
target_link_libraries(fw_common PRIVATE
        epics::ca epics::cas ldastools::framecpp
		run_number::run_number)

#c++ -c -DSERVER_VERSION=\"2.0\" -DPRODUCTION_DATE=\"now\" -DPRODUCTION_MACHINE=\"this\" -DGDS_TESTPOINT_SUPPORT
#  -D_GNU_SOURCE -D_DEFAULT_SOURCE
#  -D_X86_64_ -DUNIX -Dlinux
#  -mtune=generic -m64
#  -I/opt/epics/base-3.15.4/include -I/opt/epics/base-3.15.4/include/os/Linux -I/opt/epics/base-3.15.4/include/compiler/gcc -I/opt/gm/include -I/opt/mx/include -g -Wno-deprecated -Wno-write-strings -DDAQD_BUILD_FW -I/home/jonathan.hanks/Documents/Programming/aligo-rts-scratch-build/build/fw -I/home/jonathan.hanks/Documents/Programming/aligo-rts-scratch/src/daqd -I/home/jonathan.hanks/Documents/Programming/aligo-rts-scratch/src/include
#  -D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -g -O

add_library(fw_comm STATIC ${CMAKE_CURRENT_BINARY_DIR}/comm.cc)
target_compile_definitions(fw_comm PRIVATE
        ${DAQD_BUILD_INFO_DEFINES}
        -DDAQD_BUILD_FW ${CDS_GDSFLAGS} -D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11)
target_compile_options(fw_comm PRIVATE
        -mtune=generic -m64
        -Wno-deprecated -Wno-write-strings
        )
target_include_directories(fw_comm PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(fw_comm PRIVATE
        epics::cas ldastools::framecpp)
#set_target_properties(fw_comm
#        PROPERTIES COMPILE_FLAGS "-DDAQD_BUILD_FW ${CDS_GDSFLAGS} -D_REENTRANT -DNO_RTL=1")
# -DSERVER_VERSION=\\\"2.0\\\" -DPRODUCTION_DATE=\\\"Mon Feb  6 11:59:36 PST 2017\\\"
# -DPRODUCTION_MACHINE=\\\"Linux ubuntu 4.4.0-59-generic #80-Ubuntu SMP Fri Jan 6 17:47:47 UTC 2017 x86_64 x86_64 x86_64 GNU/Linux\\\"
# -DGDS_TESTPOINT_SUPPORT
# -std=c++11 -I/opt/ldas-tools-al-2.5.5/include -I/opt/ldas-tools-framecpp-2.5.2/include
# -D_GNU_SOURCE -D_DEFAULT_SOURCE
# -D_X86_64_ -DUNIX -Dlinux
# -mtune=generic -m64
# -I/opt/epics/base-3.15.4/include -I/opt/epics/base-3.15.4/include/os/Linux -I/opt/epics/base-3.15.4/include/compiler/gcc
# -I/opt/gm/include -I/opt/mx/include
# -g -Wno-deprecated -Wno-write-strings
# -DDAQD_BUILD_FW
# -I/home/jonathan.hanks/Documents/advLigoRTS/branches/build-3.3/build/fw -I/home/jonathan.hanks/Documents/advLigoRTS/branches/branch-3.3/src/daqd -I/home/jonathan.hanks/Documents/advLigoRTS/branches/branch-3.3/src/include
# -D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION -g -O

link_directories(${EPICS_BASE_LIBRARY_DIRS}
        ${FrameCPP_LIBRARY_DIRS})

add_executable(daqd_fw ${MAIN_DAQD_SRC})
target_include_directories(daqd_fw PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_definitions(daqd_fw PRIVATE ${DAQD_FCPP_FLAGS} -DDAQD_BUILD_FW ${CDS_GDSFLAGS} -D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11)
target_compile_options(daqd_fw PRIVATE
        ${DAQD_BUILD_INFO_DEFINES}
        -Wno-deprecated -Wno-write-strings)
target_link_libraries(daqd_fw fw_common fw_comm fw_daqd_gds ca Com cas gdd epics::ca epics::cas
        ${EPICS_BASE_CA_LIBS} ${EPICS_BASE_CAS_LIBS}
        ${FrameCPP_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
		run_number::run_number
        )

if (${MX_FOUND})
###########
# DC Build with mx support
####
set (DAQD_DC_MAIN_SRC
		archive.cc
		profiler.cc
		filesys.cc
		epics_pvs.cc
		trend.cc
		net_writer.cc
		circ.cc
		framesend.cc
		gdsmutex.cc
		edcu.cc
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
		${CMAKE_CURRENT_BINARY_DIR}/comm.cc
		${CMAKE_CURRENT_BINARY_DIR}/comm-lex.cc)

set (DAQD_DC_GDS_SRC
		gds.cc)

add_library(dc_mx_gds STATIC ${DAQD_DC_GDS_SRC})
target_compile_definitions(dc_mx_gds PRIVATE
		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux -DUNIX -Dlinux
		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
		-DLIGO_GDS
		-D_TP_DAQD
		-DARCHIVE=\"${CMAKE_CURRENT_SOURCE_DIR}/../gds\"
		-DDAQD_BUILD_DC)
target_include_directories(dc_mx_gds PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../include
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
		)
target_compile_options(dc_mx_gds PUBLIC
		-fno-common
		-Wno-deprecated
		-Wno-write-strings
		)
target_link_libraries(dc_mx_gds PRIVATE
		gds::daqd
		ldastools::framecpp
		mx::myriexpress)

add_executable(daqd_dc_mx ${DAQD_DC_MAIN_SRC} producer.cc mx_rcvr.cc)
target_compile_definitions(daqd_dc_mx PUBLIC
		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux -DUNIX -Dlinux
		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
		-DDAQD_BUILD_DC
		)
target_include_directories(daqd_dc_mx PUBLIC
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../include
		)
target_link_libraries(daqd_dc_mx PUBLIC
		gds::daqd
		dc_mx_gds
		run_number::run_number
		ca Com cas gdd epics::ca epics::cas
		${EPICS_BASE_CA_LIBS} ${EPICS_BASE_CAS_LIBS}
		ldastools::framecpp
		${CMAKE_THREAD_LIBS_INIT}
		mx::myriexpress
		)

###########
# mx-symm Build with mx support
####
set (DAQD_MXSYMM_MAIN_SRC
		archive.cc
		profiler.cc
		filesys.cc
		epics_pvs.cc
		trend.cc
		net_writer.cc
		circ.cc
		#framesend.cc
		#gdsmutex.cc
		edcu.cc
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
		${CMAKE_CURRENT_BINARY_DIR}/comm.cc
		${CMAKE_CURRENT_BINARY_DIR}/comm-lex.cc)

add_library(dc_mxsymm_gds STATIC gds.cc)
target_compile_definitions(dc_mxsymm_gds PRIVATE
		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux
		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
		-DLIGO_GDS
		-D_TP_DAQD
		-DARCHIVE=\"${CMAKE_CURRENT_SOURCE_DIR}/../gds\"
		-DGDS_TESTPOINT_SUPPORT
		-DDAQD_BUILD_MXSYMM)
target_include_directories(dc_mxsymm_gds PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../include
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
		)
target_compile_options(dc_mxsymm_gds PUBLIC
		-fno-common
		-Wno-deprecated
		-Wno-write-strings
		)
target_link_libraries(dc_mxsymm_gds PRIVATE
		gds::daqd
		ldastools::framecpp
		mx::myriexpress)

add_executable(daqd_mxsymm ${DAQD_MXSYMM_MAIN_SRC} producer.cc mx_rcvr.cc)
target_compile_definitions(daqd_mxsymm PUBLIC
		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux
		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
		-DGDS_TESTPOINT_SUPPORT
		-DDAQD_BUILD_MXSYMM
		)
target_include_directories(daqd_mxsymm PUBLIC
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../include
		)
target_link_libraries(daqd_mxsymm PUBLIC
		gds::daqd
		dc_mxsymm_gds
		run_number::run_number
		ca Com cas gdd epics::ca epics::cas
		${EPICS_BASE_CA_LIBS} ${EPICS_BASE_CAS_LIBS}
		ldastools::framecpp
		${CMAKE_THREAD_LIBS_INIT}
		mx::myriexpress
		)


install(TARGETS daqd_mxsymm daqd_dc_mx DESTINATION bin)
endif(${MX_FOUND})

if (${DAQD_CAN_BUILD_DAQD_SHMEM})
##########
## generic daq with shmem (new standiop)
####
## Reuse the main source lists from daqd_dc_mx
#
set (DAQD_SHMEM_MAIN_SRC
		archive.cc
		profiler.cc
		filesys.cc
		epics_pvs.cc
		trend.cc
		net_writer.cc
		circ.cc
		framesend.cc
		gdsmutex.cc
		edcu.cc
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
		${CMAKE_CURRENT_BINARY_DIR}/comm-lex.cc)

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
		)
target_compile_options(dc_gds_shmem PUBLIC
		-fno-common
		-Wno-deprecated
		-Wno-write-strings
		)
target_link_libraries(dc_gds_shmem PRIVATE
		gds::daqd
		ldastools::framecpp
		)

add_executable(daqd_shmem ${DAQD_SHMEM_MAIN_SRC} producer_shmem.cc)
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
		)
target_link_libraries(daqd_shmem PUBLIC
		gds::daqd
		dc_gds_shmem
		run_number::run_number
		ca Com cas gdd epics::ca epics::cas
		${EPICS_BASE_CA_LIBS} ${EPICS_BASE_CAS_LIBS}
		ldastools::framecpp
		driver::shmem
		${CMAKE_THREAD_LIBS_INIT}
		)

##########
## DC with ZMQ
####
## Reuse the main source lists from daqd_dc_mx
#
#set (DAQD_DC_ZMQ_MAIN_SRC
#		archive.cc
#		profiler.cc
#		filesys.cc
#		epics_pvs.cc
#		trend.cc
#		net_writer.cc
#		circ.cc
#		framesend.cc
#		gdsmutex.cc
#		edcu.cc
#		epicsServer.cc
#		exServer.cc
#		exScalarPV.cc
#		exVectorPV.cc
#		exPV.cc
#		exChannel.cc
#		framerecv.cc
#		listener.cc
#		daqd.cc
#		comm_impl.cc
#        ../include/daq_core.h
#		${CMAKE_CURRENT_BINARY_DIR}/comm.cc
#		${CMAKE_CURRENT_BINARY_DIR}/comm-lex.cc)
#
#add_library(dc_gds_zmq STATIC gds.cc)
#target_compile_definitions(dc_gds_zmq PRIVATE
#		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux -DUNIX -Dlinux
#		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
#		-DLIGO_GDS
#		-D_TP_DAQD
#		-DARCHIVE=\"${CMAKE_CURRENT_SOURCE_DIR}/../gds\"
#		-DDAQD_BUILD_DC_ZMQ)
#target_include_directories(dc_gds_zmq PUBLIC
#		${CMAKE_CURRENT_SOURCE_DIR}/../include
#		${CMAKE_CURRENT_SOURCE_DIR}
#		${CMAKE_CURRENT_BINARY_DIR}
#		)
#target_compile_options(dc_gds_zmq PUBLIC
#		-fno-common
#		-Wno-deprecated
#		-Wno-write-strings
#		)
#target_link_libraries(dc_gds_zmq PRIVATE
#		gds::daqd
#		ldastools::framecpp
#		zmq
#		)
#
#add_executable(daqd_dc_zmq ${DAQD_DC_ZMQ_MAIN_SRC} producer_dc_zmq.cc)
#target_compile_definitions(daqd_dc_zmq PUBLIC
#		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux -DUNIX -Dlinux
#		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
#		-DDAQD_BUILD_DC_ZMQ
#		)
#target_include_directories(daqd_dc_zmq PUBLIC
#		${CMAKE_CURRENT_BINARY_DIR}
#		${CMAKE_CURRENT_SOURCE_DIR}
#		${CMAKE_CURRENT_SOURCE_DIR}/../include
#		)
#target_link_libraries(daqd_dc_zmq PUBLIC
#		gds::daqd
#		dc_gds_zmq
#		run_number::run_number
#		ca Com cas gdd epics::ca epics::cas
#		${EPICS_BASE_CA_LIBS} ${EPICS_BASE_CAS_LIBS}
#		ldastools::framecpp
#		${CMAKE_THREAD_LIBS_INIT}
#		zmq
#		zmq::dc_recv
#		)
endif(${DAQD_CAN_BUILD_DAQD_SHMEM})

###########
# daqd_rcv A receiver daqd
####
set (DAQD_RCV_MAIN_SRC
		archive.cc
		profiler.cc
		filesys.cc
		epics_pvs.cc
		trend.cc
		net_writer.cc
		circ.cc
		edcu.cc
		epicsServer.cc
		exServer.cc
		exPV.cc
		exChannel.cc
		framerecv.cc
		listener.cc
		daqd.cc
		exScalarPV.cc
		exVectorPV.cc
		comm_impl.cc
		${CMAKE_CURRENT_BINARY_DIR}/comm.cc
		${CMAKE_CURRENT_BINARY_DIR}/comm-lex.cc)

add_library(dc_rcv_gds STATIC gds.cc)
target_compile_definitions(dc_rcv_gds PRIVATE
		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux
		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
		-DLIGO_GDS
		-D_TP_DAQD
		-DARCHIVE=\"${CMAKE_CURRENT_SOURCE_DIR}/../gds\"
		-DGDS_TESTPOINT_SUPPORT
		-DDAQD_BUILD_RCV
		-D_GNU_SOURCE -D_DEFAULT_SOURCE
		)
target_include_directories(dc_rcv_gds PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../include
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
		)
target_compile_options(dc_rcv_gds PUBLIC
		-fno-common
		-Wno-deprecated
		-Wno-write-strings
		)
target_link_libraries(dc_rcv_gds PRIVATE
		gds::daqd
		ldastools::framecpp
		)

add_executable(daqd_rcv ${DAQD_RCV_MAIN_SRC} producer_rcv.cc)
target_compile_definitions(daqd_rcv PUBLIC
		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux
		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
		-DGDS_TESTPOINT_SUPPORT
		-DDAQD_BUILD_RCV
		-D_GNU_SOURCE -D_DEFAULT_SOURCE
		)
target_include_directories(daqd_rcv PUBLIC
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../include
		)
target_link_libraries(daqd_rcv PUBLIC
		gds::daqd
		dc_rcv_gds
		run_number::run_number
		ca Com cas gdd epics::ca epics::cas
		${EPICS_BASE_CA_LIBS} ${EPICS_BASE_CAS_LIBS}
		ldastools::framecpp
		${CMAKE_THREAD_LIBS_INIT}
		)

###########
# daqd_bcst A gds broadcaster daqd
####
set (DAQD_BCST_MAIN_SRC
		archive.cc
		profiler.cc
		filesys.cc
		epics_pvs.cc
		trend.cc
		net_writer.cc
		circ.cc
		framesend.cc
		gdsmutex.cc
		edcu.cc
		epicsServer.cc
		exServer.cc
		exPV.cc
		exChannel.cc
		framerecv.cc
		listener.cc
		daqd.cc
		exScalarPV.cc
		exVectorPV.cc
		comm_impl.cc
		${CMAKE_CURRENT_BINARY_DIR}/comm.cc
		${CMAKE_CURRENT_BINARY_DIR}/comm-lex.cc)

add_library(dc_bcst_gds STATIC gds.cc)
target_compile_definitions(dc_bcst_gds PRIVATE
		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux
		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
		-DLIGO_GDS
		-D_TP_DAQD
		-DARCHIVE=\"${CMAKE_CURRENT_SOURCE_DIR}/../gds\"
		-DGDS_TESTPOINT_SUPPORT
		-DDAQD_BUILD_BCST
		-D_GNU_SOURCE -D_DEFAULT_SOURCE
		)
target_include_directories(dc_bcst_gds PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../include
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
		)
target_compile_options(dc_bcst_gds PUBLIC
		-fno-common
		-Wno-deprecated
		-Wno-write-strings
		)
target_link_libraries(dc_bcst_gds PRIVATE
		gds::daqd
		ldastools::framecpp
		)

add_executable(daqd_bcst ${DAQD_BCST_MAIN_SRC} producer.cc)
target_compile_definitions(daqd_bcst PUBLIC
		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux
		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
		-DGDS_TESTPOINT_SUPPORT
		-DDAQD_BUILD_BCST
		-D_GNU_SOURCE -D_DEFAULT_SOURCE
		)
target_include_directories(daqd_bcst PUBLIC
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../include
		)
target_link_libraries(daqd_bcst PUBLIC
		gds::daqd
		dc_bcst_gds
		run_number::run_number
		ca Com cas gdd epics::ca epics::cas
		${EPICS_BASE_CA_LIBS} ${EPICS_BASE_CAS_LIBS}
		ldastools::framecpp
		${CMAKE_THREAD_LIBS_INIT}
		)

###########
# daqd_standiop A standalone system with iop
####
set (DAQD_STANDIOP_MAIN_SRC
		archive.cc
		profiler.cc
		filesys.cc
		epics_pvs.cc
		trend.cc
		net_writer.cc
		circ.cc
		edcu.cc
		epicsServer.cc
		exServer.cc
		exPV.cc
		exChannel.cc
		framerecv.cc
		listener.cc
		daqd.cc
		exScalarPV.cc
		exVectorPV.cc
		comm_impl.cc
		${CMAKE_CURRENT_BINARY_DIR}/comm.cc
		${CMAKE_CURRENT_BINARY_DIR}/comm-lex.cc)

add_library(dc_standiop_gds STATIC gds.cc)
target_compile_definitions(dc_standiop_gds PRIVATE
		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux
		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
		-DLIGO_GDS
		-D_TP_DAQD
		-DARCHIVE=\"${CMAKE_CURRENT_SOURCE_DIR}/../gds\"
		-DGDS_TESTPOINT_SUPPORT
		-DDAQD_BUILD_STANDIOP
		-D_GNU_SOURCE -D_DEFAULT_SOURCE
		)
target_include_directories(dc_standiop_gds PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../include
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
		)
target_compile_options(dc_standiop_gds PUBLIC
		-fno-common
		-Wno-deprecated
		-Wno-write-strings
		)
target_link_libraries(dc_standiop_gds PRIVATE
		gds::daqd
		ldastools::framecpp
		)

add_executable(daqd_standiop ${DAQD_STANDIOP_MAIN_SRC} producer.cc)
target_compile_definitions(daqd_standiop PUBLIC
		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux
		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
		-DGDS_TESTPOINT_SUPPORT
		-DDAQD_BUILD_STANDIOP
		-D_GNU_SOURCE -D_DEFAULT_SOURCE
		)
target_include_directories(daqd_standiop PUBLIC
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../include
		)
target_link_libraries(daqd_standiop PUBLIC
		gds::daqd
		dc_standiop_gds
		run_number::run_number
		ca Com cas gdd epics::ca epics::cas
		${EPICS_BASE_CA_LIBS} ${EPICS_BASE_CAS_LIBS}
		ldastools::framecpp
		${CMAKE_THREAD_LIBS_INIT}
		)


install(TARGETS daqd_fw daqd_rcv daqd_bcst daqd_standiop DESTINATION bin)
