
include(CheckCXXCompilerFlag)
foreach (ENABLE_SANITIZER ${ENABLE_SANITIZERS})
    set(_e_s_saved_flags ${CMAKE_REQUIRED_LIBRARIES})

    # Sanitizers should be run w/ at least basic optimization
    # and debugging.  Enable that if we know the flags
    check_cxx_compiler_flag("-O -g" SANITIZER_OPTIMIZE_FLAGS)
    if (${SANITIZER_OPTIMIZE_FLAGS})
        set(SANITIZER_EXTRA_FLAGS ${SANITIZER_EXTRA_FLAGS} -O -g)
    endif (${SANITIZER_OPTIMIZE_FLAGS})

    set(CMAKE_REQUIRED_LIBRARIES "asan")
        check_cxx_compiler_flag("-fsanitize=address -O -g" SANITIZER_ASAN_AVAILABLE)
    set(CMAKE_REQUIRED_LIBRARIES "tsan")
        check_cxx_compiler_flag("-fsanitize=thread -O -g"  SANITIZER_TSAN_AVAILABLE)
    set(CMAKE_REQUIRED_LIBRARIES ${_e_s_saved_flags})

    if (${ENABLE_SANITIZER} STREQUAL "address")
        if (${SANITIZER_ASAN_AVAILABLE})
            add_compile_options("-fsanitize=address -O -g")
            link_libraries("asan")
            message("Enabling asan")
        else(${SANITIZER_ASAN_AVAILABLE})
            message(FATAL_ERROR "Address sanitizer requested and not available")
        endif(${SANITIZER_ASAN_AVAILABLE})
    elseif(${ENABLE_SANITIZER} STREQUAL "thread")
        if (${SANITIZER_TSAN_AVAILABLE})
            add_compile_options("-fsanitize=thread" -fPIE -pie -O -g)
            LIST(APPEND CMAKE_EXE_LINKER_FLAGS -pie)
            link_libraries("tsan")
            message("Enabling tsan")
        else(${SANITIZER_TSAN_AVAILABLE})
            message(FATAL_ERROR "Thread sanitizer requested and not available")
        endif(${SANITIZER_TSAN_AVAILABLE})
    else(${ENABLE_SANITIZER} STREQUAL "address")
        message(FATAL_ERROR "Unknown/unsupported sanitizer requested ${ENABLE_SANITIZER}")
    endif(${ENABLE_SANITIZER} STREQUAL "address")

    add_compile_options(${SANITIZER_EXTRA_FLAGS})
endforeach(ENABLE_SANITIZER)