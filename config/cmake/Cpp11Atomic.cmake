
if (cds_check_cpp11_atomic_included)
    return()
endif(cds_check_cpp11_atomic_included)
set(cds_check_cpp11_atomic_included TRUE)

INCLUDE(Cpp11)
INCLUDE(CheckCXXSourceCompiles)

set (CMAKE_REQUIRED_FLAGS_cpp11_atomic_backup_ ${CMAKE_REQUIRED_FLAGS})
set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CPP11_FLAG}")

CHECK_CXX_SOURCE_COMPILES(
"
#include <atomic>
int main(int argc, char *argv[])
{
    std::atomic<int> i;
    return 0;
}
" CXX_HAS_ATOMIC)
