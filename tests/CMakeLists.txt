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
add_executable(test_read_reference test_read_reference.c)
add_executable(test_utilities test_utilities.c)

# Link against Unity and HDF5
target_link_libraries(test_read_reference
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

target_include_directories(test_read_reference
    PRIVATE
    ${HDF5_INCLUDE_DIRS}
)

target_include_directories(test_utilities
    PRIVATE
    ${HDF5_INCLUDE_DIRS}
)

# Make test executable depend on generated test data
if(Python3_FOUND)
    add_dependencies(test_read_reference generate_test_data)
endif()

# Register tests with CTest
add_test(NAME ReadReferenceTests COMMAND test_read_reference WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_test(NAME UtilitiesTests COMMAND test_utilities WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_read_reference test_utilities
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests..."
)
