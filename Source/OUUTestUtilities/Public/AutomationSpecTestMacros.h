// Copyright (c) 2021 Jonas Reich

#pragma once

/**
 * Shorthands for calling test functions for use in specs.
 *
 * The macros do not introduce any hidden logic (early returns, etc), but they auto-fill the test function description which
 * is passed as "What" parameter. The aim when using these macros is to enforce that test descriptions should be
 * part of the expectations formulated using Describe and It, so the actual test calls must not need any additional descriptions.
 *
 * Whenever you run into instances where the description of the test case is not sufficient to explain the call of test functions,
 * it's a good sign that you should either rephrase the expectation description or break the test case down into separate smaller
 * test cases that are easy to describe.
 */
#define SPEC_TEST_TRUE(Actual)                                 TestTrue(PREPROCESSOR_TO_STRING(Actual), Actual)
#define SPEC_TEST_FALSE(Actual)                                TestFalse(PREPROCESSOR_TO_STRING(Actual), Actual)
#define SPEC_TEST_EQUAL(Actual, Expected)                      TestEqual(PREPROCESSOR_TO_STRING(Actual vs Expected), Actual, Expected)
#define SPEC_TEST_NOT_EQUAL(Actual, Expected)                  TestNotEqual(PREPROCESSOR_TO_STRING(Actual vs Expected), Actual, Expected)
#define SPEC_TEST_EQUAL_TOLERANCE(Actual, Expected, Tolerance) TestEqual(PREPROCESSOR_TO_STRING(Actual vs Expected), Actual, Expected, Tolerance)
#define SPEC_TEST_EQUAL_INSENSITIVE(Actual, Expected)          TestEqualInsensitive(PREPROCESSOR_TO_STRING(Actual vs Expected), Actual, Expected)
#define SPEC_TEST_SAME(Actual, Expected)                       TestSame(PREPROCESSOR_TO_STRING(Actual vs Expected), Actual, Expected)
#define SPEC_TEST_NOT_SAME(Actual, Expected)                   TestNotSame(PREPROCESSOR_TO_STRING(Actual vs Expected), Actual, Expected)
#define SPEC_TEST_VALID(Actual)                                TestValid(PREPROCESSOR_TO_STRING(Actual), Actual)
#define SPEC_TEST_INVALID(Actual)                              TestInvalid(PREPROCESSOR_TO_STRING(Actual), Actual)
#define SPEC_TEST_NULL(Actual)                                 TestNull(PREPROCESSOR_TO_STRING(Actual), Actual)
#define SPEC_TEST_NOT_NULL(Actual)                             TestNotNull(PREPROCESSOR_TO_STRING(Actual), Actual)
#define SPEC_TEST_ARRAYS_EQUAL(Actual, Expected)               TestArraysEqual(*this, PREPROCESSOR_TO_STRING(Actual vs Expected), Actual, Expected)
#define SPEC_TEST_ARRAYS_MATCH_UNORDERED(Actual, Expected)     TestUnorderedArraysMatch(*this, PREPROCESSOR_TO_STRING(Actual vs Expected), Actual, Expected)
