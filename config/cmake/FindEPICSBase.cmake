# use pkgconfig to find EPICS Base

pkg_check_modules(EPICS_BASE REQUIRED epics-base)

if (EPICS_BASE_FOUND)
    if (${EPICS_BASE_VERSION} VERSION_LESS "3.15")

        set (_epics_calibs_libs_intl "")
        set (_epics_caslibs_libs_intl "")

    else (${EPICS_BASE_VERSION} VERSION_LESS "3.15")

        set (_epics_calibs_libs_intl "ca;Com")
        set (_epics_caslibs_libs_intl "cas")

    endif (${EPICS_BASE_VERSION} VERSION_LESS "3.15")

    add_library(_epics_base_ca_intl INTERFACE)
    target_include_directories(_epics_base_ca_intl INTERFACE
            ${EPICS_BASE_INCLUDE_DIRS})
    target_compile_options(_epics_base_ca_intl INTERFACE
            ${EPICS_BASE_CFLAGS_OTHER})

    add_library(_epics_base_cas_intl INTERFACE)
    target_include_directories(_epics_base_cas_intl INTERFACE
            ${EPICS_BASE_INCLUDE_DIRS})
    target_compile_options(_epics_base_cas_intl INTERFACE
            ${EPICS_BASE_CFLAGS_OTHER})

    set (EPICS_BASE_CAS_LIBS ${_epics_caslibs_libs_intl})
    set (EPICS_BASE_CA_LIBS ${_epics_calibs_libs_intl})

    add_library(epics::ca ALIAS _epics_base_ca_intl)
    add_library(epics::cas ALIAS _epics_base_cas_intl)

    message("Found epics-base")
    message("version ${EPICS_BASE_VERSION}")
    message("libs ${EPICS_BASE_LIBRARIES}")
    message("lib dirs ${EPICS_BASE_LIBRARY_DIRS}")
    message("includes ${EPICS_BASE_INCLUDE_DIRS}")
    message("cflags ${EPICS_BASE_CFLAGS_OTHER}")
endif(EPICS_BASE_FOUND)
