add_library(pub_sub_plugins
        sub_plugin_rmipc.cc
        pub_plugin_daq_m.cc
        sub_plugin_daq_m.cc
        )
Message("Current source dir ${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(pub_sub_plugins PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${Boost_INCLUDE_DIRS})
target_link_libraries(pub_sub_plugins PUBLIC
        cds::pub_sub
        util)
target_link_libraries(pub_sub_plugins PRIVATE
        driver::shmem
        ${CMAKE_THREAD_LIBS_INIT})