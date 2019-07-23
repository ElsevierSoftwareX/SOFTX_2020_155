
if (cds_check_cpp11_included)
    return()
endif(cds_check_cpp11_included)
set(cds_check_cpp11_included TRUE)

INCLUDE(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG(-std=c++0x HAS_CXX_0X)
CHECK_CXX_COMPILER_FLAG(-std=c++11 HAS_CXX_11)

if (${HAS_CXX_11})
    set(CPP11_FLAG "-std=c++11")
else (${HAS_CXX_11})
    if (${HAS_CXX_0X})
        set(CPP11_FLAG "-std=c++0x")
    else (${HAS_CXX_0X})
        set(CPP11_FLAG "")
    endif (${HAS_CXX_0X})
endif (${HAS_CXX_11})


macro(target_requires_cpp11 target mode)
    if (${CMAKE_VERSION} VERSION_GREATER "3.6.99")
        target_compile_features(${target} ${mode} cxx_auto_type)
    else (${CMAKE_VERSION} VERSION_GREATER "3.6.99")
        target_compile_options(${target} ${mode} ${CPP11_FLAG})
    endif (${CMAKE_VERSION} VERSION_GREATER "3.6.99")
endmacro()
