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

    set (EPICS_BASE_CAS_LIBS "")
    set (EPICS_BASE_CA_LIBS "")

    foreach (_ca_lib IN LISTS _epics_calibs_libs_intl)
        message("Searching for ${_ca_lib} from ${_epics_calibs_libs_intl}")
        find_library("_concrete_ca_lib_${_ca_lib}" ${_ca_lib} HINTS ${EPICS_BASE_LIBRARY_DIRS})
        list(APPEND EPICS_BASE_CA_LIBS "${_concrete_ca_lib_${_ca_lib}}")
    endforeach(_ca_lib)
    message("CA LIBS = ${EPICS_BASE_CA_LIBS}")

    foreach (_cas_lib IN LISTS _epics_caslibs_libs_intl)
        message("Searching for ${_cas_lib} from ${_epics_caslibs_libs_intl}")
        find_library("_concrete_cas_lib_${_cas_lib}" ${_cas_lib} HINTS ${EPICS_BASE_LIBRARY_DIRS})
        list(APPEND EPICS_BASE_CAS_LIBS "${_concrete_cas_lib_${_cas_lib}}")
    endforeach(_cas_lib)
    message("CAS LIBS = ${EPICS_BASE_CAS_LIBS}")

    add_library(_epics_base_ca_intl INTERFACE)
    target_include_directories(_epics_base_ca_intl INTERFACE
            ${EPICS_BASE_INCLUDE_DIRS})
    target_compile_options(_epics_base_ca_intl INTERFACE
            ${EPICS_BASE_CFLAGS_OTHER})
    target_link_libraries(_epics_base_ca_intl INTERFACE
            ${EPICS_BASE_CA_LIBS})

    add_library(_epics_base_cas_intl INTERFACE)
    target_include_directories(_epics_base_cas_intl INTERFACE
            ${EPICS_BASE_INCLUDE_DIRS})
    target_compile_options(_epics_base_cas_intl INTERFACE
            ${EPICS_BASE_CFLAGS_OTHER})
    target_link_libraries(_epics_base_cas_intl INTERFACE
            ${EPICS_BASE_CAS_LIBS})

    find_path(_epics_gdd_include gdd.h
            hints ${EPICS_BASE_INCLUDE_DIRS})
    find_library(_epics_gdd_lib gdd HINTS ${EPICS_BASE_LIBRARY_DIRS})

    add_library(epics::ca ALIAS _epics_base_ca_intl)
    add_library(epics::cas ALIAS _epics_base_cas_intl)

    if (_epics_gdd_include AND _epics_gdd_lib)

        add_library(_epics_base_gdd_intl INTERFACE)
        target_include_directories(_epics_base_gdd_intl INTERFACE
                ${_epics_gdd_include})
        target_compile_options(_epics_base_gdd_intl INTERFACE
                ${EPICS_BASE_CFLAGS_OTHER})
        target_link_libraries(_epics_base_gdd_intl INTERFACE
                ${_epics_gdd_lib})

        add_library(epics::gdd ALIAS _epics_base_gdd_intl)

    else (_epics_gdd_include AND _epics_gdd_lib)
        message("EPICS gdd library not found")
    endif (_epics_gdd_include AND _epics_gdd_lib)

    message("Found epics-base")
    message("version ${EPICS_BASE_VERSION}")
    message("libs ${EPICS_BASE_LIBRARIES}")
    message("lib dirs ${EPICS_BASE_LIBRARY_DIRS}")
    message("includes ${EPICS_BASE_INCLUDE_DIRS}")
    message("cflags ${EPICS_BASE_CFLAGS_OTHER}")
endif(EPICS_BASE_FOUND)
