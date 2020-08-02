// Copyright (c) 2020 Jonas Reich

#pragma once

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_WORKER

#include "Templates/Models.h"
#include "Templates/IsEnumClass.h"

namespace OUUTests_Internal
{
/** Concept for a class that supports LexToString() */
struct CLexToStringConvertable
{
	template<typename ElementType>
	auto Requires(ElementType It) -> decltype(LexToString(DeclVal<ElementType>()));
};

/** Overload for adding an equality error of array values that DO support LexToString() */
template< typename ElementType, typename =
	typename TEnableIf<TModels<CLexToStringConvertable, ElementType>::Value &! TIsPointer<ElementType>::Value>::Type >
void AddArrayValueError(
		FAutomationTestBase& AutomationTest,
		const FString& What,
		int32 Idx,
		const ElementType& AValue,
		const ElementType& BValue)
{
	AutomationTest.AddError(
		FString::Printf(
			TEXT("%s: The two arrays have different values at index %i (expected %s, but it was %s)."),
			*What, Idx, *LexToString(AValue), *LexToString(BValue)), 1);
}

/** Overload for adding an equality error of array values that do NOT support LexToString() */
template< typename ElementType, typename =
	typename TEnableIf<TModels<CLexToStringConvertable, ElementType>::Value == false || TIsPointer<ElementType>::Value>::Type >
void AddArrayValueError(
		FAutomationTestBase& AutomationTest,
		const FString& What,
		int32 Idx,
		const ElementType& AValue,
		const ElementType& BValue,
		int32 iOverloadArg = 0)
{
	AutomationTest.AddError(
		FString::Printf(
			TEXT("%s: The two arrays have different values at index %i."),
			*What, Idx), 1);
}

template<typename T, typename = typename TEnableIf<TIsEnumClass<T>::Value == false>::Type>
T ParseValue(const FString& s)
{
	T Result;
	LexTryParseString(Result, *s);
	return Result;
}

template<typename T, typename = typename TEnableIf<TIsEnumClass<T>::Value == true>::Type>
T ParseValue(const FString& s, int32 iOverloadArg = 0)
{
	static_assert(sizeof(int32) >= sizeof(T), "Cannot parse value because enum class T underlying type is bigger than int32");
	int32 Result;
	LexTryParseString(Result, *s);
	return static_cast<T>(Result);
}

template<>
FORCEINLINE FVector ParseValue<FVector>(const FString& s)
{
	FVector Result;
	Result.InitFromString(s);
	return Result;
}

}

/**
 * Test if two arrays are equal.
 * This check doesn't have any functional difference to an arrays equality check via operator==(),
 * but this function has more verbose output because it compares individual array elements.
 */
template<typename ElementType, typename AllocatorType>
void TestArraysEqual(
	FAutomationTestBase& AutomationTest,
	const FString& What,
	const TArray<ElementType, AllocatorType>& ActualArray,
	const TArray<ElementType, AllocatorType>& ExpectedArray)
{
	// Quick initial test: Compare element counts
	int32 ActualNum = ActualArray.Num();
	int32 ExpectedNum = ExpectedArray.Num();
	if (ActualNum != ExpectedNum)
	{
		AutomationTest.AddError(
			FString::Printf(TEXT("%s: The two arrays have different length (expected %i, but it was %i)."),
				*What, ActualNum, ExpectedNum), 1);
		return;
	}

	// More thorough test: Compare individual elements for equal values at the same index
	for (int32 i = 0; i < ActualNum; i++)
	{
		if (ActualArray[i] != ExpectedArray[i])
		{
			OUUTests_Internal::AddArrayValueError(AutomationTest, What, i, ActualArray[i], ExpectedArray[i]);
			return;
		}
	}

	ensureMsgf(ActualArray == ExpectedArray, TEXT("If the two arrays did not match, we should have gotten an error before."));
}

struct FTestParameterParser
{
	const FString ParameterDelimiter;
	const FString ArrayDelimiter;
	TArray<FString> Parameters;


	FTestParameterParser(FString ParametersString, FString InParameterDelimiter, FString InArrayDelimiter) :
		ParameterDelimiter(InParameterDelimiter),
		ArrayDelimiter(InArrayDelimiter)
	{
		ParametersString.ParseIntoArray(Parameters, *ParameterDelimiter);
	}

	FTestParameterParser(FString ParametersString) :
		FTestParameterParser(ParametersString, TEXT("|"), TEXT(";"))
	{
	}

	template<typename T>
	T GetValue(int32 Index) const
	{
		return OUUTests_Internal::ParseValue<T>(Parameters[Index]);
	}

	template<typename T>
	TArray<T> GetArrayValue(int32 Index) const
	{
		TArray<FString> ParameterArray;
		Parameters[i].ParseIntoArray(ParameterArray, *ArrayDelimiter);
		TArray<T> Result;
		for (const FString& s : ParameterArray)
		{
			Result.Add(OUUTests_Internal::ParseValue(s));
		}
		return Result;
	}
};

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
OUU_COMPLEX_AUTOMATION_TESTCASE_INNER(TestCaseString, OUU_UNIQUE_VAR_NAME(StringVar))

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

#define OUU_UNIQUE_VAR_NAME(VarName) PREPROCESSOR_JOIN(VarName, __LINE__)

// simple automation test
#define OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST_INNER(TestCaseClass, PrettyName, TestFlags) \
IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestCaseClass, PrettyName, TestFlags) \
bool TestCaseClass::RunTest(const FString& Parameters)

// complex automation tests
#define OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN_INNER(TestCaseClass, PrettyName, TestFlags) \
IMPLEMENT_COMPLEX_AUTOMATION_TEST(TestCaseClass, PrettyName, TestFlags) \
void TestCaseClass::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const \
{

#define OUU_COMPLEX_AUTOMATION_TESTCASE_INNER(TestCaseString, CopiedTestCaseStringVar) \
OutTestCommands.Add(TestCaseString); \
FString CopiedTestCaseStringVar = TestCaseString; \
/* remove '.' from display strings so we don't accidentally add more sub-folders */ \
CopiedTestCaseStringVar.ReplaceInline(TEXT("."), TEXT("")); \
OutBeautifiedNames.Add(CopiedTestCaseStringVar);

#define OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_END_INNER(TestCaseClass) \
} \
bool TestCaseClass::RunTest(const FString& Parameters)

#endif
