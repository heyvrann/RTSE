cmake_minimum_required(VERSION 3.14)
project(RTSE LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 REQUIRED)

enable_testing()
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

add_library(rtse_core STATIC
    core/rtree.cpp
)
target_include_directories(rtse_core PUBLIC ${PROJECT_SOURCE_DIR}/core)

pybind11_add_module(rtse binding/pybind.cpp)
target_link_libraries(rtse PRIVATE rtse_core)
set_target_properties(rtse PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

add_executable(test_rtree tests/test_rtree.cpp)
target_link_libraries(test_rtree PRIVATE GTest::gtest_main)
target_link_libraries(test_rtree PRIVATE rtse_core)
set_target_properties(test_rtree PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/"    
)

gtest_discover_tests(test_rtree)