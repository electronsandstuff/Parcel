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
 * matches_pattern() tests
 * ========================================================================= */

void test_matches_pattern(void) {
    /* Basic matching */
    TEST_ASSERT_TRUE(matches_pattern("data_0.h5", "data_%T.h5"));
    TEST_ASSERT_TRUE(matches_pattern("data_1.h5", "data_%T.h5"));
    TEST_ASSERT_TRUE(matches_pattern("data_123.h5", "data_%T.h5"));
    TEST_ASSERT_TRUE(matches_pattern("data_999999.h5", "data_%T.h5"));

    /* No match cases */
    TEST_ASSERT_FALSE(matches_pattern("data_.h5", "data_%T.h5"));        /* Missing digits */
    TEST_ASSERT_FALSE(matches_pattern("data_a.h5", "data_%T.h5"));       /* Letter instead of digit */
    TEST_ASSERT_FALSE(matches_pattern("data_0.txt", "data_%T.h5"));      /* Wrong extension */
    TEST_ASSERT_FALSE(matches_pattern("other_0.h5", "data_%T.h5"));      /* Wrong prefix */
    TEST_ASSERT_FALSE(matches_pattern("data_0_extra.h5", "data_%T.h5")); /* Extra suffix */

    /* Pattern without %T should match exactly */
    TEST_ASSERT_TRUE(matches_pattern("exact.h5", "exact.h5"));
    TEST_ASSERT_FALSE(matches_pattern("exact_0.h5", "exact.h5"));
    TEST_ASSERT_FALSE(matches_pattern("other.h5", "exact.h5"));

    /* With prefix and suffix */
    TEST_ASSERT_TRUE(matches_pattern("sim_0_final.h5", "sim_%T_final.h5"));
    TEST_ASSERT_TRUE(matches_pattern("sim_123_final.h5", "sim_%T_final.h5"));
    TEST_ASSERT_FALSE(matches_pattern("sim_0.h5", "sim_%T_final.h5"));
    TEST_ASSERT_FALSE(matches_pattern("sim_final.h5", "sim_%T_final.h5"));

    /* Edge cases */
    TEST_ASSERT_TRUE(matches_pattern("0.h5", "%T.h5"));
    TEST_ASSERT_TRUE(matches_pattern("9.h5", "%T.h5"));
    TEST_ASSERT_TRUE(matches_pattern("00.h5", "%T.h5"));
    TEST_ASSERT_TRUE(matches_pattern("000123.h5", "%T.h5"));
    TEST_ASSERT_TRUE(matches_pattern("0", "%T"));
    TEST_ASSERT_TRUE(matches_pattern("123", "%T"));
    TEST_ASSERT_TRUE(matches_pattern("data_0", "data_%T"));
    TEST_ASSERT_TRUE(matches_pattern("data_999", "data_%T"));

    /* Empty strings */
    TEST_ASSERT_FALSE(matches_pattern("", "data_%T.h5"));
    TEST_ASSERT_FALSE(matches_pattern("data_0.h5", ""));
    TEST_ASSERT_TRUE(matches_pattern("", ""));

    /* Partial matches should fail */
    TEST_ASSERT_FALSE(matches_pattern("data_", "data_%T.h5"));
    TEST_ASSERT_FALSE(matches_pattern("data_0", "data_%T.h5"));
    TEST_ASSERT_FALSE(matches_pattern("data_0.", "data_%T.h5"));
    TEST_ASSERT_FALSE(matches_pattern("data_0.h", "data_%T.h5"));
    TEST_ASSERT_FALSE(matches_pattern("data_0.h5.bak", "data_%T.h5"));

    /* Special characters in filenames */
    TEST_ASSERT_TRUE(matches_pattern("my_data_file_0.h5", "my_data_file_%T.h5"));
    TEST_ASSERT_TRUE(matches_pattern("sim-0.h5", "sim-%T.h5"));
    TEST_ASSERT_TRUE(matches_pattern("v1.0_0.h5", "v1.0_%T.h5"));

    /* Multiple %T placeholders - all must match the same number */
    TEST_ASSERT_TRUE(matches_pattern("data_0_step_0.h5", "data_%T_step_%T.h5"));      /* Same number */
    TEST_ASSERT_TRUE(matches_pattern("data_123_step_123.h5", "data_%T_step_%T.h5"));  /* Same number */
    TEST_ASSERT_TRUE(matches_pattern("run_5_iter_5.txt", "run_%T_iter_%T.txt"));      /* Same number */
    TEST_ASSERT_FALSE(matches_pattern("data_0_step_1.h5", "data_%T_step_%T.h5"));     /* Different numbers */
    TEST_ASSERT_FALSE(matches_pattern("data_123_step_456.h5", "data_%T_step_%T.h5")); /* Different numbers */
    TEST_ASSERT_FALSE(matches_pattern("data_0_step_.h5", "data_%T_step_%T.h5"));      /* Missing second number */
    TEST_ASSERT_FALSE(matches_pattern("data__step_1.h5", "data_%T_step_%T.h5"));      /* Missing first number */
    TEST_ASSERT_FALSE(matches_pattern("data_a_step_1.h5", "data_%T_step_%T.h5"));     /* Non-digit for first %T */
    TEST_ASSERT_FALSE(matches_pattern("data_0_step_b.h5", "data_%T_step_%T.h5"));     /* Non-digit for second %T */

    /* Three or more %T placeholders - all must match */
    TEST_ASSERT_TRUE(matches_pattern("1_1_1.h5", "%T_%T_%T.h5"));
    TEST_ASSERT_TRUE(matches_pattern("100_100_100.h5", "%T_%T_%T.h5"));
    TEST_ASSERT_FALSE(matches_pattern("1_2_3.h5", "%T_%T_%T.h5"));     /* Different numbers */
    TEST_ASSERT_FALSE(matches_pattern("1_1_2.h5", "%T_%T_%T.h5"));     /* Third doesn't match */
    TEST_ASSERT_FALSE(matches_pattern("1_2_.h5", "%T_%T_%T.h5"));
}

/* =========================================================================
 * parse_iteration_pattern() tests
 * ========================================================================= */

void test_parse_iteration_pattern(void) {
    IterationPattern info;
    pmd_status status;

    /* Simple pattern with %T at end */
    status = parse_iteration_pattern("data_%T", &info);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_STRING("", info.scan_parent);
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
    TEST_ASSERT_EQUAL_STRING("", info.scan_parent);
    TEST_ASSERT_EQUAL_STRING("%T", info.first_segment);
    TEST_ASSERT_EQUAL_STRING("%T/data.h5", info.full_pattern);
    free_iteration_pattern(&info);

    /* Pattern with absolute path starting with %T */
    status = parse_iteration_pattern("/%T/data.h5", &info);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_STRING("", info.scan_parent);
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
    status = extract_iteration_from_name("data_0", "data_%T", &iteration);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(0, iteration);

    status = extract_iteration_from_name("data_123", "data_%T", &iteration);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(123, iteration);

    status = extract_iteration_from_name("data_999999", "data_%T", &iteration);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(999999, iteration);

    /* With file extension */
    status = extract_iteration_from_name("sim_42.h5", "sim_%T.h5", &iteration);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(42, iteration);

    /* Multiple %T in pattern - should extract the number */
    status = extract_iteration_from_name("data_5_step_5", "data_%T_step_%T", &iteration);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(5, iteration);

    /* Just %T pattern */
    status = extract_iteration_from_name("123", "%T", &iteration);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(123, iteration);

    /* Leading zeros */
    status = extract_iteration_from_name("data_00042", "data_%T", &iteration);
    TEST_ASSERT_EQUAL_INT(PMD_SUCCESS, status);
    TEST_ASSERT_EQUAL_INT64(42, iteration);

    /* Mismatch cases - should fail */
    status = extract_iteration_from_name("data_abc", "data_%T", &iteration);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, status);

    status = extract_iteration_from_name("wrong_5", "data_%T", &iteration);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, status);

    status = extract_iteration_from_name("data_5.txt", "data_%T.h5", &iteration);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, status);

    /* Multiple %T with different numbers - should fail */
    status = extract_iteration_from_name("data_5_step_6", "data_%T_step_%T", &iteration);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_FILE_FORMAT, status);

    /* NULL input */
    status = extract_iteration_from_name(NULL, "data_%T", &iteration);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_NULL_POINTER, status);

    status = extract_iteration_from_name("data_5", NULL, &iteration);
    TEST_ASSERT_EQUAL_INT(PMD_ERROR_NULL_POINTER, status);
}

/* =========================================================================
 * replace_iteration() tests
 * ========================================================================= */

void test_replace_iteration(void) {
    char *result;

    /* Single %T replacement */
    result = replace_iteration("data_%T.h5", 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("data_0.h5", result);
    free(result);

    result = replace_iteration("data_%T.h5", 123);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("data_123.h5", result);
    free(result);

    result = replace_iteration("iter_%T", 42);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("iter_42", result);
    free(result);

    /* Multiple %T replacements - all should get same number */
    result = replace_iteration("data_%T/step_%T/final.h5", 5);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("data_5/step_5/final.h5", result);
    free(result);

    result = replace_iteration("/path/%T/data_%T.h5", 100);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/path/100/data_100.h5", result);
    free(result);

    result = replace_iteration("sim_%T_iter_%T_final_%T.h5", 7);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("sim_7_iter_7_final_7.h5", result);
    free(result);

    /* No %T in template - should return copy of template */
    result = replace_iteration("no_iteration.h5", 999);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("no_iteration.h5", result);
    free(result);

    /* Just %T */
    result = replace_iteration("%T", 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("0", result);
    free(result);

    result = replace_iteration("%T", 12345);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("12345", result);
    free(result);

    /* Large iteration numbers */
    result = replace_iteration("data_%T.h5", 1000000);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("data_1000000.h5", result);
    free(result);

    /* NULL input */
    result = replace_iteration(NULL, 5);
    TEST_ASSERT_NULL(result);
}

/* Main test runner */
int main(void) {
    // Turn off logging for tests
    pmd_set_log_level(PMD_LOG_NONE);

    UNITY_BEGIN();
    RUN_TEST(test_matches_pattern);
    RUN_TEST(test_parse_iteration_pattern);
    RUN_TEST(test_extract_iteration_from_name);
    RUN_TEST(test_replace_iteration);
    return UNITY_END();
}
