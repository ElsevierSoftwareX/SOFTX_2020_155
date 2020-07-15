if (Boost_FOUND)

add_executable(standalone_edc
        standalone_edcu.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../drv/rfm.c)
#target_compile_options(standalone_edc PRIVATE -fsanitize=address)
#target_link_libraries(standalone_edc PRIVATE asan)
#target_compile_options(standalone_edc PRIVATE
#        -fstack-protector -fstack-protector-strong)
target_include_directories(standalone_edc PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include/drv"
        ${Boost_INCLUDE_DIRS})
target_link_libraries(standalone_edc PUBLIC
        epics::ca
        driver::ini_parsing
        ${Boost_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT})
target_requires_cpp11(standalone_edc PUBLIC)

configure_file(test/epics_test.py ${CMAKE_CURRENT_BINARY_DIR}/epics_test.py COPYONLY)
configure_file(test/daqdrc_standalone_edc_live_test ${CMAKE_CURRENT_BINARY_DIR}/daqdrc_standalone_edc_live_test COPYONLY)
configure_file(test/test_standalone_edc_live_nds.sh.in ${CMAKE_CURRENT_BINARY_DIR}/test_standalone_edc_live_nds.sh @ONLY)

add_test(NAME test_standalone_edc_live_nds
        COMMAND /bin/bash ./test_standalone_edc_live_nds.sh
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS standalone_edc DESTINATION bin)

else(Boost_FOUND)
    message(WARNING "The standalone_edc will not be build as boost was not found")
endif (Boost_FOUND)