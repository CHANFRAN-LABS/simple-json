cmake_minimum_required(VERSION 3.22)
project("SimpleJsonTests")

find_package(GTest REQUIRED)

add_executable(tests.out src/UnitTest.cpp)
target_link_libraries(tests.out ${GTEST_LIBRARIES})

enable_testing()
add_test(add tests.out)