# Specify the minimum version of CMake (using 3.26.2).
cmake_minimum_required(VERSION 3.5)

# Variable for the name of the project.
set(this lsm6dsox_test)

project(${this} C CXX)

# Set the compiler options.

# C standard.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_OUTPUT_EXTENSION_REPLACE ON)
set(CMAKE_C_FLAGS "-g -O0 --coverage")

# C++ standard.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE ON)
set(CMAKE_CXX_FLAGS "-g -O0 --coverage")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Create OBJECT_DIR variable; generated objects and gcov output files will be placed here.
set(OBJECT_DIR ${CMAKE_BINARY_DIR}/CMakeFiles/${this}.dir)
message("-- Object files will be output to: ${OBJECT_DIR}")

# Tell CMake this project will do unit testing.
enable_testing()

# Definitions for testing
add_definitions(-DLSM6DSOX_TEST)

# Include the GTest framework manually download and copy into the googletest folder. The GTest
# framework can be downloaded here:
# https://github.com/google/googletest
# Recommended Version: 1.13.0
#add_subdirectory(googletest)

# Include the GTest framework and download it ad hoc.
# Refers to version 1.13.0.
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/b796f7d44681514f58a683a3a71ff17c94edb0c1.zip
)
# For Windows: prevent overriding the parent project's compiler/linker settings.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(dir_current .)
set(dir_src ./..)
#set(dir_common ./../common/src)

# Include directories.
include_directories(${dir_current})
include_directories(${dir_src})
#include_directories(${dir_common})

set(headers
    ${dir_current}/lsm6dsox_helper.h
    ${dir_src}/lsm6dsox.h
    ${dir_src}/lsm6dsox_reg.h
)

set(sources 
    ${dir_current}/lsm6dsox_test.cpp
    ${dir_current}/lsm6dsox_helper.c
)

add_executable(${this} ${sources} ${headers})
target_link_libraries(${this} PUBLIC
    gtest_main
)

add_test(
    NAME ${this}
    COMMAND ${this}
)
