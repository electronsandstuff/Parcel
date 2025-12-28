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

/* =========================================================================
 * Series Tests
 * ========================================================================= */

/* Test: Valid file-based series with multiple files matching pattern
 * File: tests/data/file_based_series/data_{0,1,2}.h5
 * Tests: Basic file-based iteration enumeration and opening */
void test_file_based_series_basic(void) {
    pmd_series *series;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open the first file in the series */
    result = pmd_open_series("tests/data/file_based_series/data_0.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);

    /* Verify iteration encoding */
    TEST_ASSERT_EQUAL_INT(PMD_FILE_BASED, series->iteration_encoding);

    /* Get iterations - should find data_0.h5, data_1.h5, data_2.h5 */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(3, num_iterations);

    /* Verify iteration indices are sorted correctly */
    TEST_ASSERT_EQUAL_INT64(0, iterations[0]);
    TEST_ASSERT_EQUAL_INT64(1, iterations[1]);
    TEST_ASSERT_EQUAL_INT64(2, iterations[2]);

    /* Open each iteration and verify we can read it */
    for (int i = 0; i < 3; i++) {
        pmd_iteration *iter;
        result = pmd_open_iteration(series, iterations[i], &iter);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
        TEST_ASSERT_NOT_NULL(iter);

        /* Verify iteration number */
        TEST_ASSERT_EQUAL_INT64(iterations[i], iter->iteration_index);

        /* Verify we can read species */
        char **species_names;
        int num_species;
        result = pmd_get_species(iter, &species_names, &num_species);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
        TEST_ASSERT_EQUAL_INT(1, num_species);
        TEST_ASSERT_EQUAL_STRING("electron", species_names[0]);

        pmd_close_iteration(iter);
    }

    pmd_close_series(series);
}

/* Test: Open file-based series using pattern path
 * File: tests/data/file_based_series/data_%T.h5 (pattern, not specific file)
 * Tests: Opening series with %T pattern instead of specific iteration file */
void test_file_based_series_pattern_path(void) {
    pmd_series *series;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open using pattern path data_%T.h5 instead of specific file data_0.h5 */
    result = pmd_open_series("tests/data/file_based_series/data_%T.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);

    /* Verify iteration encoding */
    TEST_ASSERT_EQUAL_INT(PMD_FILE_BASED, series->iteration_encoding);

    /* Get iterations - should find data_0.h5, data_1.h5, data_2.h5 */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(3, num_iterations);

    /* Verify iteration indices */
    TEST_ASSERT_EQUAL_INT64(0, iterations[0]);
    TEST_ASSERT_EQUAL_INT64(1, iterations[1]);
    TEST_ASSERT_EQUAL_INT64(2, iterations[2]);

    /* Verify we can open and read from an iteration */
    pmd_iteration *iter;
    result = pmd_open_iteration(series, iterations[1], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(iter);
    TEST_ASSERT_EQUAL_INT64(1, iter->iteration_index);

    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: File-based series with other non-matching files in directory
 * File: tests/data/file_based_series_with_other/sim_{0,1,2}.h5
 * Tests: Iteration enumeration ignores non-matching files */
void test_file_based_series_with_other_files(void) {
    pmd_series *series;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series - should only match sim_%T.h5 pattern */
    result = pmd_open_series("tests/data/file_based_series_with_other/sim_0.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);

    /* Get iterations - should find only sim_0.h5, sim_1.h5, sim_2.h5 */
    /* Other files like other_data.h5, sim_backup.h5, readme.txt should be ignored */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(3, num_iterations);

    /* Verify correct iterations found (and sorted) */
    TEST_ASSERT_EQUAL_INT64(0, iterations[0]);
    TEST_ASSERT_EQUAL_INT64(1, iterations[1]);
    TEST_ASSERT_EQUAL_INT64(2, iterations[2]);

    pmd_close_series(series);
}

/* Test: Files with multiple %T present in pattern
 * File: tests/data/file_based_multiple_percent_t/data_{0,1}_iter_{0,1}.h5
 * Tests: Only first %T is used for iteration matching */
void test_file_based_multiple_percent_t(void) {
    pmd_series *series;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series with pattern data_%T_iter_%T.h5 */
    result = pmd_open_series("tests/data/file_based_multiple_percent_t/data_0_iter_0.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);

    /* Get iterations - only first %T should be used for iteration matching */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(2, num_iterations);

    /* Verify iterations */
    TEST_ASSERT_EQUAL_INT64(0, iterations[0]);
    TEST_ASSERT_EQUAL_INT64(1, iterations[1]);

    pmd_close_series(series);
}

/* Test: Group-based series with multiple iterations
 * File: tests/data/valid_multiple_iterations.h5
 * Tests: Basic group-based iteration enumeration */
void test_group_based_series_multiple_iterations(void) {
    pmd_series *series;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open group-based series */
    result = pmd_open_series("tests/data/valid_multiple_iterations.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);

    /* Verify iteration encoding */
    TEST_ASSERT_EQUAL_INT(PMD_GROUP_BASED, series->iteration_encoding);

    /* Get iterations - should find iterations 0, 1, 2 */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(3, num_iterations);

    /* Verify iteration indices are sorted */
    TEST_ASSERT_EQUAL_INT64(0, iterations[0]);
    TEST_ASSERT_EQUAL_INT64(1, iterations[1]);
    TEST_ASSERT_EQUAL_INT64(2, iterations[2]);

    /* Open each iteration and verify we can access it */
    for (int i = 0; i < 3; i++) {
        pmd_iteration *iter;
        result = pmd_open_iteration(series, iterations[i], &iter);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
        TEST_ASSERT_NOT_NULL(iter);
        TEST_ASSERT_EQUAL_INT64(iterations[i], iter->iteration_index);

        /* Verify we can read species */
        char **species_names;
        int num_species;
        result = pmd_get_species(iter, &species_names, &num_species);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
        TEST_ASSERT_EQUAL_INT(1, num_species);

        pmd_close_iteration(iter);
    }

    pmd_close_series(series);
}

/* Test: Multiple groups in HDF5 that don't match iteration pattern
 * File: tests/data/group_based_non_matching_groups.h5
 * Tests: Non-numeric groups are ignored during iteration enumeration */
void test_group_based_non_matching_groups(void) {
    pmd_series *series;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/group_based_non_matching_groups.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);

    /* Get iterations - should only find numeric iterations 0, 1, 2 */
    /* Groups like data/metadata, data/config, data/not_a_number should be ignored */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(3, num_iterations);

    /* Verify only numeric iterations found */
    TEST_ASSERT_EQUAL_INT64(0, iterations[0]);
    TEST_ASSERT_EQUAL_INT64(1, iterations[1]);
    TEST_ASSERT_EQUAL_INT64(2, iterations[2]);

    pmd_close_series(series);
}

/* Test: iterationFormats with prefix and suffix around %T
 * File: tests/data/iteration_format_prefix_suffix.h5
 * Tests: Iteration pattern matching with prefix and suffix (e.g., step_%T_final) */
void test_iteration_format_prefix_suffix(void) {
    pmd_series *series;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series with pattern /data/step_%T_final/ */
    result = pmd_open_series("tests/data/iteration_format_prefix_suffix.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);

    /* Get iterations - should find step_0_final, step_1_final, step_2_final */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(3, num_iterations);

    /* Verify iterations are correctly parsed and sorted */
    TEST_ASSERT_EQUAL_INT64(0, iterations[0]);
    TEST_ASSERT_EQUAL_INT64(1, iterations[1]);
    TEST_ASSERT_EQUAL_INT64(2, iterations[2]);

    /* Open an iteration to verify path construction works correctly */
    pmd_iteration *iter;
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(iter);
    TEST_ASSERT_EQUAL_INT64(0, iter->iteration_index);

    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Group-based series with complex pattern
 * File: tests/data/group_based_complex_pattern.h5
 * Tests: Complex iteration pattern (e.g., /simulations/run_%T_data/) with non-sequential indices */
void test_group_based_complex_pattern(void) {
    pmd_series *series;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series with pattern /simulations/run_%T_data/ */
    result = pmd_open_series("tests/data/group_based_complex_pattern.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_NOT_NULL(series);

    /* Get iterations - should find iterations 0, 10, 100 (non-sequential) */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(3, num_iterations);

    /* Verify iterations are sorted numerically (not lexicographically) */
    TEST_ASSERT_EQUAL_INT64(0, iterations[0]);
    TEST_ASSERT_EQUAL_INT64(10, iterations[1]);
    TEST_ASSERT_EQUAL_INT64(100, iterations[2]);

    /* Open each iteration and verify particle data can be read */
    for (int i = 0; i < 3; i++) {
        pmd_iteration *iter;
        char **species_names;
        int num_species;

        result = pmd_open_iteration(series, iterations[i], &iter);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
        TEST_ASSERT_EQUAL_INT64(iterations[i], iter->iteration_index);

        /* Get species */
        result = pmd_get_species(iter, &species_names, &num_species);
        TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
        TEST_ASSERT_EQUAL_INT(1, num_species);
        TEST_ASSERT_EQUAL_STRING("electron", species_names[0]);

        pmd_close_iteration(iter);
    }

    pmd_close_series(series);
}

/* Main test runner */
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_read_metadata_attr_count_32);
    RUN_TEST(test_read_particle_data_attr_count_32);

    /* Series tests */
    RUN_TEST(test_file_based_series_basic);
    RUN_TEST(test_file_based_series_pattern_path);
    RUN_TEST(test_file_based_series_with_other_files);
    RUN_TEST(test_file_based_multiple_percent_t);
    RUN_TEST(test_group_based_series_multiple_iterations);
    RUN_TEST(test_group_based_non_matching_groups);
    RUN_TEST(test_iteration_format_prefix_suffix);
    RUN_TEST(test_group_based_complex_pattern);

    return UNITY_END();
}
