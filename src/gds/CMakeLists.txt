include(FindThreads)
FIND_PROGRAM(RPCGEN_PROG rpcgen)
MESSAGE("Found rpcgen at ${RPCGEN_PROG}")

SET(max_chnname_size "-DMAX_CHNNAME_SIZE=60")
SET( RPCFLAGS -C -N -M -DPORTMAP -DLIGO_GDS -DGDS_NO_EPICS ${max_chnname_size})

if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
    Message(FATAL_ERROR "The GDS code cannot build with source dir == build dir!")
endif (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})


SET (GDS_SRC
        awgapi.c
        awgfunc.c
        awg.c
        awg_server.c
        caldate_mjd.c
        cobox.c
        confserver.c
        ds340.c
        gdschannel.c
        gdsdac.c
        gdserr.c
        gdsheartbeat.c
        gdsics115.c
        gdsprm.c
        gdsrand.c
        gdssock.c
        gdsstring.c
        gdstask.c
        gdsxdr_util.c
        gpst2gpss.c
        pci.c
        rmapi.c
        rpcinc.c
        sockutil.c
        tconv.c
        testpointinfo.c
        testpoint.c
        testpoint_server.c
        timingcard.c
        gdssched_client.c
        gdssched.c
        gdssched_server.c
        gdssched_util.c
        awgtpman.c
        )

SET (GDS_CC_SRC
        gmutex.cc
        testchn.cc
        testpointmgr.cc
        )

SET (GDS_HDR
        awgapi.h
        awgfunc.h
        awg.h
        awg_server.h
        awgtype.h
        cobox.h
        confinfo.h
        confserver.h
        conftype.h
        ds340.h
        epics.h
        gdschannel.h
        gdsconst.h
        gdsdac.h
        gdserr.h
        gdserrmsg.h
        gdsheartbeat.h
        gdsics115.h
        gdsLib2.h
        gdsmain.h
        gdsprm.h
        gdsrand.h
        gdssched_client.h
        gdssched.h
        gdssched_server.h
        gdssched_util.h
        gdssock.h
        gdsstring.h
        gdstask.h
        gdsutil.h
        gdsxdr_util.h
        gpsclkdef.h
        gpsclk.h
        gpstime.h
        hardware.h
        ics115.h
        map.h
        map_v1.h
        map_v2.h
        map_v3.h
        ntp.h
        pci.h
        PConfig.h
        rchannel.h
        rlaunch.h
        rleap.h
        rmapi.h
        rmorg.h
        rpcinc.h
        sockutil.h
        targets.h
        tconv.h
        testpoint.h
        testpointinfo.h
        testpoint_server.h
        timingcard.h
        )

set(GDS_CC_HDR
        gmutex.hh
        testchn.hh
        testpointmgr.hh)

# in newer cmake we can do this in one step, becuase copy_if_different takes
# a list of files in newer cmake.  Well until then this is a multi-step process.
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gds_includes
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/gds_includes
        COMMAND ${CMAKE_COMMAND} -E create_symlink . ${CMAKE_CURRENT_BINARY_DIR}/gds_includes/dtt
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set (GDS_BIN_HDRS "")
foreach (GDS_HDR_FILE ${GDS_HDR})
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gds_includes/${GDS_HDR_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GDS_HDR_FILE} ${CMAKE_CURRENT_BINARY_DIR}/gds_includes
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gds_includes
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    list(APPEND GDS_BIN_HDRS ${CMAKE_CURRENT_BINARY_DIR}/gds_includes/${GDS_HDR_FILE})
endforeach(GDS_HDR_FILE)

foreach (GDS_CC_HDR_FILE ${GDS_CC_HDR})
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gds_includes/${GDS_CC_HDR_FILE}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GDS_CC_HDR_FILE} ${CMAKE_CURRENT_BINARY_DIR}/gds_includes
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gds_includes
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    list(APPEND GDS_BIN_HDRS ${CMAKE_CURRENT_BINARY_DIR}/gds_includes/${GDS_CC_HDR_FILE})
endforeach(GDS_CC_HDR_FILE)

SET (RPC_BASE gdsrsched rawgapi rtestpoint)

foreach (RPC_TEMPLATE ${RPC_BASE})
    set (RPC_TEMPLATE_OUT
            ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}_xdr.c
            ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}_svc.c
            ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}_clnt.c
            ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.h
            )
    set (GDS_SRC "${GDS_SRC}" ${RPC_TEMPLATE_OUT})
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.x
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${RPC_TEMPLATE}.x ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.x
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${RPC_TEMPLATE}.x
            )
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.h
            COMMAND ${RPCGEN_PROG} ${RPCFLAGS} -h -o ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.h ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.x
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.x
            )
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gds_includes/${RPC_TEMPLATE}.h
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.h ${CMAKE_CURRENT_BINARY_DIR}/gds_includes/${RPC_TEMPLATE}.h
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.h ${CMAKE_CURRENT_BINARY_DIR}/gds_includes
            MAIN_DEPENDENCY ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.h
            )
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}_xdr.c
            COMMAND ${RPCGEN_PROG} ${RPCFLAGS} -c -o ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}_xdr.c ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.x
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.x
            )
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}_clnt.c
            COMMAND ${RPCGEN_PROG} ${RPCFLAGS} -l -o ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}_clnt.c ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.x
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.x
            )
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}_svc.c
            COMMAND ${RPCGEN_PROG} ${RPCFLAGS} -m -o ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}_svc.c ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.x
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}.x
            )
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}_xdr.c
            PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/gds_includes/${RPC_TEMPLATE}.h;${GDS_BIN_HDRS}"
            )
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}_clnt.c
            PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/gds_includes/${RPC_TEMPLATE}.h;${GDS_BIN_HDRS}"
            )
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${RPC_TEMPLATE}_svc.c
            PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/gds_includes/${RPC_TEMPLATE}.h;${GDS_BIN_HDRS}"
            )
endforeach(RPC_TEMPLATE)

# we do this in two libraries, as the C files need -std=gnu99, the C++ compilers give warnings
# when building with this, so split the C & C++ portions into seperate pieces

add_library(gds_c OBJECT
        ${GDS_BIN_HDRS}
        ${GDS_SRC}
        )
target_compile_options(gds_c PRIVATE -std=gnu99)
target_include_directories(gds_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(gds_c PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/gds_includes)
target_compile_definitions(gds_c PRIVATE -DPORTMAP -DLIGO_GDS -DGDS_NO_EPICS ${max_chnname_size})

add_library(gds_cc OBJECT
    ${GDS_CC_SRC}
    )
target_compile_definitions(gds_cc PRIVATE -DPORTMAP -DLIGO_GDS -DGDS_NO_EPICS ${max_chnname_size})


add_executable(awgtpman $<TARGET_OBJECTS:gds_c> $<TARGET_OBJECTS:gds_cc> ${CMAKE_CURRENT_BINARY_DIR}/gds_includes)
target_link_libraries(awgtpman ${CMAKE_THREAD_LIBS_INIT})

#SET( CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-std=gnu99 -I. -I../include -g -DPORTMAP -DLIGO_GDS -DGDS_NO_EPICS ${max_chnname_size}" )
#SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -DPORTMAP -DLIGO_GDS -DGDS_NO_EPICS ${max_chnname_size}" )


#######
# builds needed for specific daqd
##

######
# Daqd dc specific
##
add_library(gds_daqd_dc_gds_main OBJECT
        gdsstring.c
        gdserr.c
        ${CMAKE_CURRENT_BINARY_DIR}/rtestpoint_clnt.c
        ${CMAKE_CURRENT_BINARY_DIR}/rtestpoint_xdr.c
        gdsheartbeat.c
        rpcinc.c
        gdssched.c
        tconv.c
        gdstask.c
        gdsxdr_util.c
        sockutil.c)
target_compile_definitions(gds_daqd_dc_gds_main PRIVATE
        -D_TP_DAQD
        -D_TESTPOINT_DIRECT=0)
target_include_directories(gds_daqd_dc_gds_main PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/gds_includes
        )

add_library(gds_daqd_dc_gds_tp OBJECT
        testpoint.c)
target_compile_definitions(gds_daqd_dc_gds_tp PRIVATE
        -D_TP_DAQD -D_TESTPOINT_DIRECT=0 -D_NO_KEEP_ALIVE)
target_include_directories(gds_daqd_dc_gds_tp PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/gds_includes
        )

add_library(gds_daqd_dc_gds STATIC $<TARGET_OBJECTS:gds_daqd_dc_gds_main> $<TARGET_OBJECTS:gds_daqd_dc_gds_tp>)
add_library(gds::dc_gds ALIAS gds_daqd_dc_gds)
