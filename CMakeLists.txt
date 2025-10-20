cmake_minimum_required(VERSION 3.14)
project(RTSE LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(pybind11 REQUIRED)

add_library(rtse_core STATIC
    core/rtree.cpp
)
target_include_directories(rtse_core PUBLIC ${PROJECT_SOURCE_DIR}/core)

pybind11_add_module(rtse binding/pybind.cpp)
target_link_libraries(rtse PRIVATE rtse_core)
target_include_directories(rtse PRIVATE ${pybind11_INCLUDE_DIRS})

add_executable(dummy tests/test_dummy.cpp)
set_target_properties(dummy PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)