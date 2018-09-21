if (cds_find_dolphin_included)
    return()
endif(cds_find_dolphin_included)
set(cds_find_dolphin_included TRUE)

find_path(dolphin_sisci_types_h sisci_types.h
        hints /opt/DIS/include
        ${DOLPHIN_PATH}/include
        )

find_library(dolphin_sisci_so sisci
        hints /opt/DIS/lib64
        ${DOLPHIN_PATH}/lib64
        )

if (dolphin_sisci_types_h AND dolphin_sisci_so)
    SET (DOLPHIN_FOUND TRUE)
    Message ("Dolphin libraries were found")

    add_library(_sisci SHARED IMPORTED)
    set_target_properties(_sisci PROPERTIES
            IMPORTED_LOCATION ${dolphin_sisci_so}
            )

    add_library(_sisci_intl INTERFACE)
    target_include_directories(_sisci_intl INTERFACE
            ${dolphin_sisci_types_h}
            ${dolphin_sisci_types_h}/dis
            ${dolphin_sisci_types_h}/../src/include
            )
    target_compile_definitions(_sisci_intl INTERFACE
            HAVE_CONFIG_H
            OS_IS_LINUX=196616
            LINUX
            UNIX
            LITTLE_ENDIAN
            DIS_LITTLE_ENDIAN
            CPU_WORD_IS_64_BIT
            CPU_ADDR_IS_64_BIT
            CPU_WORD_SIZE=64
            CPU_ADDR_SIZE=64
            CPU_ARCH_IS_X86_64
            ADAPTER_IS_IX
            _REENTRANT
            )
    target_compile_options(_sisci_intl INTERFACE -m64)
    target_link_libraries(_sisci_intl INTERFACE _sisci)

    add_library(dolphin::sisci ALIAS _sisci_intl)
else (dolphin_sisci_types_h AND dolphin_sisci_so)
    SET (DOLPHIN_FOUND FALSE)
    Message ("Dolphin libraries not found")
endif (dolphin_sisci_types_h AND dolphin_sisci_so)