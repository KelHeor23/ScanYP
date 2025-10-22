cmake_minimum_required(VERSION 3.25)
project(scan VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(GTest REQUIRED)

set(target scan)

add_library(${target} INTERFACE)

target_include_directories(${target} INTERFACE include/)

set(test_target scan_tests)

add_executable(${test_target} tests/main.cpp tests/scan_test.cpp)
target_link_libraries(${test_target} PRIVATE ${target} GTest::GTest GTest::Main)

# Включаем тестирование
enable_testing()
add_test(NAME ${test_target} COMMAND ${PROJECT_NAME}_tests)