add_library(util OBJECT
        modelrate.c)
target_include_directories(util PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")

add_executable(test_util test_util.c)
target_link_libraries(test_util util)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME "test_util"
        COMMAND "./test_util"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")