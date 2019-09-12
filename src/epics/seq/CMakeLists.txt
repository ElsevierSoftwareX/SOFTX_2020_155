add_executable(standalone_edc
        standalone_edcu.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../../drv/rfm.c)
target_include_directories(standalone_edc PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include/drv")
target_link_libraries(standalone_edc PUBLIC
        epics::ca
        epics::seq)

add_executable(edc
        edcu.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../../drv/rfm.c)
target_include_directories(edc PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include/drv")
target_link_libraries(edc PUBLIC
        epics::ca
        epics::seq)

install(TARGETS standalone_edc DESTINATION bin)