add_library(simple_pv STATIC simple_pv.c)
target_include_directories(simple_pv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_library(pv::simple_pv ALIAS simple_pv)

add_library(simple_pv_cxx simple_pv.cc
        simple_epics.cc)
target_link_libraries(simple_pv_cxx PUBLIC epics::cas epics::gdd /usr/lib/epics/lib/linux-x86_64/libCom.so)

add_executable(test_simple_pv tests/test_simple_pv.cc)
target_link_libraries(test_simple_pv PUBLIC simple_pv_cxx)
target_include_directories(test_simple_pv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})