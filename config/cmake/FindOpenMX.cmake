if (OPENMX_FOUND)
else (OPENMX_FOUND)


find_path(openmx_OPENMX_H open-mx.h
    hints /opt/open-mx/include
    ${OPENMX_PATH}/include
    )


find_library(openmx_OPENMX_SO open-mx
        hints /opt/open-mx/lib
        ${OPENMX_PATH}/lib
        )

if (openmx_OPENMX_H AND openmx_OPENMX_SO)
    set(OPENMX_FOUND TRUE)

    add_library(_openmx SHARED IMPORTED)
    set_target_properties(_openmx PROPERTIES
            IMPORTED_LOCATION ${openmx_OPENMX_SO}
            )

    add_library(_openmx_intl INTERFACE)
    target_include_directories(_openmx_intl INTERFACE ${openmx_OPENMX_H})
    target_link_libraries(_openmx_intl INTERFACE _openmx)

    add_library(openmx::openmx ALIAS _openmx_intl)

    Message("Open-MX Libraries found")
else (openmx_OPENMX_H AND openmx_OPENMX_SO)
    SET (OPENMX_FOUND FALSE)
    Message("Open-MX Libraries not found")
endif(openmx_OPENMX_H AND openmx_OPENMX_SO)

endif (OPENMX_FOUND)