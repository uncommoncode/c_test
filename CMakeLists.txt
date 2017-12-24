cmake_minimum_required(VERSION 3.0)

project(c_test)
add_definitions(-DC_TEST_USE_PRINTF_RUNNER)
add_definitions(-DC_TEST_USE_PRINTF_TIMER)
add_library(c_test SHARED src/c_test.c)
set_target_properties(c_test PROPERTIES PUBLIC_HEADER "src/c_test.h")
install(TARGETS c_test 
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include/c_test
)
