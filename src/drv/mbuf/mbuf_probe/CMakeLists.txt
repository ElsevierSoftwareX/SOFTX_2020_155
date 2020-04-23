
add_executable(mbuf_probe
        gap_check.cc
        mbuf_probe.cc
        mbuf_decoders.cc
        analyze_daq_multi_dc.cc
        analyze_rmipc.cc)
target_include_directories(mbuf_probe PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        )
target_link_libraries(mbuf_probe PUBLIC driver::shmem)
target_requires_cpp11(mbuf_probe PRIVATE)

install(TARGETS mbuf_probe DESTINATION BIN)