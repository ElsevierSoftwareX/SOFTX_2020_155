find_package(ZMQ4)
find_path(CATCH_PATH catch.hpp
        HINTS /usr/include /usr/local/include ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(run_number run_number_main.cc run_number.cc run_number.hh)
target_link_libraries(run_number PUBLIC zmq)
target_compile_features(run_number PRIVATE cxx_nullptr)

add_executable(test_run_number tests/test_main.cc tests/test_run_number.cc tests/test_run_number_structs.cc run_number.cc run_number.hh)
target_include_directories(test_run_number PRIVATE ${CATCH_PATH})
target_compile_features(test_run_number PRIVATE cxx_nullptr)

add_executable(run_number_test_client run_number_test_client.cc)
target_link_libraries(run_number_test_client PUBLIC zmq)

add_test(_rn_basic_run_number_tests_ test_run_number)
