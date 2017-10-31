add_library(fe_stream_generator INTERFACE)
target_include_directories(fe_stream_generator INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(fe_stream_test
        fe_stream_test.cc
        ../drv/rfm.c
        )
target_link_libraries(fe_stream_test
        PRIVATE
        fe_stream_generator)