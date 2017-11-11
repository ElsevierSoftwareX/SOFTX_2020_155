
if (cds_find_nds2_client_included)
    return()
endif(cds_find_nds2_client_included)
set(cds_find_nds2_client_included TRUE)

pkg_check_modules(libNDS2Client nds2-client>=0.14.3)

if (libNDS2Client_FOUND)
    find_library(libNDS2Client_lib_path ndsxxwrap
            PATHS ${libNDS2Client_LIBRARY_DIRS})

    find_path(libNDS2client_include_path nds.hh
            PATHS ${libNDS2Client_INCLUDE_DIRS})

    add_library(_nds2client SHARED IMPORTED)
    set_target_properties(_nds2client PROPERTIES
            IMPORTED_LOCATION ${libNDS2Client_lib_path})

    add_library(_nds2client_intl INTERFACE)
    target_include_directories(_nds2client_intl INTERFACE ${libNDS2client_include_path})
    target_link_libraries(_nds2client_intl INTERFACE _nds2client)

    add_library(nds2client::cxx ALIAS _nds2client_intl)


endif(libNDS2Client_FOUND)

