cmake_minimum_required(VERSION 3.14)
project(lottery_monte_carlo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

# Core library: draw generation and uniformity analysis.
add_library(lottery STATIC src/lottery.cpp)
target_include_directories(lottery PUBLIC include)

# Demonstration program.
add_executable(lottery_demo src/main.cpp)
target_link_libraries(lottery_demo PRIVATE lottery)

# Unit tests, runnable via ctest.
enable_testing()
add_executable(test_lottery tests/test_lottery.cpp)
target_link_libraries(test_lottery PRIVATE lottery)
add_test(NAME lottery_unit_tests COMMAND test_lottery)
