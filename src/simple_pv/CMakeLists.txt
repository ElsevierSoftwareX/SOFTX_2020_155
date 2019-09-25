add_library(simple_pv STATIC simple_pv.c)
target_include_directories(simple_pv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_library(pv::simple_pv ALIAS simple_pv)