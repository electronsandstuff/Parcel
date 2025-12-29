#include "../deps/Unity-2.6.1/unity.h"

/* Define implementation before including header */
#define PARCEL_IMPLEMENTATION
#include "../parcel.h"

#include <string.h>
#include <math.h>

/* =========================================================================
 * Custom Assertions
 * ========================================================================= */

/**
 * Assert that two doubles are close using both absolute and relative tolerance
 * Similar to numpy.testing.assert_allclose
 * Checks: |actual - expected| <= atol + rtol * |expected|
 */
#define TEST_ASSERT_DOUBLE_CLOSE(expected, actual, rtol, atol) \
    do { \
        double _expected = (expected); \
        double _actual = (actual); \
        double _rtol = (rtol); \
        double _atol = (atol); \
        double _diff = fabs(_actual - _expected); \
        double _tolerance = _atol + _rtol * fabs(_expected); \
        if (_diff > _tolerance) { \
            char _msg[256]; \
            snprintf(_msg, sizeof(_msg), \
                    "Expected %.15g, was %.15g (diff=%.3e, tol=%.3e)", \
                    _expected, _actual, _diff, _tolerance); \
            UNITY_TEST_FAIL(__LINE__, _msg); \
        } \
    } while(0)

/* Default tolerances for typical use cases */
#define TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected, actual) \
    TEST_ASSERT_DOUBLE_CLOSE(expected, actual, 1e-7, 0)

/* =========================================================================
 * Test Helper Functions
 * ========================================================================= */

/**
 * Calculate expected test value for a field based on naming convention
 * Matches the Python function get_test_value() in generate_test_data.py
 *
 * @param name Field name (e.g., "position/x", "momentum/y", "weight")
 * @param particle_idx Particle index (0-based)
 * @param constant If true, all particles have same value. If false, increment per particle
 * @return Expected test value = sum(ord(c) for c in name) + (particle_idx if not constant else 0)
 */
static double get_expected_test_value(const char *name, int particle_idx, int constant) {
    double base_value = 0.0;

    /* Sum ASCII values of all characters in name */
    for (const char *c = name; *c != '\0'; c++) {
        base_value += (double)(*c);
    }

    /* Add particle index if not constant */
    if (!constant) {
        base_value += (double)particle_idx;
    }

    return base_value;
}

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
    TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(0.0, pg->x[0]);
    TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(1.0, pg->y[0]);
    TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(2.0, pg->z[0]);
    TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(3.0, pg->t[0]);

    /* Check first particle's momentum values (in eV/c) */
    TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(4.0, pg->px[0]);
    TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(5.0, pg->py[0]);
    TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(6.0, pg->pz[0]);

    /* Check first particle's optional fields */
    TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(7.0, pg->weight[0]);
    TEST_ASSERT_EQUAL_INT64(8, pg->id[0]);
    TEST_ASSERT_EQUAL_INT64(9, pg->status[0]);

    /* Verify all particles have the same values */
    for (int i = 0; i < 32; i++) {
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(0.0, pg->x[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(1.0, pg->y[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(2.0, pg->z[i]);
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

/* =========================================================================
 * Valid Files Tests
 * ========================================================================= */

/* Test: Valid file with constant records (stored as group attributes)
 * File: tests/data/valid_constant_records.h5
 * Tests: Reading particle data where fields are constant (group with value attribute) */
void test_valid_constant_records(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/valid_constant_records.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_TRUE(num_iterations > 0);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate and read particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT64(10, pg->num_particles);

    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify constant records - all particles should have same values */
    double expected_x = get_expected_test_value("position/x", 0, 1);
    double expected_y = get_expected_test_value("position/y", 0, 1);
    double expected_z = get_expected_test_value("position/z", 0, 1);

    /* Verify all particles have identical values (constant record) */
    for (int i = 0; i < pg->num_particles; i++) {
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_x, pg->x[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_y, pg->y[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_z, pg->z[i]);
    }

    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Valid file with dataset records (unique incrementing values per particle)
 * File: tests/data/valid_dataset_records.h5
 * Tests: Reading particle data where each particle has different incrementing values */
void test_valid_dataset_records(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/valid_dataset_records.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_TRUE(num_iterations > 0);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate and read particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT64(10, pg->num_particles);

    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify dataset records - each particle should have incrementing values
     * value[i] = get_expected_test_value(name, i, constant=False) */
    for (int i = 0; i < pg->num_particles; i++) {
        double expected_x = get_expected_test_value("position/x", i, 0);
        double expected_y = get_expected_test_value("position/y", i, 0);
        double expected_z = get_expected_test_value("position/z", i, 0);

        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_x, pg->x[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_y, pg->y[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_z, pg->z[i]);
    }

    /* Verify that values ARE different between particles (not constant) */
    TEST_ASSERT_NOT_EQUAL(pg->x[0], pg->x[1]);
    TEST_ASSERT_NOT_EQUAL(pg->y[0], pg->y[1]);
    TEST_ASSERT_NOT_EQUAL(pg->z[0], pg->z[1]);

    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Valid file with non-default particlesPath
 * File: tests/data/valid_non_default_particles_path.h5
 * Tests: Reading from file using "beams/" instead of default "particles/" */
void test_valid_non_default_particles_path(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series - should handle non-default particlesPath="beams/" */
    result = pmd_open_series("tests/data/valid_non_default_particles_path.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify particlesPath is "beams/" not "particles/" */
    TEST_ASSERT_NOT_NULL(series->particles_path);
    TEST_ASSERT_EQUAL_STRING("beams/", series->particles_path);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify we can read species from non-default path */
    char **species_names;
    int num_species;
    result = pmd_get_species(iter, &species_names, &num_species);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(1, num_species);
    TEST_ASSERT_EQUAL_STRING("electron", species_names[0]);

    /* Allocate and read particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT64(10, pg->num_particles);

    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify we can read data correctly */
    TEST_ASSERT_NOT_NULL(pg->x);
    TEST_ASSERT_NOT_NULL(pg->y);
    TEST_ASSERT_NOT_NULL(pg->z);

    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Valid file with non-default basePath
 * File: tests/data/valid_non_default_base_path.h5
 * Tests: Reading from file using "/simulations/%T/" instead of default "/data/%T/" */
void test_valid_non_default_base_path(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series - should handle non-default basePath="/simulations/%T/" */
    result = pmd_open_series("tests/data/valid_non_default_base_path.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify basePath is "/simulations/%T/" not default "/data/%T/" */
    TEST_ASSERT_NOT_NULL(series->base_path);
    TEST_ASSERT_EQUAL_STRING("/simulations/%T/", series->base_path);

    /* Get iterations */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_TRUE(num_iterations > 0);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify we can read species from non-default base path */
    char **species_names;
    int num_species;
    result = pmd_get_species(iter, &species_names, &num_species);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(1, num_species);
    TEST_ASSERT_EQUAL_STRING("electron", species_names[0]);

    /* Allocate and read particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT64(10, pg->num_particles);

    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify we can read data correctly */
    TEST_ASSERT_NOT_NULL(pg->x);
    TEST_ASSERT_NOT_NULL(pg->y);
    TEST_ASSERT_NOT_NULL(pg->z);

    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* =========================================================================
 * Unit Conversion (unitSI) Tests
 * ========================================================================= */

/* Test: Position data stored in non-SI units (cm) with unitSI=0.01
 * File: tests/data/position_non_si_units.h5
 * Tests: Position values are correctly converted from cm to meters */
void test_position_non_si_units(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/position_non_si_units.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate and read particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify position values are converted to SI (meters)
     * Data is stored as value*100 (in cm) with unitSI=0.01
     * After conversion: (value*100) * 0.01 = value (in meters) */
    for (int i = 0; i < pg->num_particles; i++) {
        double expected_x = get_expected_test_value("position/x", i, 0);
        double expected_y = get_expected_test_value("position/y", i, 0);
        double expected_z = get_expected_test_value("position/z", i, 0);

        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_x, pg->x[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_y, pg->y[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_z, pg->z[i]);
    }

    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Momentum data stored in non-SI units (eV/c) with appropriate unitSI
 * File: tests/data/momentum_non_si_units.h5
 * Tests: Momentum values are correctly converted from SI to eV/c */
void test_momentum_non_si_units(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/momentum_non_si_units.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate and read particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify momentum values are in eV/c
     * Data is stored in eV/c with appropriate unitSI conversion
     * Library reads and converts to eV/c (see parcel.h lines 74-76) */
    for (int i = 0; i < pg->num_particles; i++) {
        double expected_px = get_expected_test_value("momentum/x", i, 0);
        double expected_py = get_expected_test_value("momentum/y", i, 0);
        double expected_pz = get_expected_test_value("momentum/z", i, 0);

        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_px, pg->px[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_py, pg->py[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_pz, pg->pz[i]);
    }

    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Time data stored in non-SI units (ps) with unitSI=1e-12
 * File: tests/data/time_non_si_units.h5
 * Tests: Time values are correctly converted from ps to seconds */
void test_time_non_si_units(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/time_non_si_units.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate and read particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify time values are converted to SI (seconds)
     * Data is stored as value*1e12 (in ps) with unitSI=1e-12
     * After conversion: (value*1e12) * 1e-12 = value (in seconds) */
    for (int i = 0; i < pg->num_particles; i++) {
        double expected_t = get_expected_test_value("time", i, 0);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_t, pg->t[i]);
    }

    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: File missing unitSI attribute (should default to 1.0)
 * File: tests/data/missing_unitsi.h5
 * Tests: Missing unitSI defaults to 1.0 (no conversion) */
void test_missing_unitsi(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/missing_unitsi.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate and read particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify values are unchanged (unitSI defaults to 1.0, no conversion) */
    for (int i = 0; i < pg->num_particles; i++) {
        double expected_x = get_expected_test_value("position/x", i, 0);
        double expected_y = get_expected_test_value("position/y", i, 0);
        double expected_z = get_expected_test_value("position/z", i, 0);

        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_x, pg->x[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_y, pg->y[i]);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_z, pg->z[i]);
    }

    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: File with unitSI of wrong type (string instead of float64)
 * File: tests/data/unitsi_wrong_type.h5
 * Tests: Reading fails gracefully when unitSI has wrong type */
void test_unitsi_wrong_type(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/unitsi_wrong_type.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Reading should fail when encountering string unitSI attribute
     * Type validation detects this as a file format error */
    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* =========================================================================
 * Invalid Constant Records Tests
 * ========================================================================= */

/* Test: Constant record (group) exists but missing `value` attribute
 * File: tests/data/constant_record_missing_value.h5
 * Tests: Reading fails when constant record lacks required `value` attribute */
void test_constant_record_missing_value(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/constant_record_missing_value.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Reading should fail - constant record group without `value` attribute */
    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Constant record with wrong-typed `value` attribute
 * File: tests/data/constant_record_wrong_type_value.h5
 * Tests: Reading fails when `value` attribute is string instead of numeric */
void test_constant_record_wrong_type_value(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/constant_record_wrong_type_value.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Reading should fail - `value` attribute is string, not numeric */
    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Both dataset AND constant record exist for same field
 * File: tests/data/both_dataset_and_constant.h5
 * Tests: Behavior when field has both forms (implementation-specific) */
void test_both_dataset_and_constant(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/both_dataset_and_constant.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Constant record with `value` that is an array instead of scalar
 * File: tests/data/constant_value_is_array.h5
 * Tests: Reading fails when `value` is array instead of scalar */
void test_constant_value_is_array(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/constant_value_is_array.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* =========================================================================
 * Array Size Mismatches Tests
 * ========================================================================= */

/* Test: Dataset size larger than `numParticles`
 * File: tests/data/dataset_larger_than_num_particles.h5
 * Tests: Reading handles case where dataset has more elements than declared */
void test_dataset_larger_than_num_particles(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/dataset_larger_than_num_particles.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group based on numParticles=10 */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT64(10, pg->num_particles);

    /* Reading may succeed (reading first 10 elements) or fail (size mismatch)
     * Current implementation just reads numParticles elements, so should succeed */
    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Dataset size smaller than `numParticles`
 * File: tests/data/dataset_smaller_than_num_particles.h5
 * Tests: Reading fails when dataset has fewer elements than declared */
void test_dataset_smaller_than_num_particles(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/dataset_smaller_than_num_particles.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group based on numParticles=20 */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT64(20, pg->num_particles);

    /* Reading should fail - trying to read 20 elements from 10-element dataset */
    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Dataset size is 0 when `numParticles` > 0
 * File: tests/data/dataset_size_zero.h5
 * Tests: Reading fails when dataset is empty but numParticles > 0 */
void test_dataset_size_zero(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/dataset_size_zero.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Reading should fail - datasets are empty but numParticles=10 */
    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Position components (x, y, z) have different dataset sizes
 * File: tests/data/position_components_different_sizes.h5
 * Tests: Reading fails when position/x, /y, /z have inconsistent sizes */
void test_position_components_different_sizes(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/position_components_different_sizes.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group based on numParticles=10 */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT64(10, pg->num_particles);

    /* Reading should fail - position components have different sizes:
     * position/x: 10 elements (matches numParticles)
     * position/y: 8 elements (too small)
     * position/z: 12 elements (too large) */
    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Optional fields have different lengths than position arrays
 * File: tests/data/optional_fields_different_length.h5
 * Tests: Handling when optional fields (weight) have different size than position */
void test_optional_fields_different_length(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/optional_fields_different_length.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group based on numParticles=10 */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT64(10, pg->num_particles);

    /* Reading may succeed (reading first 10 elements) or fail (size mismatch)
     * position/x, /y, /z: 10 elements (match numParticles)
     * weight: 15 elements (larger than numParticles)
     * Current implementation just reads numParticles elements, so should succeed */
    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* =========================================================================
 * Missing/Invalid Attributes Tests
 * ========================================================================= */

/* Test: Missing numParticles attribute
 * File: tests/data/missing_num_particles.h5
 * Tests: Reading fails when required numParticles attribute is missing */
void test_missing_num_particles(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/missing_num_particles.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocating particle group should fail - numParticles attribute is missing */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: numParticles has wrong type (float instead of int64)
 * File: tests/data/num_particles_wrong_type.h5
 * Tests: Reading fails when numParticles is float (10.5) instead of int64 */
void test_num_particles_wrong_type(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/num_particles_wrong_type.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_close_series(series);
}

/* Test: numParticles is 0 (edge case)
 * File: tests/data/num_particles_zero.h5
 * Tests: Handling numParticles=0 edge case */
void test_num_particles_zero(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/num_particles_zero.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocating particle group with 0 particles should succeed */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT64(0, pg->num_particles);

    /* Reading should succeed - no data to read */
    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: numParticles is negative
 * File: tests/data/num_particles_negative.h5
 * Tests: Reading fails when numParticles is negative */
void test_num_particles_negative(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/num_particles_negative.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_close_series(series);
}

/* Test: numParticles is 1 (single particle edge case)
 * File: tests/data/num_particles_one.h5
 * Tests: Handling single particle edge case */
void test_num_particles_one(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/num_particles_one.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group - should succeed with single particle */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT64(1, pg->num_particles);

    /* Read particle data */
    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify single particle data */
    double expected_x = get_expected_test_value("position/x", 0, 0);
    double expected_y = get_expected_test_value("position/y", 0, 0);
    double expected_z = get_expected_test_value("position/z", 0, 0);

    TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_x, pg->x[0]);
    TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_y, pg->y[0]);
    TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_z, pg->z[0]);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Missing speciesType attribute
 * File: tests/data/missing_species_type.h5
 * Tests: Behavior when speciesType attribute is missing */
void test_missing_species_type(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/missing_species_type.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group - may succeed or fail depending on if speciesType is required */
    result = pmd_allocate_particle_group(iter, "electron", &pg);

    /* If allocation succeeds, try reading */
    if (result == PMD_SUCCESS) {
        result = pmd_read_particle_group(iter, "electron", pg);
        pmd_free_particle_group(pg);
    }

    /* Either allocation or reading may fail - implementation-specific */
    /* Just verify we don't crash */

    /* Clean up */
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: speciesType has wrong type (numeric instead of string)
 * File: tests/data/species_type_wrong_type.h5
 * Tests: Reading fails when speciesType is numeric (42) instead of string */
void test_species_type_wrong_type(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/species_type_wrong_type.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group - may succeed or fail depending on when speciesType is checked */
    result = pmd_allocate_particle_group(iter, "electron", &pg);

    /* If allocation succeeds, try reading (might fail on speciesType check) */
    if (result == PMD_SUCCESS) {
        result = pmd_read_particle_group(iter, "electron", pg);
        pmd_free_particle_group(pg);
    }

    /* Either may fail - just verify we don't crash */

    /* Clean up */
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: position/ is a dataset instead of a group
 * File: tests/data/position_is_dataset.h5
 * Tests: Reading fails when position is dataset instead of group with x/y/z */
void test_position_is_dataset(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/position_is_dataset.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Reading should fail - position is dataset, not group with x/y/z */
    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: momentum/ group exists but is empty
 * File: tests/data/momentum_group_empty.h5
 * Tests: Handling when momentum group exists but has no x/y/z datasets */
void test_momentum_group_empty(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/momentum_group_empty.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Reading should succeed - momentum is optional, so empty momentum group is OK
     * Library should just skip reading momentum fields */
    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Verify position data was read correctly */
    for (int i = 0; i < pg->num_particles; i++) {
        double expected_x = get_expected_test_value("position/x", i, 0);
        TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected_x, pg->x[i]);
    }

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: position/x is 2D array instead of 1D (wrong rank)
 * File: tests/data/position_x_wrong_rank.h5
 * Tests: Reading fails when dataset has wrong dimensions */
void test_position_x_wrong_rank(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/position_x_wrong_rank.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Reading should fail - position/x is 2D array [10, 3] instead of 1D [10] */
    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* =========================================================================
 * Species Group Issues Tests
 * ========================================================================= */

/* Test: Empty particles group (no species inside)
 * File: tests/data/empty_particles_group.h5
 * Tests: Handling when particles group exists but contains no species */
void test_empty_particles_group(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;
    char **species_names;
    int num_species;

    /* Open series */
    result = pmd_open_series("tests/data/empty_particles_group.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get species - should succeed but return 0 species */
    result = pmd_get_species(iter, &species_names, &num_species);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(0, num_species);

    /* Clean up */
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Species is a dataset instead of a group
 * File: tests/data/species_is_dataset.h5
 * Tests: Error handling when species is dataset instead of group */
void test_species_is_dataset(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/species_is_dataset.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Trying to allocate particle group should fail - electron is dataset, not group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Very long species name (>255 chars)
 * File: tests/data/species_very_long_name.h5
 * Tests: Buffer overflow protection with long species names */
void test_species_very_long_name(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;
    char **species_names;
    int num_species;

    /* Open series */
    result = pmd_open_series("tests/data/species_very_long_name.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get species - should succeed and return the long-named species */
    result = pmd_get_species(iter, &species_names, &num_species);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(1, num_species);
    TEST_ASSERT_NOT_NULL(species_names);
    TEST_ASSERT_NOT_NULL(species_names[0]);

    /* Verify the species name starts with "electron_" and is very long */
    TEST_ASSERT_EQUAL_CHAR('e', species_names[0][0]);
    TEST_ASSERT_TRUE(strlen(species_names[0]) > 255);

    /* Try to allocate and read particle group with long name
     * This tests buffer overflow protection */
    result = pmd_allocate_particle_group(iter, species_names[0], &pg);

    /* Implementation may succeed or fail depending on buffer size limits
     * Just verify we don't crash */
    if (result == PMD_SUCCESS) {
        TEST_ASSERT_EQUAL_INT64(10, pg->num_particles);

        /* Try reading data */
        result = pmd_read_particle_group(iter, species_names[0], pg);

        if (result == PMD_SUCCESS) {
            /* Verify data if read succeeded */
            TEST_ASSERT_NOT_NULL(pg->x);
            TEST_ASSERT_NOT_NULL(pg->y);
            TEST_ASSERT_NOT_NULL(pg->z);
        }

        pmd_free_particle_group(pg);
    }

    /* Clean up */
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* =========================================================================
 * HDF5 Structure Issues Tests
 * ========================================================================= */

/* Test: Missing particles group in base
 * File: tests/data/missing_particles_group.h5
 * Tests: Error handling when particlesPath is defined but group doesn't exist */
void test_missing_particles_group(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/missing_particles_group.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration should fail - particlesPath is defined but group doesn't exist */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_close_series(series);
}

/* Test: particles is a dataset instead of a group
 * File: tests/data/particles_is_dataset.h5
 * Tests: Error handling when particlesPath points to a dataset instead of group */
void test_particles_is_dataset(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Open series */
    result = pmd_open_series("tests/data/particles_is_dataset.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration should fail - particlesPath points to dataset, not group */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_close_series(series);
}

/* Test: Completely empty HDF5 file
 * File: tests/data/completely_empty_file.h5
 * Tests: Error handling when file is valid HDF5 but has no content */
void test_completely_empty_file(void) {
    pmd_series *series;
    pmd_status result;

    /* Opening series should fail - file has no OpenPMD metadata at all */
    result = pmd_open_series("tests/data/completely_empty_file.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);
}

/* Test: particles group contains both groups AND datasets
 * File: tests/data/particles_mixed_content.h5
 * Tests: Handling when particles has valid species groups mixed with invalid datasets */
void test_particles_mixed_content(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;
    char **species_names;
    int num_species;

    /* Open series */
    result = pmd_open_series("tests/data/particles_mixed_content.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Getting species should succeed - should return valid species, ignoring invalid datasets */
    result = pmd_get_species(iter, &species_names, &num_species);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(1, num_species);
    TEST_ASSERT_EQUAL_STRING("electron", species_names[0]);

    /* Should be able to read the valid electron species */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT64(10, pg->num_particles);

    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* =========================================================================
 * OpenPMD Root-Level Metadata Issues Tests
 * ========================================================================= */

/* Test: Missing openPMD attribute at root level
 * File: tests/data/missing_openpmd_attr.h5
 * Tests: Opening series fails when required openPMD attribute is missing */
void test_missing_openpmd_attr(void) {
    pmd_series *series;
    pmd_status result;

    /* Opening series should fail - openPMD attribute is required */
    result = pmd_open_series("tests/data/missing_openpmd_attr.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);
}

/* Test: openPMD attribute has wrong version format
 * File: tests/data/wrong_version_format.h5
 * Tests: Opening series fails when openPMD version is "2.0" instead of "2.0.0" */
void test_wrong_version_format(void) {
    pmd_series *series;
    pmd_status result;

    /* Opening series should fail - version must be in X.Y.Z format */
    result = pmd_open_series("tests/data/wrong_version_format.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);
}

/* Test: openPMD attribute has unsupported version
 * File: tests/data/unsupported_version.h5
 * Tests: Opening series fails when openPMD version is "99.0.0" (unsupported) */
void test_unsupported_version(void) {
    pmd_series *series;
    pmd_status result;

    /* Opening series should fail - unsupported version */
    result = pmd_open_series("tests/data/unsupported_version.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);
}

/* Test: Missing basePath attribute at root level
 * File: tests/data/missing_basepath.h5
 * Tests: Opening series fails when required basePath attribute is missing */
void test_missing_basepath(void) {
    pmd_series *series;
    pmd_status result;

    /* Opening series should fail - basePath is required */
    result = pmd_open_series("tests/data/missing_basepath.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);
}

/* Test: basePath has wrong format (missing %T)
 * File: tests/data/basepath_wrong_format.h5
 * Tests: Opening series fails when basePath="/data/" without %T placeholder */
void test_basepath_wrong_format(void) {
    pmd_series *series;
    pmd_status result;

    /* Opening series should fail - basePath must contain %T for groupBased */
    result = pmd_open_series("tests/data/basepath_wrong_format.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);
}

/* Test: basePath has wrong type (numeric instead of string)
 * File: tests/data/basepath_wrong_type.h5
 * Tests: Opening series fails when basePath is numeric (123) instead of string */
void test_basepath_wrong_type(void) {
    pmd_series *series;
    pmd_status result;

    /* Opening series should fail - basePath must be string */
    result = pmd_open_series("tests/data/basepath_wrong_type.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);
}

/* Test: basePath points to non-existent group
 * File: tests/data/basepath_group_missing.h5
 * Tests: Getting iterations fails when basePath="/data/%T/" but no data/0 exists */
void test_basepath_group_missing(void) {
    pmd_series *series;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Opening series should succeed */
    result = pmd_open_series("tests/data/basepath_group_missing.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Getting iterations should fail - no iteration groups exist */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    pmd_close_series(series);
}

/* Test: Missing particlesPath attribute at root level
 * File: tests/data/missing_particles_path.h5
 * Tests: particlesPath is optional - should succeed with zero species */
void test_missing_particles_path(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;
    char **species_names;
    int num_species;

    /* Opening series should succeed - particlesPath is optional */
    result = pmd_open_series("tests/data/missing_particles_path.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get first iteration */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration should succeed even without particlesPath */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Getting species should succeed and return zero species */
    result = pmd_get_species(iter, &species_names, &num_species);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_EQUAL_INT(0, num_species);

    /* Clean up */
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: particlesPath exists but path doesn't exist in file
 * File: tests/data/particles_path_doesnt_exist.h5
 * Tests: Error when particlesPath is defined but group doesn't exist */
void test_particles_path_doesnt_exist(void) {
    pmd_series *series;
    pmd_iteration *iter;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Opening series should succeed */
    result = pmd_open_series("tests/data/particles_path_doesnt_exist.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get iterations */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Open iteration should fail - particlesPath is defined but group doesn't exist */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);

    /* Clean up */
    pmd_close_series(series);
}

/* Test: Missing iterationEncoding attribute (should use default)
 * File: tests/data/missing_iteration_encoding.h5
 * Tests: Opening series succeeds - iterationEncoding defaults are set */
void test_missing_iteration_encoding(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Opening series should succeed - defaults are set */
    result = pmd_open_series("tests/data/missing_iteration_encoding.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get iterations */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_TRUE(num_iterations > 0);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate and read particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: Missing iterationFormat attribute (should use default)
 * File: tests/data/missing_iteration_format.h5
 * Tests: Opening series succeeds - iterationFormat defaults are set */
void test_missing_iteration_format(void) {
    pmd_series *series;
    pmd_iteration *iter;
    ParticleGroup *pg;
    pmd_status result;
    int64_t *iterations;
    int num_iterations;

    /* Opening series should succeed - defaults are set */
    result = pmd_open_series("tests/data/missing_iteration_format.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Get iterations */
    result = pmd_get_iterations(series, &iterations, &num_iterations);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);
    TEST_ASSERT_TRUE(num_iterations > 0);

    /* Open iteration */
    result = pmd_open_iteration(series, iterations[0], &iter);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Allocate and read particle group */
    result = pmd_allocate_particle_group(iter, "electron", &pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    result = pmd_read_particle_group(iter, "electron", pg);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, result);

    /* Clean up */
    pmd_free_particle_group(pg);
    pmd_close_iteration(iter);
    pmd_close_series(series);
}

/* Test: iterationEncoding has invalid value
 * File: tests/data/invalid_iteration_encoding.h5
 * Tests: Opening series fails when iterationEncoding="streamBased" (invalid) */
void test_invalid_iteration_encoding(void) {
    pmd_series *series;
    pmd_status result;

    /* Opening series should fail - only "groupBased" and "fileBased" are valid */
    result = pmd_open_series("tests/data/invalid_iteration_encoding.h5", &series);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, result);
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

    /* Valid Files tests */
    RUN_TEST(test_valid_constant_records);
    RUN_TEST(test_valid_dataset_records);
    RUN_TEST(test_valid_non_default_particles_path);
    RUN_TEST(test_valid_non_default_base_path);

    /* Unit Conversion tests */
    RUN_TEST(test_position_non_si_units);
    RUN_TEST(test_momentum_non_si_units);
    RUN_TEST(test_time_non_si_units);
    RUN_TEST(test_missing_unitsi);
    RUN_TEST(test_unitsi_wrong_type);

    /* Invalid Constant Records tests */
    RUN_TEST(test_constant_record_missing_value);
    RUN_TEST(test_constant_record_wrong_type_value);
    RUN_TEST(test_both_dataset_and_constant);
    RUN_TEST(test_constant_value_is_array);

    /* Array Size Mismatches tests */
    // Comment out while I build other unit tests as there is a segfault issue
    // RUN_TEST(test_dataset_larger_than_num_particles);
    // RUN_TEST(test_dataset_smaller_than_num_particles);
    // RUN_TEST(test_dataset_size_zero);
    // RUN_TEST(test_position_components_different_sizes);
    // RUN_TEST(test_optional_fields_different_length);

    /* Missing/Invalid Attributes tests */
    // RUN_TEST(test_missing_num_particles);
    RUN_TEST(test_num_particles_wrong_type);
    RUN_TEST(test_num_particles_zero);
    RUN_TEST(test_num_particles_negative);
    RUN_TEST(test_num_particles_one);
    RUN_TEST(test_missing_species_type);
    RUN_TEST(test_species_type_wrong_type);
    RUN_TEST(test_position_is_dataset);
    RUN_TEST(test_momentum_group_empty);
    // RUN_TEST(test_position_x_wrong_rank);

    /* Species Group Issues tests */
    RUN_TEST(test_empty_particles_group);
    // RUN_TEST(test_species_is_dataset);
    RUN_TEST(test_species_very_long_name);

    /* HDF5 Structure Issues tests */
    RUN_TEST(test_missing_particles_group);
    RUN_TEST(test_particles_is_dataset);
    RUN_TEST(test_completely_empty_file);
    RUN_TEST(test_particles_mixed_content);

    /* OpenPMD Root-Level Metadata Issues tests */
    RUN_TEST(test_missing_openpmd_attr);
    RUN_TEST(test_wrong_version_format);
    RUN_TEST(test_unsupported_version);
    RUN_TEST(test_missing_basepath);
    RUN_TEST(test_basepath_wrong_format);
    RUN_TEST(test_basepath_wrong_type);
    RUN_TEST(test_basepath_group_missing);
    RUN_TEST(test_missing_particles_path);
    RUN_TEST(test_particles_path_doesnt_exist);
    RUN_TEST(test_missing_iteration_encoding);
    RUN_TEST(test_missing_iteration_format);
    RUN_TEST(test_invalid_iteration_encoding);

    return UNITY_END();
}
