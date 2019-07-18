
pkg_check_modules(FrameCPP REQUIRED framecpp)

if (FrameCPP_FOUND)
    set (_fcpp_lib_list "")

    # This function takes a external library name
    # looks it up in the framecpp library paths
    # creates a IMPORTED library target and adds
    # the target to the _fcpp_lib_list variable
    function(_fcpp_add_library libname)
        # find library MUST have a unique variable name
        # as it gets cached, hence the recursive name
        find_library(${libname}_LIBRARY_PATH name ${libname}
            PATHS ${FrameCPP_LIBRARY_DIRS}
                /lib /usr/lib
                /lib64 /usr/lib64
                /usr/lib/x86_64-linux-gnu
                /usr/local/lib /usr/local/lib64
            NO_DEFAULT_PATH)
        add_library(${libname} SHARED IMPORTED)
        set_target_properties(${libname} PROPERTIES
            IMPORTED_LOCATION ${${libname}_LIBRARY_PATH})
        set (_fcpp_lib_list ${_fcpp_lib_list} ${libname} PARENT_SCOPE)
    endfunction()

    # convert the pkg-config library list to cmake library targets
    foreach (_FCPP_LIB ${FrameCPP_LIBRARIES})
        _fcpp_add_library(${_FCPP_LIB})
    endforeach(_FCPP_LIB)

    #message("fcpp lib list ${_fcpp_lib_list}")
    # Create an interface library to attach all the includes
    # and actual framecpp libraries to
    add_library(_framecpp_intl INTERFACE)

    if (${FrameCPP_VERSION} VERSION_LESS 2.0)
        message(FATAL_ERROR "FrameCPP 2.0+ required")
    endif(${FrameCPP_VERSION} VERSION_LESS 2.0)

    if (${FrameCPP_VERSION} VERSION_LESS 2.3.4 AND NOT VERSION_LESS 2.0)
        # FrameCPP 2.0-2.3.3 use C++11 but do not specify it in the
        # compiler cflags.  So we need to manually require C++11
        target_compile_features(_framecpp_intl INTERFACE cxx_auto_type)
    else (${FrameCPP_VERSION} VERSION_LESS 2.3.4 AND NOT VERSION_LESS 2.0)
        target_compile_options(_framecpp_intl INTERFACE ${FrameCPP_CFLAGS_OTHER})
    endif (${FrameCPP_VERSION} VERSION_LESS 2.3.4 AND NOT VERSION_LESS 2.0)

    # The ldas-tools 2.5 release breaks ldas-tools up into
    # smaller packages.  Let the application know where to look
    # for version information
    if (${FrameCPP_VERSION} VERSION_LESS 2.5)
        target_compile_definitions(_framecpp_intl INTERFACE USE_LDAS_VERSION)
    else(${FrameCPP_VERSION} VERSION_LESS 2.5)
        target_compile_definitions(_framecpp_intl INTERFACE USE_FRAMECPP_VERSION)
    endif(${FrameCPP_VERSION} VERSION_LESS 2.5)

    target_include_directories(_framecpp_intl INTERFACE ${FrameCPP_INCLUDE_DIRS})
    target_link_libraries(_framecpp_intl INTERFACE ${_fcpp_lib_list})

    # framecpp 2.6 uses boost/shared_ptr instead of ldastools::al::shared_ptr
    if (${FrameCPP_VERSION} VERSION_LESS 2.6)
        # cmake 3.0.2 (Debian 8) doesn't support VERSION_GREATER_EQUAL
    else(${FrameCPP_VERSION} VERSION_LESS 2.6)
        find_package(Boost REQUIRED)
        target_include_directories(_framecpp_intl INTERFACE ${Boost_INCLUDE_DIRS})
    endif(${FrameCPP_VERSION} VERSION_LESS 2.6)

    # Give the interface library a nice name for exporting
    add_library(ldastools::framecpp ALIAS _framecpp_intl)
endif(FrameCPP_FOUND)
