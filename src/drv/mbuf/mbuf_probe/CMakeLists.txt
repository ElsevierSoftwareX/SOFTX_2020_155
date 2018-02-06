
add_executable(mbuf_probe mbuf_probe.cc)
target_include_directories(mbuf_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(mbuf_probe PUBLIC driver::shmem)