add_library(sub_plugin_rmipc
        sub_plugin_rmipc.cc)
Message("Current source dir ${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(sub_plugin_rmipc PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../
        ${Boost_INCLUDE_DIRS})
target_link_libraries(sub_plugin_rmipc PUBLIC
        cds::pub_sub)
target_link_libraries(sub_plugin_rmipc PRIVATE
        driver::shmem
        ${CMAKE_THREAD_LIBS_INIT})