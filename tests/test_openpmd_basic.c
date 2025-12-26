#include "../deps/Unity-2.6.1/unity.h"

/* Define implementation before including header */
#define OPENPMD_BeamPhysics_C_IMPLEMENTATION
#include "../openpmd_beamphysics.h"

#include <string.h>

/* Unity setup and teardown functions */
void setUp(void) {
    /* This runs before each test */
}

void tearDown(void) {
    /* This runs after each test */
}

/* Test reading metadata from attr_count_32.h5 */
void test_read_metadata_attr_count_32(void) {
    BeamPhysicsMD metadata;
    int result;

    /* Read metadata from test file */
    result = beamphysics_read_metadata("data/attr_count_32.h5", &metadata);

    /* Check that read was successful */
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Check number of species */
    TEST_ASSERT_EQUAL_INT(1, metadata.num_species);

    /* Check species name is "electron" */
    TEST_ASSERT_NOT_NULL(metadata.species_names);
    TEST_ASSERT_NOT_NULL(metadata.species_names[0]);
    TEST_ASSERT_EQUAL_STRING("electron", metadata.species_names[0]);

    /* Check particle count is 32 */
    TEST_ASSERT_NOT_NULL(metadata.num_particles);
    TEST_ASSERT_EQUAL_INT64(32, metadata.num_particles[0]);

    /* Clean up */
    beamphysics_free_metadata(&metadata);
}

/* Placeholder test for future HDF5 file creation */
void test_create_empty_particle_group(void) {
    /* TODO: Implement when library functions exist */
    TEST_IGNORE_MESSAGE("Not implemented yet");
}

/* Placeholder test for future HDF5 file reading */
void test_read_particle_group(void) {
    /* TODO: Implement when library functions exist */
    TEST_IGNORE_MESSAGE("Not implemented yet");
}

/* Main test runner */
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_read_metadata_attr_count_32);
    RUN_TEST(test_create_empty_particle_group);
    RUN_TEST(test_read_particle_group);

    return UNITY_END();
}
