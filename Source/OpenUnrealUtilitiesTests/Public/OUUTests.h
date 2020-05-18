// Copyright (c) 2020 Jonas Reich

#pragma once

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_WORKER

/**
 * Combination of automation test flags that can be used for most if not all project/project-plugin automation tests.
 * It's recommended only to use these as is OR to explicitly define all flags as opposed to combining this macro with other flags.
 */
#define DEFAULT_OUU_TEST_FLAGS EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter

/**
 * Use this macro in conjunction with a predefined OUU_TEST_CATEGORY and OUU_TEST_TYPE to declare a simple automation test.
 * Used like this:
 * -------------------------
 * #define OUU_TEST_CATEGORY OpenUnrealUtilities.Foo
 * #define OUU_TEST_TYPE Bar
 * OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(HelloWorld, DEFAULT_OUU_TEST_FLAGS)
 * {
 *     // test case implementation
 * }
 * #undef OUU_TEST_CATEGORY
 * #undef OUU_TEST_TYPE
 * -------------------------
 * ...which declares a test class FBarTests_HelloWorld in the "OpenUnrealUtilities.Foo" category.
 * For more thorough examples, refer to any of the tests declared in this module.
 */
#define OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestCase, TestFlags) OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST_INNER(OUU_TEST_CLASS(TestCase), OUU_TESTCASE_PRETTY_NAME(TestCase), TestFlags)

// ---------------------------------

#define OUU_CONCAT_TEST_CLASS_NAME(a, b, c, d) a ## b ## c ## d
#define OUU_EXPAND_TEST_CLASS_NAME(TestType, TestCase) OUU_CONCAT_TEST_CLASS_NAME(F, TestType, Tests_, TestCase)
#define OUU_TEST_CLASS(TestCase) OUU_EXPAND_TEST_CLASS_NAME(OUU_TEST_TYPE, TestCase)

#define OUU_STRINGIFY_TESTCASE_PRETTY_NAME(PrettyName) PREPROCESSOR_TO_STRING(PrettyName)
#define OUU_EXPAND_TESTCASE_PRETTY_NAME(Category, TestType, TestCase) OUU_STRINGIFY_TESTCASE_PRETTY_NAME(Category ##.## TestType ##.## TestCase)
#define OUU_TESTCASE_PRETTY_NAME(TestCase) OUU_EXPAND_TESTCASE_PRETTY_NAME(OUU_TEST_CATEGORY, OUU_TEST_TYPE, TestCase)

#define OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST_INNER(TestCaseClass, PrettyName, TestFlags) IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestCaseClass, PrettyName, TestFlags)\
bool TestCaseClass::RunTest(const FString& Parameters)

#endif
