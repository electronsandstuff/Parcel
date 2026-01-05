cmake_minimum_required(VERSION 3.12)
project(Parcel VERSION 0.1.0 LANGUAGES C CXX)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find HDF5
find_package(HDF5 REQUIRED COMPONENTS C)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${HDF5_INCLUDE_DIRS})

# Header-only library interface
add_library(parcel INTERFACE)
target_include_directories(parcel INTERFACE ${CMAKE_SOURCE_DIR})

# Unity testing framework
add_library(unity STATIC
    deps/Unity-2.6.1/unity.c
)
target_include_directories(unity PUBLIC Unity-2.6.1)
target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)
# Disable clang-tidy for third-party code
set_target_properties(unity PROPERTIES CXX_CLANG_TIDY "")
set_target_properties(unity PROPERTIES C_CLANG_TIDY "")

# Setup clang-tidy (must be before add_subdirectory)
find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_EXE}")
else()
    message(WARNING "clang-tidy not found")
endif()

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)

