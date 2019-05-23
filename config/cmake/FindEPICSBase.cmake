# use pkgconfig to find EPICS Base

if (find_epics_base_included)
    return()
endif(find_epics_base_included)
set(find_epics_base_included TRUE)


pkg_check_modules(EPICS_BASE REQUIRED epics-base)

if (EPICS_BASE_FOUND)
    if (${EPICS_BASE_VERSION} VERSION_LESS "3.15")

        set (_epics_calibs_libs_intl "")
        set (_epics_caslibs_libs_intl "")
        set (_epics_Comlibs_libs_intl "")

    else (${EPICS_BASE_VERSION} VERSION_LESS "3.15")

        set (_epics_calibs_libs_intl "ca;Com")
        set (_epics_caslibs_libs_intl "cas")
        set (_epics_Comlibs_libs_intl "Com")

    endif (${EPICS_BASE_VERSION} VERSION_LESS "3.15")

    function(_epicsbase_add_library libname)
        # message("Searching for ${libname} in ${EPICS4_DIR}/${libname}/lib/linux-x86_64")
        find_library(_epicsbase_${libname}_lib name ${libname}
                PATHS ${EPICS_BASE_LIBRARY_DIRS})
        add_library(_epicsbase_${libname}_bin SHARED IMPORTED)
        set_target_properties(_epicsbase_${libname}_bin PROPERTIES
                IMPORTED_LOCATION ${_epicsbase_${libname}_lib})
        # message("Found binary for ${libname} at ${_epicsbase_${libname}_lib}")
    endfunction()

    _epicsbase_add_library(ca)
    _epicsbase_add_library(Com)
    _epicsbase_add_library(cas)

    add_library(_epics_base_ca_intl INTERFACE)
    target_include_directories(_epics_base_ca_intl INTERFACE
            ${EPICS_BASE_INCLUDE_DIRS})
    target_compile_options(_epics_base_ca_intl INTERFACE
            ${EPICS_BASE_CFLAGS_OTHER})
    target_link_libraries(_epics_base_ca_intl INTERFACE
            _epicsbase_Com_bin
            _epicsbase_ca_bin)

    add_library(_epics_base_cas_intl INTERFACE)
    target_include_directories(_epics_base_cas_intl INTERFACE
            ${EPICS_BASE_INCLUDE_DIRS})
    target_compile_options(_epics_base_cas_intl INTERFACE
            ${EPICS_BASE_CFLAGS_OTHER})
    target_link_libraries(_epics_base_cas_intl INTERFACE
            _epicsbase_cas_bin)

    add_library(_epics_base_Com_intl INTERFACE)
    target_include_directories(_epics_base_Com_intl INTERFACE
            ${EPICS_BASE_INCLUDE_DIRS})
    target_compile_options(_epics_base_Com_intl INTERFACE
            ${EPICS_BASE_CFLAGS_OTHER})
    target_link_libraries(_epics_base_Com_intl INTERFACE
            _epicsbase_Com_bin
            )

    set (EPICS_BASE_CAS_LIBS ${_epics_caslibs_libs_intl})
    set (EPICS_BASE_CA_LIBS ${_epics_calibs_libs_intl})
    set (EPICS_BASE_COM_LIBS ${_epics_Comlibs_libs_intl})

    add_library(epics::ca ALIAS _epics_base_ca_intl)
    add_library(epics::cas ALIAS _epics_base_cas_intl)
    add_library(epics::Com ALIAS _epics_base_Com_intl)

    message("Found epics-base")
    message("version ${EPICS_BASE_VERSION}")
    message("libs ${EPICS_BASE_LIBRARIES}")
    message("lib dirs ${EPICS_BASE_LIBRARY_DIRS}")
    message("includes ${EPICS_BASE_INCLUDE_DIRS}")
    message("cflags ${EPICS_BASE_CFLAGS_OTHER}")
endif(EPICS_BASE_FOUND)
