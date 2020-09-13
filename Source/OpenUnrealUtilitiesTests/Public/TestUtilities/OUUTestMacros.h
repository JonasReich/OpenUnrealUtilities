// Copyright (c) 2020 Jonas Reich

#pragma once

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_WORKER

//////////////////////////////////////////////////////////////////////////
// This header file contains utility macros that should aid in the creation
// of automated tests. They work in conjunction with the macros defined in
// AutomationTest.h and are not supposed to be a complete replacement,
// but rather an augmentation of the latter.
//
// All of the macros in this file must contain the initials OUU to avoid
// name clashes and make them easily distinguishable!
//////////////////////////////////////////////////////////////////////////

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
#define OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestCase, TestFlags) \
OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST_INNER(OUU_TEST_CLASS(TestCase), OUU_TESTCASE_PRETTY_NAME(TestCase), TestFlags)

/**
 * Use this macro in conjunction with a predefined OUU_TEST_CATEGORY and OUU_TEST_TYPE to declare a complex automation test.
 * In contrast to simple tests, complex automation tests are called on multiple test-cases based on strings.
 * This utility allows declaring those test-cases in a simple list instead of manually implementing the GetTests() function of the test class.
 * Used like this:
 * -------------------------
 * OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN(PrintString, DEFAULT_OUU_TEST_FLAGS)
 * OUU_COMPLEX_AUTOMATION_TESTCASE("Hello")
 * OUU_COMPLEX_AUTOMATION_TESTCASE("World")
 * OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_END(PrintString)
 * {
 *     // test case implementation
 *     // use parameter (const FString& Parameters) to get one of
 *     // the test-case strings "Hello" and "World"
 * }
 * -------------------------
 */
#define OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN(TestCase, TestFlags) \
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN_INNER(OUU_TEST_CLASS(TestCase), OUU_TESTCASE_PRETTY_NAME(TestCase), TestFlags)

/**
 * Allows defining a test-case without having to explicitly name it.
 * Use this inline after OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN()
 */
#define OUU_COMPLEX_AUTOMATION_TESTCASE(TestCaseString) \
OUU_COMPLEX_AUTOMATION_TESTCASE_INNER(TestCaseString)

/**
 * Allows defining a test-case together with a display name to go along with it.
 * Use this inline after OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN()
 */
#define OUU_COMPLEX_AUTOMATION_TESTCASE_NAMED(DisplayName, TestCaseString) \
OutTestCommands.Add(TestCaseString); \
OutBeautifiedNames.Add(DisplayName);

/**
 * End the header of a complex automation test definition.
 * After this comes the test function body in curly brackets.
 */
#define OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_END(TestCase) \
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_END_INNER(OUU_TEST_CLASS(TestCase))

// ---------------------------------

// shared macros
#define OUU_CONCAT_TEST_CLASS_NAME(a, b, c, d) a ## b ## c ## d
#define OUU_EXPAND_TEST_CLASS_NAME(TestType, TestCase) OUU_CONCAT_TEST_CLASS_NAME(F, TestType, Tests_, TestCase)
#define OUU_TEST_CLASS(TestCase) OUU_EXPAND_TEST_CLASS_NAME(OUU_TEST_TYPE, TestCase)

#define OUU_STRINGIFY_TESTCASE_PRETTY_NAME(PrettyName) PREPROCESSOR_TO_STRING(PrettyName)
#define OUU_EXPAND_TESTCASE_PRETTY_NAME(Category, TestType, TestCase) OUU_STRINGIFY_TESTCASE_PRETTY_NAME(Category ##.## TestType ##.## TestCase)
#define OUU_TESTCASE_PRETTY_NAME(TestCase) OUU_EXPAND_TESTCASE_PRETTY_NAME(OUU_TEST_CATEGORY, OUU_TEST_TYPE, TestCase)

// simple automation test
#define OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST_INNER(TestCaseClass, PrettyName, TestFlags) \
IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestCaseClass, PrettyName, TestFlags) \
bool TestCaseClass::RunTest(const FString& Parameters)

// complex automation tests
#define OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN_INNER(TestCaseClass, PrettyName, TestFlags) \
IMPLEMENT_COMPLEX_AUTOMATION_TEST(TestCaseClass, PrettyName, TestFlags) \
void TestCaseClass::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const \
{

#define OUU_COMPLEX_AUTOMATION_TESTCASE_INNER(TestCaseString) \
{ \
	OutTestCommands.Add(TestCaseString); \
	FString CopiedTestCaseStringVar = TestCaseString; \
	/* remove '.' from display strings so we don't accidentally add more sub-folders */ \
	CopiedTestCaseStringVar.ReplaceInline(TEXT("."), TEXT("")); \
	OutBeautifiedNames.Add(CopiedTestCaseStringVar); \
}

#define OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_END_INNER(TestCaseClass) \
} \
bool TestCaseClass::RunTest(const FString& Parameters)

// ---------------------------------

/**
 * Shorthands for calling test functions for use in specs.
 * A detailed plain-text description should already be provided in the surrounding It(),
 * so it's often even really hard to even provide a good description other than copying the object/function call
 * that is tested, which is exactly what this macro does.
 * Doesn't have any early exit conditions (like UTEST macros) because those obfuscate the test flow without adding
 * any significant advantages.
 */ 
#define SPEC_TEST_TRUE(Actual) TestTrue(PREPROCESSOR_TO_STRING(Actual), Actual)
#define SPEC_TEST_FALSE(Actual) TestFalse(PREPROCESSOR_TO_STRING(Actual), Actual)
#define SPEC_TEST_EQUAL(Actual, Expected) TestEqual(WHAT_SPEC_TEST_COMPARE(Actual, Expected), Actual, Expected)
#define SPEC_TEST_NOT_EQUAL(Actual, Expected) TestNotEqual(WHAT_SPEC_TEST_COMPARE(Actual, Expected), Actual, Expected)
#define SPEC_TEST_EQUAL_TOLERANCE(Actual, Expected, Tolerance) TestEqual(WHAT_SPEC_TEST_COMPARE(Actual, Expected) " (tolerance: " PREPROCESSOR_TO_STRING(Tolerance) ")", Actual, Expected, Tolerance)
#define SPEC_TEST_EQUAL_INSENSITIVE(Actual, Expected) TestEqualInsensitive(WHAT_SPEC_TEST_COMPARE(Actual, Expected), Actual, Expected)
#define SPEC_TEST_SAME(Actual, Expected) TestSame(WHAT_SPEC_TEST_COMPARE(Actual, Expected), Actual, Expected)
#define SPEC_TEST_NOT_SAME(Actual, Expected) TestNotSame(WHAT_SPEC_TEST_COMPARE(Actual, Expected), Actual, Expected)
#define SPEC_TEST_VALID(Actual) TestValid(PREPROCESSOR_TO_STRING(Actual), Actual)
#define SPEC_TEST_INVALID(Actual) TestInvalid(PREPROCESSOR_TO_STRING(Actual), Actual)
#define SPEC_TEST_NULL(Actual) TestNull(PREPROCESSOR_TO_STRING(Actual), Actual)
#define SPEC_TEST_NOT_NULL(Actual) TestNotNull(PREPROCESSOR_TO_STRING(Actual), Actual)
#define SPEC_TEST_ARRAYS_EQUAL(Actual, Expected) TestArraysEqual(*this, WHAT_SPEC_TEST_COMPARE(Actual, Expected), Actual, Expected)
#define SPEC_TEST_ARRAYS_MATCH_UNORDERED(Actual, Expected) TestUnorderedArraysMatch(*this, WHAT_SPEC_TEST_COMPARE(Actual, Expected), Actual, Expected)

// shared inner macros
#define WHAT_SPEC_TEST_COMPARE(Actual, Expected) PREPROCESSOR_TO_STRING(Actual vs Expected)

// ---------------------------------

#endif
