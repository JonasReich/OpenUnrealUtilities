// Copyright (c) 2020 Jonas Reich

#pragma once

#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "Templates/ArrayUtils.h"

#define OUU_TEST_CATEGORY OpenUnrealUtilities
#define OUU_TEST_TYPE ArrayUtils

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(SetAllTo, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	TArray<int32> WorkingArray = { 1, 2, 3, 4, 5, 6 };
	const TArray<int32> ExpectedResult = { 8, 8, 8, 8, 8, 8 };

	// Act
	FArrayUtils::SetAllTo(WorkingArray, 8);
	
	// Assert
	TestArraysEqual(*this, "All elements are set to the same value", WorkingArray, ExpectedResult);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(SetNumTo_Increase, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	TArray<int32> WorkingArray = { 1, 2, 3 };
	const TArray<int32> ExpectedResult = { 8, 8, 8, 8, 8, 8 };

	// Act
	FArrayUtils::SetNumTo(WorkingArray, 6, 8);

	// Assert
	TestArraysEqual(*this, "All elements are set to the same value, count was increased", WorkingArray, ExpectedResult);

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(SetNumTo_Decrease, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	TArray<int32> WorkingArray = { 1, 2, 3, 4, 5, 6 };
	const TArray<int32> ExpectedResult = { 8, 8, 8 };

	// Act
	FArrayUtils::SetNumTo(WorkingArray, 3, 8);

	// Assert
	TestArraysEqual(*this, "All elements are set to the same value, count was decreased", WorkingArray, ExpectedResult);

	return true;
}

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_CATEGORY
#undef OUU_TEST_TYPE

#endif
