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

/* Main test runner */
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_matches_pattern);
    return UNITY_END();
}
