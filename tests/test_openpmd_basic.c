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
    result = beamphysics_read_metadata("tests/data/attr_count_32.h5", &metadata);

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

/* Test reading particle data from attr_count_32.h5 */
void test_read_particle_data_attr_count_32(void) {
    BeamPhysicsMD metadata;
    ParticleGroup pg;
    int result;

    /* First, read metadata */
    result = beamphysics_read_metadata("tests/data/attr_count_32.h5", &metadata);
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Allocate particle group for electron species */
    result = beamphysics_allocate_particle_group(&pg, "electron", &metadata);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT64(32, pg.num_particles);

    /* Read particle data */
    result = beamphysics_read_particle_group("tests/data/attr_count_32.h5", "electron", &pg);
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Verify the data values match the generated test data:
     * x=0, y=1, z=2, t=3, px=4, py=5, pz=6, weight=7, id=8, status=9 */
    TEST_ASSERT_NOT_NULL(pg.x);
    TEST_ASSERT_NOT_NULL(pg.y);
    TEST_ASSERT_NOT_NULL(pg.z);

    /* Check first particle's position values */
    TEST_ASSERT_EQUAL_DOUBLE(0.0, pg.x[0]);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, pg.y[0]);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, pg.z[0]);
    TEST_ASSERT_EQUAL_DOUBLE(3.0, pg.t[0]);

    /* Check first particle's momentum values */
    TEST_ASSERT_EQUAL_DOUBLE(4.0, pg.px[0]);
    TEST_ASSERT_EQUAL_DOUBLE(5.0, pg.py[0]);
    TEST_ASSERT_EQUAL_DOUBLE(6.0, pg.pz[0]);

    /* Check first particle's optional fields */
    TEST_ASSERT_EQUAL_DOUBLE(7.0, pg.weight[0]);
    TEST_ASSERT_EQUAL_INT64(8, pg.id[0]);
    TEST_ASSERT_EQUAL_INT64(9, pg.status[0]);

    /* Verify all particles have the same values */
    for (int i = 0; i < 32; i++) {
        TEST_ASSERT_EQUAL_DOUBLE(0.0, pg.x[i]);
        TEST_ASSERT_EQUAL_DOUBLE(1.0, pg.y[i]);
        TEST_ASSERT_EQUAL_DOUBLE(2.0, pg.z[i]);
    }

    /* Clean up */
    beamphysics_free_particle_group(&pg);
    beamphysics_free_metadata(&metadata);
}

/* Placeholder test for future HDF5 file creation */
void test_create_empty_particle_group(void) {
    /* TODO: Implement when library functions exist */
    TEST_IGNORE_MESSAGE("Not implemented yet");
}

/* Main test runner */
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_read_metadata_attr_count_32);
    RUN_TEST(test_read_particle_data_attr_count_32);
    RUN_TEST(test_create_empty_particle_group);

    return UNITY_END();
}
