if (Boost_FOUND)

add_executable(standalone_edc
        standalone_edcu.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../drv/rfm.c)
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

install(TARGETS standalone_edc DESTINATION bin)

else(Boost_FOUND)
    message(WARNING "The standalone_edc will not be build as boost was not found")
endif (Boost_FOUND)