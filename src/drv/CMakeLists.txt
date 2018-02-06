add_subdirectory(mbuf/mbuf_probe)

add_library(shmem STATIC shmem.c)
target_include_directories(shmem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(shmem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mbuf)

add_library(driver::shmem ALIAS shmem)