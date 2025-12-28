#include "../deps/Unity-2.6.1/unity.h"

/* Define implementation before including header */
#define PARCEL_IMPLEMENTATION
#include "../parcel.h"

#include <string.h>

/* Unity setup and teardown functions */
void setUp(void) {
    /* This runs before each test */
}

void tearDown(void) {
    /* This runs after each test */
}

/* Test reading series and iteration metadata from attr_count_32.h5 */
void test_read_metadata_attr_count_32(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;
    char **species_names;
    int num_species;
    int64_t particle_count;

    /* Open series */
    result = pmd_open_series("tests/data/attr_count_32.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);

    /* Get iterations */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_TRUE(num_iterations > 0);

    /* Open first iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(iter);

    /* Get species */
    result = pmd_get_species(iter, &species_names, &num_species);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(1, num_species);
    TEST_ASSERT_NOT_NULL(species_names);
    TEST_ASSERT_NOT_NULL(species_names[0]);
    TEST_ASSERT_EQUAL_STRING("electron", species_names[0]);

    /* Get particle count */
    result = pmd_get_num_particles(iter, "electron", &particle_count);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT64(32, particle_count);

    /* Clean up */
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test reading particle data from attr_count_32.h5 */
void test_read_particle_data_attr_count_32(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/attr_count_32.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get iterations */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open first iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group for electron species */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(pg);
    TEST_ASSERT_EQUAL_INT64(32, pg->num_particles);

    /* Read particle data */
    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify the data values match the generated test data:
     * x=0, y=1, z=2, t=3, px=4, py=5, pz=6, weight=7, id=8, status=9 */
    TEST_ASSERT_NOT_NULL(pg->x);
    TEST_ASSERT_NOT_NULL(pg->y);
    TEST_ASSERT_NOT_NULL(pg->z);

    /* Check first particle's position values */
    TEST_ASSERT_EQUAL_DOUBLE(0.0, pg->x[0]);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, pg->y[0]);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, pg->z[0]);
    TEST_ASSERT_EQUAL_DOUBLE(3.0, pg->t[0]);

    /* Check first particle's momentum values (in eV/c) */
    TEST_ASSERT_DOUBLE_WITHIN(1e-30, 4.0, pg->px[0]);
    TEST_ASSERT_DOUBLE_WITHIN(1e-30, 5.0, pg->py[0]);
    TEST_ASSERT_DOUBLE_WITHIN(1e-30, 6.0, pg->pz[0]);

    /* Check first particle's optional fields */
    TEST_ASSERT_EQUAL_DOUBLE(7.0, pg->weight[0]);
    TEST_ASSERT_EQUAL_INT64(8, pg->id[0]);
    TEST_ASSERT_EQUAL_INT64(9, pg->status[0]);

    /* Verify all particles have the same values */
    for (int i = 0; i < 32; i++) {
        TEST_ASSERT_EQUAL_DOUBLE(0.0, pg->x[i]);
        TEST_ASSERT_EQUAL_DOUBLE(1.0, pg->y[i]);
        TEST_ASSERT_EQUAL_DOUBLE(2.0, pg->z[i]);
    }

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
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
