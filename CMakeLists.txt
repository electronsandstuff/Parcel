cmake_minimum_required(VERSION 3.12)
project(Parcel VERSION 1.0.0 LANGUAGES C CXX)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)  # Disable GNU extensions, enforce strict C99

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# C99 compliance flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C>:-std=c99>       # Enforce C99 standard
        $<$<COMPILE_LANGUAGE:C>:-pedantic>      # Strict ISO C compliance
        $<$<COMPILE_LANGUAGE:C>:-Wall>          # Enable common warnings
        $<$<COMPILE_LANGUAGE:C>:-Wextra>        # Enable extra warnings
    )
elseif(MSVC)
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C>:/W4>            # Warning level 4
        $<$<COMPILE_LANGUAGE:C>:/std:c11>       # MSVC uses C11 mode for C99 features
    )
    # Silence MSVC deprecation warnings for the standard C functions this project uses
    # (C4996 on strncpy/strcpy/sscanf/fopen/gmtime, and on the POSIX name strdup).
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_WARNINGS
    )
endif()

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
option(CLANG_TIDY "Run clang-tidy everytime the program is built" Off)
if(CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
        set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_EXE}")
    else()
        message(WARNING "clang-tidy not found")
    endif()
endif()

option(DEBUG "Enable debug flags" Off)
if(DEBUG)
    add_compile_options(-g)
endif()

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)

