if (cds_find_clock_get_time)
    return()
endif(cds_find_clock_get_time)
set(cds_find_clock_get_time TRUE)

INCLUDE(CheckCSourceRuns)

set(CMAKE_REQUIRED_LIBRARIES "")

CHECK_C_SOURCE_RUNS(
"#include <time.h>

int main(int argc, char **argv)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return 0;
}" _cgt_NO_RT
)

set(CMAKE_REQUIRED_LIBRARIES "rt")
CHECK_C_SOURCE_RUNS(
        "#include <time.h>

int main(int argc, char **argv)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return 0;
}" _cgt_RT
)

if (_cgt_NO_RT)

    add_library(clock_gettime INTERFACE)

else(_cgt_NO_RT)

    if (_cgt_RT)
        add_library(clock_gettime INTERFACE)
        target_link_libraries(clock_gettime INTERFACE rt)
    else (_cgt_RT)

        Message(FATAL " Connot determine how to link with clock_gettime")

    endif (_cgt_RT)
endif(_cgt_NO_RT)