add_library(fe_stream_generator STATIC
        fe_stream_generator.hh
        str_split.cc
        str_split.hh)
target_include_directories(fe_stream_generator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(fe_stream_test
        fe_stream_test.cc
        gps.hh
        )
target_link_libraries(fe_stream_test
        PRIVATE
        fe_stream_generator
        rt)

add_executable(fe_stream_check
        fe_stream_check.cc
        )
target_link_libraries(fe_stream_check
        PRIVATE
        fe_stream_generator
        rt
        driver::shmem)

if (${libNDS2Client_FOUND})

#    add_executable(fe_stream_check
#            fe_stream_check.cc
#            )
#    target_link_libraries(fe_stream_check
#            PRIVATE
#            fe_stream_generator
#            nds2client::cxx)

endif (${libNDS2Client_FOUND})
