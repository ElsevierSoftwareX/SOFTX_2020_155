
FIND_PACKAGE(PkgConfig)
FIND_PACKAGE(EPICSBase REQUIRED)

if (find_epics_v4_cpp_included)
    return()
endif(find_epics_v4_cpp_included)
set(find_epics_v4_cpp_included TRUE)

if (USE_EPICS4_CMAKE)
    set (EPICSv4_FOUND 1)
    add_subdirectory(${USE_EPICS4_CMAKE} ${PROJECT_BINARY_DIR}/EPICSv4_CMAKE_BUILD)
    return()
endif(USE_EPICS4_CMAKE)


if (EPICS4_DIR)
    set (EPICSv4_FOUND 1)

    function(_epicsv4_add_library libname libbinname deps)
        # message("Searching for ${libname} in ${EPICS4_DIR}/${libname}/lib/linux-x86_64")
        find_library(_epicsv4_${libname}_lib name ${libbinname}
                PATHS ${EPICS4_DIR}/${libname}/lib/linux-x86_64)
        add_library(_epicsv4_${libname}_bin SHARED IMPORTED)
        set_target_properties(_epicsv4_${libname}_bin PROPERTIES
                IMPORTED_LOCATION ${_epicsv4_${libname}_lib})
        # message("Found binary for ${libname} at ${_epicsv4_${libname}_lib}")

        add_library(_epicsv4_${libname}_intr INTERFACE)
        target_include_directories(_epicsv4_${libname}_intr INTERFACE
                ${EPICS4_DIR}/${libname}/include)
        target_link_libraries(_epicsv4_${libname}_intr INTERFACE
                _epicsv4_${libname}_bin ${deps})
        # message("link deps for ${libname} is _epicsv4_${libname}_bin and ${deps}")

        add_library(EPICSv4::${libname} ALIAS _epicsv4_${libname}_intr)
    endfunction()

    _epicsv4_add_library(pvDataCPP pvData "epics::ca")
    _epicsv4_add_library(pvAccessCPP pvAccess EPICSv4::pvDataCPP)
    _epicsv4_add_library(normativeTypesCPP nt EPICSv4::pvDataCPP)
    _epicsv4_add_library(pvDatabaseCPP pvDatabase EPICSv4::pvAccessCPP)
    _epicsv4_add_library(pvaSrvCPP pvaSrv EPICSv4::pvAccessCPP)
    _epicsv4_add_library(pvaClientCPP pvaClient "EPICSv4::pvAccessCPP;EPICSv4::normativeTypesCPP")

endif (EPICS4_DIR)