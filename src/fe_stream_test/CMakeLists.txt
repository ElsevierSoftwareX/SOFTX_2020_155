add_library(fe_stream_generator STATIC
        fe_stream_generator.cc
        fe_stream_generator.hh
        str_split.cc
        str_split.hh)
target_include_directories(fe_stream_generator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_library(fe_generator_support STATIC
        fe_generator_support.hh
        fe_generator_support.cc)
target_include_directories(fe_generator_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(fe_multi_stream_test
        fe_multi_stream_test.cc
        gps.hh)
target_link_libraries(fe_multi_stream_test
        PRIVATE
        fe_stream_generator
        fe_generator_support
        rt)


add_executable(fe_stream_check
        fe_stream_check.cc
        )
target_link_libraries(fe_stream_check
        PRIVATE
        fe_stream_generator
        rt
        driver::shmem)

add_executable(fe_check fe_check.cc)
target_link_libraries(fe_check
        PRIVATE
        rt
        driver::shmem)

if (libNDS2Client_FOUND)

add_executable(fe_stream_check_nds fe_stream_check_nds.cc)
target_link_libraries(fe_stream_check_nds
        PRIVATE
        fe_stream_generator
        nds2client::cxx)

add_executable(fe_stream_check_edcu_nds fe_stream_check_edcu_nds.cc)
target_include_directories(fe_stream_check_edcu_nds PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(fe_stream_check_edcu_nds
        PRIVATE
        fe_stream_generator
        nds2client::cxx)

endif (libNDS2Client_FOUND)