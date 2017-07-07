
find_path(mx_MYRIEXPRESS_H myriexpress.h
        hints /opt/mx/include
        ${MX_PATH}/include
        )

find_library(mx_MYRIEXPRESS_SO myriexpress
        hints /opt/mx/lib
        ${MX_PATH}/lib
        )

if (mx_MYRIEXPRESS_H AND mx_MYRIEXPRESS_SO)
    set(MX_FOUND TRUE)

    add_library(_myriexpress SHARED IMPORTED)
    set_target_properties(_myriexpress PROPERTIES
            IMPORTED_LOCATION ${mx_MYRIEXPRESS_SO}
            )

    add_library(_myriexpress_intl INTERFACE)
    target_include_directories(_myriexpress_intl INTERFACE ${mx_MYRIEXPRESS_H})
    target_link_libraries(_myriexpress_intl INTERFACE _myriexpress)

    add_library(mx::myriexpress ALIAS _myriexpress_intl)

    Message("MX Libraries found")
else (mx_MYRIEXPRESS_H AND mx_MYRIEXPRESS_SO)
    SET (MX_FOUND FALSE)
    Message("MX Libraries not found")
endif(mx_MYRIEXPRESS_H AND mx_MYRIEXPRESS_SO)
