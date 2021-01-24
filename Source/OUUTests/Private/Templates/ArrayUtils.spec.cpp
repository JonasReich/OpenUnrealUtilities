// Copyright (c) 2021 Jonas Reich

#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "Templates/ArrayUtils.h"

BEGIN_DEFINE_SPEC(FArrayUtilsSpec, "OpenUnrealUtilities.Templates.ArrayUtils", DEFAULT_OUU_TEST_FLAGS)
TArray<int32> WorkingArray;
END_DEFINE_SPEC(FArrayUtilsSpec)
void FArrayUtilsSpec::Define()
{
	BeforeEach([this]()
	{
		WorkingArray = { 1, 2, 3, 4, 5, 6 };
	});

	It("SetAllTo should set all elements in an array to the same value", [this]()
	{
		FArrayUtils::SetAllTo(WorkingArray, 8);
		TestArraysEqual(*this, "All elements are set to the same value", WorkingArray, { 8, 8, 8, 8, 8, 8 });
	});

	Describe("SetNumTo", [this]()
	{
		It("should decrease the number of elements when called with a value smaller than current count", [this]()
		{
			FArrayUtils::SetNumTo(WorkingArray, 3, 8);
			TestArraysEqual(*this, "All elements are set to the same value, count was increased", WorkingArray, { 8, 8, 8 });
		});

		It("should increase the number of elements when called with a value bigger than current count", [this]()
		{
			FArrayUtils::SetNumTo(WorkingArray, 10, 8);
			TestArraysEqual(*this, "All elements are set to the same value, count was increased", WorkingArray, { 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 });
		});
	});
}

#endif
