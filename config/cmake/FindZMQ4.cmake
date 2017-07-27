
if (cds_find_zmq4_included)
    return()
endif(cds_find_zmq4_included)
set(cds_find_zmq4_included TRUE)


FIND_PACKAGE(PkgConfig)

pkg_check_modules(LibZMQ REQUIRED libzmq>=4.0.0)
if (LibZMQ_FOUND)
    set (_fcpp_lib_list "")

    find_library(libzmq4_LIBARY_PATH name zmq
            PATHS ${LibZMQ_LIBRARY_DIRS}
    )
    add_library(_zmq4 SHARED IMPORTED)
    set_target_properties(_zmq4 PROPERTIES
            IMPORTED_LOCATION ${libzmq4_LIBARY_PATH})

    add_library(_zmq4_intl INTERFACE)
    target_include_directories(_zmq4_intl INTERFACE ${LibZMQ_INCLUDE_DIRS})
    target_link_libraries(_zmq4_intl INTERFACE _zmq4)
    
    # Give the interface library a nice name for exporting
    add_library(zmq4::zmq ALIAS _zmq4_intl)
endif(LibZMQ_FOUND)

