cmake_minimum_required(VERSION 3.18)
project(cppexpect)
set(CMAKE_CXX_STANDARD 17)

# Configure lib
add_library(cppexpect cppexpect.h cppexpect.cpp)

# Build tests
option(BUILD_TESTS "Build tests" OFF)
if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()