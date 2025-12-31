# Generate test data files using Python script
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/data/.generated
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/python/generate_test_data.py ${CMAKE_CURRENT_BINARY_DIR}/data
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/data/.generated
        DEPENDS ${CMAKE_SOURCE_DIR}/python/generate_test_data.py
        COMMENT "Generating test data files..."
        VERBATIM
    )
    add_custom_target(generate_test_data ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/data/.generated
    )
else()
    message(WARNING "Python3 not found - test data will not be generated automatically")
endif()

# Test executables
add_executable(test_read test_read.c)
add_executable(test_utilities test_utilities.c)
add_executable(test_cpp_api test_cpp_api.cpp)
add_executable(test_write test_write.c)
add_executable(test_read_write test_read_write.c)
add_executable(test_generate_openpmd test_generate_openpmd.c)

# Link against Unity and HDF5
target_link_libraries(test_read
    PRIVATE
    unity
    parcel
    ${HDF5_LIBRARIES}
)

target_link_libraries(test_utilities
    PRIVATE
    unity
    parcel
    ${HDF5_LIBRARIES}
)

target_link_libraries(test_cpp_api
    PRIVATE
    unity
    parcel
    ${HDF5_LIBRARIES}
)

target_link_libraries(test_write
    PRIVATE
    unity
    parcel
    ${HDF5_LIBRARIES}
)

target_link_libraries(test_read_write
    PRIVATE
    unity
    parcel
    ${HDF5_LIBRARIES}
)

target_link_libraries(test_generate_openpmd
    PRIVATE
    unity
    parcel
    ${HDF5_LIBRARIES}
)

target_include_directories(test_read
    PRIVATE
    ${HDF5_INCLUDE_DIRS}
)

target_include_directories(test_utilities
    PRIVATE
    ${HDF5_INCLUDE_DIRS}
)

target_include_directories(test_cpp_api
    PRIVATE
    ${HDF5_INCLUDE_DIRS}
)

target_include_directories(test_write
    PRIVATE
    ${HDF5_INCLUDE_DIRS}
)

target_include_directories(test_read_write
    PRIVATE
    ${HDF5_INCLUDE_DIRS}
)

target_include_directories(test_generate_openpmd
    PRIVATE
    ${HDF5_INCLUDE_DIRS}
)

# Make test executables depend on generated test data
if(Python3_FOUND)
    add_dependencies(test_read generate_test_data)
    add_dependencies(test_cpp_api generate_test_data)
    add_dependencies(test_write generate_test_data)
    add_dependencies(test_read_write generate_test_data)
endif()

# Register tests with CTest
add_test(NAME UtilitiesTests COMMAND test_utilities WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME ReadTests COMMAND test_read WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME WriteTests COMMAND test_write WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME ReadWriteTests COMMAND test_read_write WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME GenerateOpenPMDTests COMMAND test_generate_openpmd WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME CppApiTests COMMAND test_cpp_api WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_read test_utilities test_cpp_api test_write test_read_write test_generate_openpmd
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests..."
)
