cmake_minimum_required(VERSION 3.14)
project(iq_imbalance_compensation CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

add_library(iq_imbalance STATIC src/iq_imbalance.cpp)
target_include_directories(iq_imbalance PUBLIC include)

add_executable(iq_demo src/main.cpp)
target_link_libraries(iq_demo PRIVATE iq_imbalance)

enable_testing()
add_executable(test_iq_imbalance tests/test_iq_imbalance.cpp)
target_link_libraries(test_iq_imbalance PRIVATE iq_imbalance)
add_test(NAME iq_unit_tests COMMAND test_iq_imbalance)
