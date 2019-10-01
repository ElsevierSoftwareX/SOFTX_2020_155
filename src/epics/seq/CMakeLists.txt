add_executable(standalone_edc
        standalone_edcu.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../drv/rfm.c)
target_include_directories(standalone_edc PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include/drv")
target_link_libraries(standalone_edc PUBLIC
        epics::ca
        driver::ini_parsing)

install(TARGETS standalone_edc DESTINATION bin)