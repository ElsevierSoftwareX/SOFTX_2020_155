add_subdirectory(mbuf/mbuf_probe)

add_library(shmem STATIC shmem.c rfm.c)
target_include_directories(shmem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(shmem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mbuf)

add_library(driver::shmem ALIAS shmem)

add_library(ini_parsing STATIC param.c crc.c)
target_include_directories(ini_parsing PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_library(driver::ini_parsing ALIAS ini_parsing)