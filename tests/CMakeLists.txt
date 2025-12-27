# Copy test data to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Test executables
add_executable(test_read_reference test_read_reference.c)

# Link against Unity and HDF5
target_link_libraries(test_read_reference
    PRIVATE
    unity
    parcel
    ${HDF5_LIBRARIES}
)

target_include_directories(test_read_reference
    PRIVATE
    ${HDF5_INCLUDE_DIRS}
)

# Register test with CTest
add_test(NAME ReadReferenceTests COMMAND test_read_reference WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_read_reference
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests..."
)
