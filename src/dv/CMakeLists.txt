FIND_PACKAGE(Grace)
FIND_PACKAGE(Motif)
FIND_PACKAGE(X11)

if (${GRACE_FOUND} AND ${MOTIF_FOUND} AND ${X11_Xpm_FOUND} AND ${X11_Xt_FOUND} AND ${X11_FOUND})
    Message("Dataviewr will be built")

    # create some targets to make things look nice
    add_library(motif_ INTERFACE)
    target_include_directories(motif_ INTERFACE ${MOTIF_INCLUDE_DIR})
    target_link_libraries(motif_ INTERFACE ${MOTIF_LIBRARIES})
    add_library(motif::motif ALIAS motif_)

    add_library(dv_x11_ INTERFACE)
    target_include_directories(dv_x11_ INTERFACE ${X11_INCLUDE_DIR})
    target_link_libraries(dv_x11_ INTERFACE
            ${X11_Xpm_LIB}
            ${X11_Xt_LIB}
            ${X11_LIBRARIES})
    add_library(dv::X11 ALIAS dv_x11_)

    add_subdirectory(Lib/UTC_GPS)
    add_subdirectory(Th)
    add_subdirectory(Display)
    add_subdirectory(Control)

    configure_file(dataviewer.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/dataviewer @ONLY)
    configure_file(dataviewer_msgwin.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/dataviewer_msgwin @ONLY)

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/dataviewer"
            "${CMAKE_CURRENT_BINARY_DIR}/dataviewer_msgwin"
            DESTINATION bin
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

else (${GRACE_FOUND} AND ${MOTIF_FOUND} AND ${X11_Xpm_FOUND} AND ${X11_Xt_FOUND} AND ${X11_FOUND})
    Message("Dataviewer will NOT be built")
    Message("X11 found = ${X11_FOUND}")
    Message("motif found = ${MOTIF_FOUND}")
    Message("Xpm found = ${X11_Xpm_FOUND}")
    Message("Xt found = ${X11_Xt_FOUND}")
endif (${GRACE_FOUND} AND ${MOTIF_FOUND} AND ${X11_Xpm_FOUND} AND ${X11_Xt_FOUND} AND ${X11_FOUND})