
if (cds_find_grace_included)
    return()
endif(cds_find_grace_included)
set(cds_find_grace_included TRUE)

find_path(grace_GRACE_HEADER grace_np.h
        hints /usr/include
        ${GRACE_PATH}/include
        )

find_library(grace_GRACE_LIB grace_np
        hints /usr/lib
        ${GRACE_PATH}/lib
        )


if (grace_GRACE_HEADER AND grace_GRACE_LIB)
    set(GRACE_FOUND TRUE)

    add_library(grace_ STATIC IMPORTED)
    set_target_properties(grace_ PROPERTIES
            IMPORTED_LOCATION ${grace_GRACE_LIB}
            )

    add_library(grace_intl INTERFACE)
    target_include_directories(grace_intl INTERFACE ${grace_GRACE_HEADER})
    target_link_libraries(grace_intl INTERFACE grace_)

    add_library(grace::grace ALIAS grace_intl)

    Message("Grace Libraries found")

else (grace_GRACE_HEADER AND grace_GRACE_LIB)
    SET (GRACE_FOUND FALSE)
    Message("Grace Libraries not found")
endif(grace_GRACE_HEADER AND grace_GRACE_LIB)
