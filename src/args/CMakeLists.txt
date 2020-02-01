add_library(args args.cc)
target_include_directories(args PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_requires_cpp11(args PRIVATE)

add_executable(test_args tests/test_main.cc tests/test_args.cc)
target_include_directories(test_args PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(test_args args)
target_requires_cpp11(test_args PUBLIC)
