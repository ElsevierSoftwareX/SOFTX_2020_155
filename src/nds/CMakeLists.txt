set( NDS_SRC
        datastreamcombiner.cc
        mmstream.cc
        daqd_net.cc
        nds.cc
        readtocframefilearchive.cc
        rawminutetrend.cc
        main.cc
        spec.cc
        scanarchivedir.cc
        )

execute_process(COMMAND uname -a
        OUTPUT_VARIABLE NDS_UNAME)
execute_process(COMMAND date
        OUTPUT_VARIABLE NDS_DATE)

string(REPLACE "#" "" NDS_UNAME ${NDS_UNAME})
string(REPLACE "\n" "" NDS_UNAME ${NDS_UNAME})
string(REPLACE "\n" "" NDS_DATE ${NDS_DATE})
set(NDS_COMMON_DEFINES -DSERVER_VERSION=\"1.5\"
        -DPRODUCTION_MACHINE=\"${NDS_UNAME}\"
        -DPRODUCTION_DATE=\"${NDS_DATE}\")

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

#c++ -c
#-DSERVER_VERSION=\\"1.5\\" -DPRODUCTION_DATE=\\"date here\\" -DPRODUCTION_MACHINE=\\"Linux ubuntu 4.4.0-78-generic #99-Ubuntu SMP Thu Apr 27 15:29:09 UTC 2017 x86_64 x86_64 x86_64 GNU/Linux\\"
#-DDAQD_CPP11 -DUSE_FRAMECPP_VERSION
#-std=c++11
#-I/opt/ldas-tools-al-2.5.5/include -I/opt/ldas-tools-framecpp-2.5.2/include
#-g -DNDEBUG
#-I/home/jonathan.hanks/Documents/advLigoRTS/branches/build-3.3/build/nds
#-I/home/jonathan.hanks/Documents/advLigoRTS/branches/branch-3.3/src/nds
#-I/home/jonathan.hanks/Documents/advLigoRTS/branches/branch-3.3/src/nds/client
#-I/home/jonathan.hanks/Documents/advLigoRTS/branches/branch-3.3/src/nds/framelib/src
#-I/home/jonathan.hanks/Documents/advLigoRTS/branches/branch-3.3/src/nds/framelib/src/zlib
#-I/home/jonathan.hanks/Documents/advLigoRTS/branches/branch-3.3/src/gds
#-D_REENTRANT

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../gds)

add_executable(nds ${NDS_SRC})
target_compile_definitions(nds PRIVATE -D_REENTRANT -DDAQD_CPP11)
target_compile_definitions(nds PRIVATE ${NDS_COMMON_DEFINES})
target_compile_definitions(nds PRIVATE -DDAQD_CPP11)
target_include_directories(nds PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/client
    ${CMAKE_CURRENT_SOURCE_DIR}/framelib/src
    ${CMAKE_CURRENT_SOURCE_DIR}/framelib/src/zlib
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_link_libraries(nds PRIVATE ldastools::framecpp z bz2 dl nsl)

install(TARGETS nds DESTINATION bin)

# -L/opt/ldas-tools-al-2.5.5/lib -L/opt/ldas-tools-framecpp-2.5.2/lib -lframecpp -lframecpp8 -lframecpp7 -lframecpp6 -lframecpp4 -lframecpp3 -lframecppcmn -lldastoolsal -lz -lbz2 -ldl -lnsl