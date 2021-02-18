// Copyright (c) 2021 Jonas Reich

#include "OUUTests.h"
#include "Templates/RingAggregator.h"

#if WITH_AUTOMATION_WORKER

BEGIN_DEFINE_SPEC(FRingAggregatorSpec, "OpenUnrealUtilities.Templates.RingAggregator", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FRingAggregatorSpec)
void FRingAggregatorSpec::Define()
{
	Describe("Fixed Size Aggregator", [this]()
	{
		It("should wrap around and overwrite the first elements after the fixed size was reached", [this]()
		{
			TFixedSizeRingAggregator<int32, 3> TestAggregator;
			TestAggregator.Add(1);
			TestAggregator.Add(2);
			TestAggregator.Add(3);
			TestAggregator.Add(4);
			const TArray<int32, TFixedAllocator<3>> ExpectedStorage = {4, 2, 3};
			TestArraysEqual(*this, "aggregator storage", TestAggregator.GetStorage(), ExpectedStorage);
		});
	});
}

#endif
