/**
 * Test utility macros and helpers for OpenPMD test suite
 */

#ifndef TESTS_UTILS_H
#define TESTS_UTILS_H

#include <math.h>
#include <stdio.h>
#include "../deps/Unity-2.6.1/unity.h"

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
        int _err; \
        if (_diff > _tolerance) { \
            char _msg[256]; \
            _err = snprintf(_msg, sizeof(_msg), \
                            "Expected %.15g, was %.15g (diff=%.3e, tol=%.3e)", \
                             _expected, _actual, _diff, _tolerance); \
            if (_err < 0) { \
                UNITY_TEST_FAIL(__LINE__, "snprintf failed in error message"); \
            } \
            UNITY_TEST_FAIL(__LINE__, _msg); \
        } \
    } while(0)

/* Default tolerances for typical use cases */
#define TEST_ASSERT_DOUBLE_CLOSE_DEFAULT(expected, actual) \
    TEST_ASSERT_DOUBLE_CLOSE(expected, actual, 1e-7, 0)

#endif /* TESTS_UTILS_H */
