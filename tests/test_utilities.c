/* Test utilities functions */

#include "../deps/Unity-2.6.1/unity.h"

#define PARCEL_IMPLEMENTATION
#include "../parcel.h"

void setUp(void) {
    /* This is run before each test */
}

void tearDown(void) {
    /* This is run after each test */
}


/* =========================================================================
 * parse_iteration_pattern() tests
 * ========================================================================= */

void test_parse_iteration_pattern(void) {
    iteration_pattern info;
    pmd_status status;

    /* Simple pattern with %T at end */
    status = parse_iteration_pattern("data_%T", &info);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_STRING(".", info.scan_parent);
    TEST_ASSERT_EQUAL_STRING("data_%T", info.first_segment);
    TEST_ASSERT_EQUAL_STRING("data_%T", info.full_pattern);
    free_iteration_pattern(&info);

    /* Pattern with path prefix and %T in filename */
    status = parse_iteration_pattern("/data/results/sim_%T.h5", &info);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_STRING("/data/results", info.scan_parent);
    TEST_ASSERT_EQUAL_STRING("sim_%T.h5", info.first_segment);
    TEST_ASSERT_EQUAL_STRING("/data/results/sim_%T.h5", info.full_pattern);
    free_iteration_pattern(&info);

    /* Pattern with %T in directory name */
    status = parse_iteration_pattern("/data/iter_%T/particles.h5", &info);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_STRING("/data", info.scan_parent);
    TEST_ASSERT_EQUAL_STRING("iter_%T", info.first_segment);
    TEST_ASSERT_EQUAL_STRING("/data/iter_%T/particles.h5", info.full_pattern);
    free_iteration_pattern(&info);

    /* Complex pattern with multiple %T across directories */
    status = parse_iteration_pattern("/a/path/data_%T/step_%T/final.h5", &info);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_STRING("/a/path", info.scan_parent);
    TEST_ASSERT_EQUAL_STRING("data_%T", info.first_segment);
    TEST_ASSERT_EQUAL_STRING("/a/path/data_%T/step_%T/final.h5", info.full_pattern);
    free_iteration_pattern(&info);

    /* Pattern starting with %T (root level) */
    status = parse_iteration_pattern("%T/data.h5", &info);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_STRING(".", info.scan_parent);
    TEST_ASSERT_EQUAL_STRING("%T", info.first_segment);
    TEST_ASSERT_EQUAL_STRING("%T/data.h5", info.full_pattern);
    free_iteration_pattern(&info);

    /* Pattern with absolute path starting with %T */
    status = parse_iteration_pattern("/%T/data.h5", &info);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_STRING(".", info.scan_parent);
    TEST_ASSERT_EQUAL_STRING("%T", info.first_segment);
    TEST_ASSERT_EQUAL_STRING("/%T/data.h5", info.full_pattern);
    free_iteration_pattern(&info);

    /* Pattern with multiple %T in same segment */
    status = parse_iteration_pattern("/data/sim_%T_iter_%T.h5", &info);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_STRING("/data", info.scan_parent);
    TEST_ASSERT_EQUAL_STRING("sim_%T_iter_%T.h5", info.first_segment);
    TEST_ASSERT_EQUAL_STRING("/data/sim_%T_iter_%T.h5", info.full_pattern);
    free_iteration_pattern(&info);

    /* NULL input */
    status = parse_iteration_pattern(NULL, &info);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_NULL_POINTER, status);
}

/* =========================================================================
 * extract_iteration_from_name() tests
 * ========================================================================= */

void test_extract_iteration_from_name(void) {
    int64_t iteration;
    pmd_status status;

    /* Simple extraction */
    status = extract_iteration_from_name("data_0", "data_%T", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(0, iteration);

    status = extract_iteration_from_name("data_123", "data_%T", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(123, iteration);

    status = extract_iteration_from_name("data_999999", "data_%T", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(999999, iteration);

    /* With file extension */
    status = extract_iteration_from_name("sim_42.h5", "sim_%T.h5", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(42, iteration);

    /* Multiple %T in pattern - should extract the number */
    status = extract_iteration_from_name("data_5_step_5", "data_%T_step_%T", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(5, iteration);

    /* Just %T pattern */
    status = extract_iteration_from_name("123", "%T", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(123, iteration);

    /* Leading zeros */
    status = extract_iteration_from_name("data_00042", "data_%T", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(42, iteration);

    /* Mismatch cases - should fail */
    status = extract_iteration_from_name("data_abc", "data_%T", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR, status);

    status = extract_iteration_from_name("wrong_5", "data_%T", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR, status);

    status = extract_iteration_from_name("data_5.txt", "data_%T.h5", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR, status);

    /* Multiple %T with different numbers - should fail */
    status = extract_iteration_from_name("data_5_step_6", "data_%T_step_%T", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR, status);

    /* Consecutive %T patterns - ambiguous, should fail */
    status = extract_iteration_from_name("data_123", "data_%T%T", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR, status);

    status = extract_iteration_from_name("123", "%T%T", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR, status);

    status = extract_iteration_from_name("sim_456_end", "sim_%T%T_end", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR, status);

    /* NULL input */
    status = extract_iteration_from_name(NULL, "data_%T", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_NULL_POINTER, status);

    status = extract_iteration_from_name("data_5", NULL, &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_NULL_POINTER, status);
}

/* =========================================================================
 * extract_iteration_from_name() padding detection tests
 * ========================================================================= */

void test_extract_iteration_padding(void) {
    int64_t iteration;
    unsigned int padding;
    pmd_status status;

    /* Zero padded names report the full digit width */
    status = extract_iteration_from_name("00001", "%T", &iteration, &padding);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(1, iteration);
    TEST_ASSERT_EQUAL_UINT(5, padding);

    status = extract_iteration_from_name("data_0042.h5", "data_%T.h5", &iteration, &padding);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(42, iteration);
    TEST_ASSERT_EQUAL_UINT(4, padding);

    /* All zeros is still padding when wider than the bare value */
    status = extract_iteration_from_name("0000", "%T", &iteration, &padding);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(0, iteration);
    TEST_ASSERT_EQUAL_UINT(4, padding);

    /* Unpadded names report no padding, including multi digit ones */
    status = extract_iteration_from_name("1", "%T", &iteration, &padding);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_UINT(0, padding);

    status = extract_iteration_from_name("10", "%T", &iteration, &padding);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(10, iteration);
    TEST_ASSERT_EQUAL_UINT(0, padding);

    status = extract_iteration_from_name("0", "%T", &iteration, &padding);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(0, iteration);
    TEST_ASSERT_EQUAL_UINT(0, padding);

    /* Padding is reset on every call, so a stale value cannot leak through */
    padding = 7;
    status = extract_iteration_from_name("123", "%T", &iteration, &padding);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_UINT(0, padding);

    /* Padding out is optional */
    status = extract_iteration_from_name("00001", "%T", &iteration, NULL);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(1, iteration);
}

/* =========================================================================
 * replace_iteration() tests
 * ========================================================================= */

void test_replace_iteration(void) {
    char *result;

    /* Single %T replacement */
    result = replace_iteration("data_%T.h5", 0, 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("data_0.h5", result);
    free(result);

    result = replace_iteration("data_%T.h5", 123, 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("data_123.h5", result);
    free(result);

    result = replace_iteration("iter_%T", 42, 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("iter_42", result);
    free(result);

    /* Multiple %T replacements - all should get same number */
    result = replace_iteration("data_%T/step_%T/final.h5", 5, 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("data_5/step_5/final.h5", result);
    free(result);

    result = replace_iteration("/path/%T/data_%T.h5", 100, 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/path/100/data_100.h5", result);
    free(result);

    result = replace_iteration("sim_%T_iter_%T_final_%T.h5", 7, 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("sim_7_iter_7_final_7.h5", result);
    free(result);

    /* No %T in template - should return copy of template */
    result = replace_iteration("no_iteration.h5", 999, 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("no_iteration.h5", result);
    free(result);

    /* Just %T */
    result = replace_iteration("%T", 0, 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("0", result);
    free(result);

    result = replace_iteration("%T", 12345, 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("12345", result);
    free(result);

    /* Large iteration numbers */
    result = replace_iteration("data_%T.h5", 1000000, 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("data_1000000.h5", result);
    free(result);

    /* NULL input */
    result = replace_iteration(NULL, 5, 0);
    TEST_ASSERT_NULL(result);
}

/* =========================================================================
 * replace_iteration() padding tests
 * ========================================================================= */

void test_replace_iteration_padding(void) {
    char *result;

    /* Zero padded to the requested width */
    result = replace_iteration("/data/%T/", 1, 5);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/data/00001/", result);
    free(result);

    result = replace_iteration("data_%T.h5", 42, 4);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("data_0042.h5", result);
    free(result);

    result = replace_iteration("%T", 0, 3);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("000", result);
    free(result);

    /* Every %T gets the same padded width */
    result = replace_iteration("data_%T/step_%T.h5", 7, 3);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("data_007/step_007.h5", result);
    free(result);

    /* Padding narrower than the number does not truncate it */
    result = replace_iteration("data_%T.h5", 123456, 3);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("data_123456.h5", result);
    free(result);

    /* Padding equal to the number width changes nothing */
    result = replace_iteration("data_%T.h5", 123, 3);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("data_123.h5", result);
    free(result);

    /* Padding of 0 is the unpadded behaviour */
    result = replace_iteration("data_%T.h5", 5, 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("data_5.h5", result);
    free(result);

    /* Padding wider than the internal buffer is rejected */
    result = replace_iteration("data_%T.h5", 5, 64);
    TEST_ASSERT_NULL(result);

    result = replace_iteration("data_%T.h5", 5, 1000);
    TEST_ASSERT_NULL(result);
}

/* Main test runner */
int main(void) {
    // Turn off logging for tests
    pmd_set_log_level(PMD_LOG_NONE);

    UNITY_BEGIN();
    RUN_TEST(test_parse_iteration_pattern);
    RUN_TEST(test_extract_iteration_from_name);
    RUN_TEST(test_extract_iteration_padding);
    RUN_TEST(test_replace_iteration);
    RUN_TEST(test_replace_iteration_padding);

    /* Clean up HDF5 library internal resources */
    H5close();

    return UNITY_END();
}
