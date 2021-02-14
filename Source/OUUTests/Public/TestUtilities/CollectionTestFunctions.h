// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Misc/AutomationTest.h"
#include "Templates/StringUtils.h"

#if WITH_AUTOMATION_WORKER

/** Functions used within other functions declared in this file. */
namespace OUUTests_Internal
{
	template< typename ElementType >
	void AddArrayValueError(
		FAutomationTestBase& AutomationTest,
		const FString& What,
		int32 Idx,
		const ElementType& ActualValue,
		const ElementType& ExpectedValue)
	{
		AutomationTest.AddError(
			FString::Printf(
				TEXT("%s: The two arrays have different values at index %i (expected %s, but it was %s)."),
				*What, Idx, *LexToString(ExpectedValue), *LexToString(ActualValue)), 1);
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
	const TArray<ElementType, AllocatorType>& ExpectedArray,
	const bool bPrintEntireArrayOnError = false)
{
	auto ConditionalPrintEntireArrayContents = [&]()
	{
		if (bPrintEntireArrayOnError)
		{
			AutomationTest.AddError(FString::Printf(TEXT("%s: Expected array: %s"),
				*What, *ArrayToString(ExpectedArray)));
			AutomationTest.AddError(FString::Printf(TEXT("%s: Actual array: %s"),
				*What, *ArrayToString(ActualArray)));
		}
	};

	// Quick initial test: Compare element counts
	int32 ActualNum = ActualArray.Num();
	int32 ExpectedNum = ExpectedArray.Num();
	if (ActualNum != ExpectedNum)
	{
		AutomationTest.AddError(
			FString::Printf(TEXT("%s: The two arrays have different length (expected %i, but it was %i)."),
				*What, ExpectedNum, ActualNum), 1);
		ConditionalPrintEntireArrayContents();
		return;
	}

	// More thorough test: Compare individual elements for equal values at the same index
	for (int32 i = 0; i < ActualNum; i++)
	{
		if (ActualArray[i] != ExpectedArray[i])
		{
			OUUTests_Internal::AddArrayValueError(AutomationTest, What, i, ActualArray[i], ExpectedArray[i]);
			ConditionalPrintEntireArrayContents();
			return;
		}
	}

	ensureMsgf(ActualArray == ExpectedArray, TEXT("If the two arrays did not match, we should have gotten an error before."));
}

/**
 * Test if two unordered arrays have matching elements.
 * Iterates several times over array to match array items, so it's pretty expensive.
 * However this allows to give quite detailed feedback which elements are missing from either array.
 */
template<typename ElementType, typename AllocatorType>
void TestUnorderedArraysMatch(
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
				*What, ExpectedNum, ActualNum), 1);
		return;
	}
	TArray<ElementType, AllocatorType> ActualArrayCopy = ActualArray;
	TArray<ElementType, AllocatorType> ExpectedArrayCopy = ExpectedArray;
	for (int32 i = ExpectedArrayCopy.Num() - 1; i >= 0; i--)
	{
		const auto& Value = ExpectedArrayCopy[i];
		int32 j = ActualArrayCopy.Find(Value);
		if (j == INDEX_NONE)
		{
			AutomationTest.AddError(
				FString::Printf(
					TEXT("%s: The value %s from the expected array was not found in the actual array"),
					*What, *LexToString(Value)), 1);
			return;
		}

		ExpectedArrayCopy.RemoveAtSwap(i);
		ActualArrayCopy.RemoveAtSwap(j);
	}

	if (ActualArrayCopy.Num() > 0)
	{
		AutomationTest.AddError(
			FString::Printf(
				TEXT("%s: The actual array contains %i that could not be matched to the expected array"),
				*What, ActualArrayCopy.Num()), 1);
		return;
	}

	ensureMsgf(ActualArrayCopy.Num() == 0 && ExpectedArrayCopy.Num() == 0, TEXT("If we didn't find all values in both arrays, we should have gotten an error before."));
}

#endif
