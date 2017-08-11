add_library(tai STATIC
        tai_add.c
        tai_now.c
        tai_pack.c
        tai_sub.c
        tai_unpack.c
        taia_add.c
        taia_approx.c
        taia_fmtfrac.c
        taia_frac.c
        taia_half.c
        taia_less.c
        taia_now.c
        taia_pack.c
        taia_sub.c
        taia_tai.c
        taia_unpack.c
        caldate_fmjd.c
        caldate_mjd.c
        caldate_fmt.c
        caldate_scan.c
        leapsecs_read.c
        leapsecs_init.c
        leapsecs_add.c
        leapsecs_sub.c
        caltime_tai.c
        caltime_utc.c
        caltime_fmt.c
        caltime_scan.c
        utc-to-gps.c
        gps-to-utc.c)
add_library(tai::tai ALIAS tai)

add_executable(tai_leapsecs leapsecs.c)
target_link_libraries(tai_leapsecs tai)

add_executable(tai_check check.c)
target_link_libraries(tai_check tai)

add_executable(tai_easter easter.c)
target_link_libraries(tai_easter tai)

add_executable(tai_yearcal yearcal.c)
target_link_libraries(tai_yearcal tai)

add_executable(tai_nowutc nowutc.c)
target_link_libraries(tai_nowutc tai)

add_executable(tai_convert convert.c)
target_link_libraries(tai_convert tai)

add_custom_target(build_leapsec.dat ALL
        tai_leapsecs < ${CMAKE_CURRENT_SOURCE_DIR}/leapsecs.txt > leapsecs.dat
        DEPENDS tai_leapsecs leapsecs.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        SOURCES leapsecs.txt
        )
