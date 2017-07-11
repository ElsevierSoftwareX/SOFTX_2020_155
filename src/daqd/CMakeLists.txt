FIND_PROGRAM(UNAME_PROG uname)
FIND_PROGRAM(DATE_PROG date)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

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
		gds::dc_gds
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
		gds::dc_gds
		dc_mx_gds
		run_number::run_number
		ca Com cas gdd epics::ca epics::cas
		${EPICS_BASE_CA_LIBS} ${EPICS_BASE_CAS_LIBS}
		ldastools::framecpp
		${CMAKE_THREAD_LIBS_INIT}
		mx::myriexpress
		)
endif(${MX_FOUND})

#########
# DC with ZMQ
###
# Reuse the main source lists from daqd_dc_mx

add_library(dc_gds_zmq STATIC ${DAQD_DC_GDS_SRC})
target_compile_definitions(dc_gds_zmq PRIVATE
		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux -DUNIX -Dlinux
		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
		-DLIGO_GDS
		-D_TP_DAQD
		-DARCHIVE=\"${CMAKE_CURRENT_SOURCE_DIR}/../gds\"
		-DDAQD_BUILD_DC_ZMQ)
target_include_directories(dc_gds_zmq PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../include
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
		)
target_compile_options(dc_gds_zmq PUBLIC
		-fno-common
		-Wno-deprecated
		-Wno-write-strings
		)
target_link_libraries(dc_gds_zmq PRIVATE
		gds::dc_gds
		ldastools::framecpp
		zmq
		)

add_executable(daqd_dc_zmq ${DAQD_DC_MAIN_SRC} producer_zmq.cc)
target_compile_definitions(daqd_dc_zmq PUBLIC
		${DAQD_BUILD_INFO_DEFINES} -DUNIX -Dlinux -DUNIX -Dlinux
		-D_REENTRANT -DNO_RTL=1 -DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
		-DDAQD_BUILD_DC_ZMQ
		)
target_include_directories(daqd_dc_zmq PUBLIC
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../include
		)
target_link_libraries(daqd_dc_zmq PUBLIC
		gds::dc_gds
		dc_gds_zmq
		run_number::run_number
		ca Com cas gdd epics::ca epics::cas
		${EPICS_BASE_CA_LIBS} ${EPICS_BASE_CAS_LIBS}
		ldastools::framecpp
		${CMAKE_THREAD_LIBS_INIT}
		zmq
		)