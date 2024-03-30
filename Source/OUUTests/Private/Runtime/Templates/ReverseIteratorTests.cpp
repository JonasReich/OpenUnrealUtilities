// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "Engine/EngineTypes.h"
	#include "Templates/ReverseIterator.h"
	#include "Templates/StringUtils.h"

	#define OUU_TEST_CATEGORY OpenUnrealUtilities.Runtime.Templates
	#define OUU_TEST_TYPE	  ReverseIterator

namespace OUU::Tests::ReverseIteratorTests
{
	struct FFoo
	{
		int32 Num = 0;
		FString DisplayString = "Zero";

		FFoo() = default;
		FFoo(int32 i)
		{
			Num = i;
			switch (i)
			{
			case 1: DisplayString = "One"; break;
			case 2: DisplayString = "Two"; break;
			case 3: DisplayString = "Three"; break;
			case 4: DisplayString = "Four"; break;
			case 5: DisplayString = "Five"; break;
			default: DisplayString = "Default"; break;
			}
		}

		bool operator==(const FFoo& Other) const { return Num == Other.Num && DisplayString == Other.DisplayString; }

		bool operator!=(const FFoo& Other) const { return !(*this == Other); }

		FString ToString() const { return DisplayString; }
	};

	// Test reverse iteration on a non-const linear collection by adding elements to a result array and comparing to the
	// expected result array
	template <typename ContainerType, typename ElementType>
	void TestReverseIterator(
		FAutomationTestBase& AutomationTest,
		ContainerType& OriginalContainer,
		TArray<ElementType> ExpectedResult,
		FString CollectionName)
	{
		// Arrange
		TArray<ElementType> ResultArray;

		// Act
		for (ElementType& Element : ReverseRange(OriginalContainer))
		{
			ResultArray.Add(Element);
		}

		// Assert
		TestArraysEqual(
			AutomationTest,
			"Checking reverse iterated array against expected result",
			ResultArray,
			ExpectedResult);
	}

	// Test reverse iteration on a const linear collection by adding elements to a result array and comparing to the
	// expected result array
	template <typename ContainerType, typename ElementType>
	void TestConstReverseIterator(
		FAutomationTestBase& AutomationTest,
		const ContainerType& OriginalContainer,
		TArray<ElementType> ExpectedResult,
		FString CollectionName)
	{
		// Arrange
		TArray<ElementType> ResultArray;

		// Act
		for (const ElementType& Element : ReverseRange(OriginalContainer))
		{
			ResultArray.Add(Element);
		}

		// Assert
		TestArraysEqual(
			AutomationTest,
			"Checking reverse iterated array against expected result",
			ResultArray,
			ExpectedResult);
	}

	static_assert(
		TModels<CMemberToStringConvertable, OUU::Tests::ReverseIteratorTests::FFoo>::Value,
		"Sanity check: FFoo has ToString() member");

	static_assert(
		TModels<CLexToStringConvertible, OUU::Tests::ReverseIteratorTests::FFoo>::Value,
		"Sanity check: Because FFoo has ToString() member it should be ToStringConvertable()!");
} // namespace OUU::Tests::ReverseIteratorTests

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(CArray_Int, DEFAULT_OUU_TEST_FLAGS)
{
	using namespace OUU::Tests::ReverseIteratorTests;
	int32 OriginalCArray[5]{1, 2, 3, 4, 5};
	TestReverseIterator<int32[5], int32>(*this, OriginalCArray, TArray<int32>{5, 4, 3, 2, 1}, "int32[]");
	TestConstReverseIterator<int32[5], int32>(*this, OriginalCArray, TArray<int32>{5, 4, 3, 2, 1}, "int32[]");
	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(TArray_Int, DEFAULT_OUU_TEST_FLAGS)
{
	using namespace OUU::Tests::ReverseIteratorTests;
	TArray<int32> OriginArray = {1, 2, 3, 4, 5};
	TestReverseIterator<TArray<int32>, int32>(*this, OriginArray, TArray<int32>{5, 4, 3, 2, 1}, "TArray<int32>");
	TestConstReverseIterator<TArray<int32>, int32>(*this, OriginArray, TArray<int32>{5, 4, 3, 2, 1}, "TArray<int32>");
	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(CArray_FFoo, DEFAULT_OUU_TEST_FLAGS)
{
	using namespace OUU::Tests::ReverseIteratorTests;
	FFoo OriginCArray[5] = {1, 2, 3, 4, 5};
	TestReverseIterator<FFoo[5], FFoo>(*this, OriginCArray, TArray<FFoo>{5, 4, 3, 2, 1}, "FFoo[]");
	TestConstReverseIterator<FFoo[5], FFoo>(*this, OriginCArray, TArray<FFoo>{5, 4, 3, 2, 1}, "FFoo[]");
	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(TArray_FFoo, DEFAULT_OUU_TEST_FLAGS)
{
	using namespace OUU::Tests::ReverseIteratorTests;
	TArray<FFoo> OriginArray = {1, 2, 3, 4, 5};
	TestReverseIterator<TArray<FFoo>, FFoo>(*this, OriginArray, TArray<FFoo>{5, 4, 3, 2, 1}, "TArray<FFoo>");
	TestConstReverseIterator<TArray<FFoo>, FFoo>(*this, OriginArray, TArray<FFoo>{5, 4, 3, 2, 1}, "TArray<FFoo>");
	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(CArray_FFooPtr, DEFAULT_OUU_TEST_FLAGS)
{
	using namespace OUU::Tests::ReverseIteratorTests;

	// Arrange
	FFoo Instances[5] = {1, 2, 3, 4, 5};
	FFoo* OriginCArray[5] = {&Instances[0], &Instances[1], &Instances[2], &Instances[3], &Instances[4]};
	const TArray<FFoo*> ExpectedArray = {&Instances[4], &Instances[3], &Instances[2], &Instances[1], &Instances[0]};

	// Test
	TestReverseIterator<FFoo* [5], FFoo*>(*this, OriginCArray, ExpectedArray, "FFoo*[]");
	TestConstReverseIterator<FFoo* [5], FFoo*>(*this, OriginCArray, ExpectedArray, "FFoo*[]");

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(TArray_FFooPtr, DEFAULT_OUU_TEST_FLAGS)
{
	using namespace OUU::Tests::ReverseIteratorTests;

	// Arrange
	FFoo Instances[5] = {1, 2, 3, 4, 5};
	TArray<FFoo*> OriginArray = {&Instances[0], &Instances[1], &Instances[2], &Instances[3], &Instances[4]};
	const TArray<FFoo*> ExpectedArray = {&Instances[4], &Instances[3], &Instances[2], &Instances[1], &Instances[0]};

	// Test
	TestReverseIterator<TArray<FFoo*>, FFoo*>(*this, OriginArray, ExpectedArray, "TArray<FFoo*>");
	TestConstReverseIterator<TArray<FFoo*>, FFoo*>(*this, OriginArray, ExpectedArray, "TArray<FFoo*>");

	return true;
}

//////////////////////////////////////////////////////////////////////////

	#undef OUU_TEST_CATEGORY
	#undef OUU_TEST_TYPE

#endif
