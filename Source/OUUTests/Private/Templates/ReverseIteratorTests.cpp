// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "Engine/EngineTypes.h"
#include "Templates/ReverseIterator.h"
#include "Algo/IsSorted.h"

#define OUU_TEST_CATEGORY OpenUnrealUtilities.Templates
#define OUU_TEST_TYPE ReverseIterator

namespace ReverseIteratorTests
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
			case 1:
				DisplayString = "One";
				break;
			case 2:
				DisplayString = "Two";
				break;
			case 3:
				DisplayString = "Three";
				break;
			case 4:
				DisplayString = "Four";
				break;
			case 5:
				DisplayString = "Five";
				break;
			default:
				DisplayString = "Default";
				break;
			}
		}

		bool operator==(const FFoo& Other) const
		{
			return Num == Other.Num && DisplayString == Other.DisplayString;
		}

		bool operator!=(const FFoo& Other) const
		{
			return !(*this == Other);
		}

		FString ToString() const
		{
			return DisplayString;
		}
	};

	// Test reverse iteration on a non-const linear collection by adding elements to a result array and comparing to the expected result array
	template<typename ContainerType, typename ElementType>
	void TestReverseIterator(FAutomationTestBase& AutomationTest, ContainerType& OriginalContainer, TArray<ElementType> ExpectedResult, FString CollectionName)
	{
		// Arrange
		TArray<ElementType> ResultArray;

		// Act
		for (ElementType& Element : ReverseRange(OriginalContainer))
		{
			ResultArray.Add(Element);
		}

		// Assert
		TestArraysEqual(AutomationTest, "Checking reverse iterated array against expected result", ResultArray, ExpectedResult);
	}

	// Test reverse iteration on a const linear collection by adding elements to a result array and comparing to the expected result array
	template<typename ContainerType, typename ElementType>
	void TestConstReverseIterator(FAutomationTestBase& AutomationTest, const ContainerType& OriginalContainer, TArray<ElementType> ExpectedResult, FString CollectionName)
	{
		// Arrange
		TArray<ElementType> ResultArray;

		// Act
		for (const ElementType& Element : ReverseRange(OriginalContainer))
		{
			ResultArray.Add(Element);
		}

		// Assert
		TestArraysEqual(AutomationTest, "Checking reverse iterated array against expected result", ResultArray, ExpectedResult);
	}
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(CArray_Int, DEFAULT_OUU_TEST_FLAGS)
{
	int32 OriginalCArray[5]{ 1, 2, 3, 4, 5 };
	ReverseIteratorTests::TestReverseIterator<int32[5], int32>(*this, OriginalCArray, { 5, 4, 3, 2, 1 }, "int32[]");
	ReverseIteratorTests::TestConstReverseIterator<int32[5], int32>(*this, OriginalCArray, { 5, 4, 3, 2, 1 }, "int32[]");
	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(TArray_Int, DEFAULT_OUU_TEST_FLAGS)
{
	TArray<int32> OriginArray = { 1, 2, 3, 4, 5 };
	ReverseIteratorTests::TestReverseIterator<TArray<int32>, int32>(*this, OriginArray, { 5, 4, 3, 2, 1 }, "TArray<int32>");
	ReverseIteratorTests::TestConstReverseIterator<TArray<int32>, int32>(*this, OriginArray, { 5, 4, 3, 2, 1 }, "TArray<int32>");
	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(CArray_FFoo, DEFAULT_OUU_TEST_FLAGS)
{
	using ReverseIteratorTests::FFoo;
	FFoo OriginCArray[5] = { 1, 2, 3, 4, 5 };
	ReverseIteratorTests::TestReverseIterator<FFoo[5], FFoo>(*this, OriginCArray, { 5, 4, 3, 2, 1 }, "FFoo[]");
	ReverseIteratorTests::TestConstReverseIterator<FFoo[5], FFoo>(*this, OriginCArray, { 5, 4, 3, 2, 1 }, "FFoo[]");
	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(TArray_FFoo, DEFAULT_OUU_TEST_FLAGS)
{
	using ReverseIteratorTests::FFoo;
	TArray<FFoo> OriginArray = { 1, 2, 3, 4, 5 };
	ReverseIteratorTests::TestReverseIterator<TArray<FFoo>, FFoo>(*this, OriginArray, { 5, 4, 3, 2, 1 }, "TArray<FFoo>");
	ReverseIteratorTests::TestConstReverseIterator<TArray<FFoo>, FFoo>(*this, OriginArray, { 5, 4, 3, 2, 1 }, "TArray<FFoo>");
	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(CArray_FFooPtr, DEFAULT_OUU_TEST_FLAGS)
{
	using ReverseIteratorTests::FFoo;

	// Arrange
	FFoo Instances[5] = { 1, 2, 3, 4, 5 };
	FFoo* OriginCArray[5] = { &Instances[0], &Instances[1], &Instances[2], &Instances[3], &Instances[4] };
	TArray<FFoo*> ExpectedArray = { &Instances[4], &Instances[3], &Instances[2], &Instances[1], &Instances[0] };

	// Test
	ReverseIteratorTests::TestReverseIterator<FFoo* [5], FFoo*>(*this, OriginCArray, ExpectedArray, "FFoo*[]");
	ReverseIteratorTests::TestConstReverseIterator<FFoo* [5], FFoo*>(*this, OriginCArray, ExpectedArray, "FFoo*[]");

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(TArray_FFooPtr, DEFAULT_OUU_TEST_FLAGS)
{
	using ReverseIteratorTests::FFoo;

	// Arrange
	FFoo Instances[5] = { 1, 2, 3, 4, 5 };
	TArray<FFoo*> OriginArray = { &Instances[0], &Instances[1], &Instances[2], &Instances[3], &Instances[4] };
	TArray<FFoo*> ExpectedArray = { &Instances[4], &Instances[3], &Instances[2], &Instances[1], &Instances[0] };

	// Test
	ReverseIteratorTests::TestReverseIterator<TArray<FFoo*>, FFoo*>(*this, OriginArray, ExpectedArray, "TArray<FFoo*>");
	ReverseIteratorTests::TestConstReverseIterator<TArray<FFoo*>, FFoo*>(*this, OriginArray, ExpectedArray, "TArray<FFoo*>");

	return true;
}

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_CATEGORY
#undef OUU_TEST_TYPE

#endif
