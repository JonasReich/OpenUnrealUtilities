// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "Templates/RingAggregator.h"

BEGIN_DEFINE_SPEC(FRingAggregatorSpec, "OpenUnrealUtilities.Templates.RingAggregator", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FRingAggregatorSpec)

void FRingAggregatorSpec::Define()
{
	Describe("Fixed Size Aggregator", [this]()
	{
		Describe("HasData", [this]()
		{
			It("should return false when freshly constructed", [this]()
			{
				const TFixedSizeRingAggregator<int32, 3> TestAggregator;
				SPEC_TEST_FALSE(TestAggregator.HasData());
			});

			It("should return true after the first element was added", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(1);
				SPEC_TEST_TRUE(TestAggregator.HasData());
			});
		});

		It("should wrap around and overwrite the first elements after the fixed size was reached", [this]()
		{
			TFixedSizeRingAggregator<int32, 3> TestAggregator;
			TestAggregator.Add(1);
			TestAggregator.Add(2);
			TestAggregator.Add(3);
			TestAggregator.Add(4);
			TestAggregator.Add(5);
			//   |1|2|3|
			// + |4|5| |
			// = |4|5|3|
			const TArray<int32, TFixedAllocator<3>> ExpectedStorage = {4, 5, 3};
			TestArraysEqual(*this, "aggregator storage", TestAggregator.GetStorage(), ExpectedStorage, true);
		});

		Describe("Num", [this]()
		{
			It("should return the number of items already added < max size as long as the write index did not wrap", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(1);
				TestAggregator.Add(2);
				SPEC_TEST_EQUAL(TestAggregator.Num(), 2);
			});

			It("should return the number of items still stored as soon as the aggregator wrapped and already overwrote some elements", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(1);
				TestAggregator.Add(2);
				TestAggregator.Add(3);
				TestAggregator.Add(4);
				TestAggregator.Add(5);
				TestAggregator.Add(6);
				SPEC_TEST_EQUAL(TestAggregator.Num(), 3);
			});
		});

		It("Average", [this]()
		{
			TFixedSizeRingAggregator<int32, 3> TestAggregator;
			TestAggregator.Add(1);
			TestAggregator.Add(2);
			TestAggregator.Add(3);
			const int32 Avg = TestAggregator.Average();
			SPEC_TEST_EQUAL(Avg, 2);
		});

		Describe("Oldest", [this]()
		{
			It("should return the first element as long as the limit was not surpassed", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(1);
				TestAggregator.Add(2);
				const int32 Oldest = TestAggregator.Oldest();
				SPEC_TEST_EQUAL(Oldest, 1);
			});

			It("should return the oldest element that was not overwritten as soon as the array limit was surpassed", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(1);
				TestAggregator.Add(2);
				TestAggregator.Add(3);
				TestAggregator.Add(4);
				const int32 Oldest = TestAggregator.Oldest();
				SPEC_TEST_EQUAL(Oldest, 2);
			});
		});

		Describe("Last", [this]()
		{
			It("should return the item that was added last when the buffer did not wrap yet", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(1);
				TestAggregator.Add(2);
				const int32 Last = TestAggregator.Last();
				SPEC_TEST_EQUAL(Last, 2);
			});

			It("should return the first indexed item immediately after wrap-around", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(1);
				TestAggregator.Add(2);
				TestAggregator.Add(3);
				TestAggregator.Add(4);
				const int32 Last = TestAggregator.Last();
				SPEC_TEST_EQUAL(Last, 4);

				TestAggregator.Add(5);
				const int32 LastStepTwo = TestAggregator.Last();
				SPEC_TEST_EQUAL(LastStepTwo, 5);
			});
		});

		Describe("Max", [this]()
		{
			It("should return 3 for the sequence 1,2,3", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(1);
				TestAggregator.Add(2);
				TestAggregator.Add(3);
				SPEC_TEST_EQUAL(TestAggregator.Max(), 3);
			});
			It("should return 3 for the sequence 3,2,1", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(3);
				TestAggregator.Add(2);
				TestAggregator.Add(1);
				SPEC_TEST_EQUAL(TestAggregator.Max(), 3);
			});
			It("should return 3 for the sequence 2,3,1", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(2);
				TestAggregator.Add(3);
				TestAggregator.Add(1);
				SPEC_TEST_EQUAL(TestAggregator.Max(), 3);
			});
		});

		Describe("Min", [this]()
		{
			It("should return 1 for the sequence 1,2,3", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(1);
				TestAggregator.Add(2);
				TestAggregator.Add(3);
				SPEC_TEST_EQUAL(TestAggregator.Max(), 3);
			});
			It("should return 1 for the sequence 3,2,1", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(3);
				TestAggregator.Add(2);
				TestAggregator.Add(1);
				SPEC_TEST_EQUAL(TestAggregator.Max(), 3);
			});
			It("should return 1 for the sequence 2,3,1", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(2);
				TestAggregator.Add(3);
				TestAggregator.Add(1);
				SPEC_TEST_EQUAL(TestAggregator.Max(), 3);
			});
		});

		Describe("ranged-based for loops", [this]()
		{
			It("should match a range-based floor on underlying data as long before wrap-around", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(1);
				TestAggregator.Add(2);
				TestAggregator.Add(3);
				TArray<int32> NumbersInRangedFor = {};
				for (int32 i : TestAggregator)
				{
					NumbersInRangedFor.Add(i);
				}
				const TArray<int32> ExpectedNumbers = {1, 2, 3};
				TestArraysEqual(*this, "items returned from ranged-based for loop", NumbersInRangedFor, ExpectedNumbers, true);
			});

			It("should still return all items in chronological order after wrap-around", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(1);
				TestAggregator.Add(2);
				TestAggregator.Add(3);
				TestAggregator.Add(4);
				TestAggregator.Add(5);
				TArray<int32> NumbersInRangedFor = {};
				for (int32 i : TestAggregator)
				{
					NumbersInRangedFor.Add(i);
				}
				const TArray<int32> ExpectedNumbers = {3, 4, 5};
				TestArraysEqual(*this, "items returned from ranged-based for loop", NumbersInRangedFor, ExpectedNumbers, true);
			});
		});

		Describe("operator[]", [this]()
		{
			It("should allow access to the correct elements (before wrap)", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(42);
				SPEC_TEST_EQUAL(TestAggregator[0], 42);
				TestAggregator.Add(3);
				SPEC_TEST_EQUAL(TestAggregator[0], 42);
				SPEC_TEST_EQUAL(TestAggregator[1], 3);
				TestAggregator.Add(4);
				SPEC_TEST_EQUAL(TestAggregator[0], 42);
				SPEC_TEST_EQUAL(TestAggregator[1], 3);
				SPEC_TEST_EQUAL(TestAggregator[2], 4);
			});

			It("should allow access to the correct elements (after wrap)", [this]()
			{
				TFixedSizeRingAggregator<int32, 3> TestAggregator;
				TestAggregator.Add(42);
				TestAggregator.Add(3);
				TestAggregator.Add(4);
				TestAggregator.Add(69);
				SPEC_TEST_EQUAL(TestAggregator[0], 3);
				SPEC_TEST_EQUAL(TestAggregator[1], 4);
				SPEC_TEST_EQUAL(TestAggregator[2], 69);
			});
		});
	});

	Describe("TRingAggregator", [this]()
	{
		It("should be reconstructable with different sizes", [this]()
		{
			TRingAggregator<int32> TestAggregator(0);
			// Iterate from 5 to 1, decreasing the number of available elements per iteration.
			// That way we can be sure that the test does not succeed simply because
			// there is still space allocated because of the previous iteration.
			for (int32 i = 5; i > 0; i--)
			{
				TestAggregator = TRingAggregator<int32>(i);
				for (int32 j = 0; j < i + 1; j++)
				{
					TestAggregator.Add(j);
				}
				SPEC_TEST_EQUAL(TestAggregator.Num(), i);
			}
		});
	});
}

#endif
