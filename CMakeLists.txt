cmake_minimum_required(VERSION 3.12)
project(OpenPMD_BeamPhysics_C VERSION 0.1.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find HDF5
find_package(HDF5 REQUIRED COMPONENTS C)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${HDF5_INCLUDE_DIRS})

# Header-only library interface
add_library(openpmd_beamphysics INTERFACE)
target_include_directories(openpmd_beamphysics INTERFACE ${CMAKE_SOURCE_DIR})

# Unity testing framework
add_library(unity STATIC
    deps/Unity-2.6.1/unity.c
)
target_include_directories(unity PUBLIC Unity-2.6.1)
target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)
