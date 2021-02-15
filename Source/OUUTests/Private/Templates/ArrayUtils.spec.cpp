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

	Describe("TakeAt", [this]()
	{
		It("should remove the correct element from the array", [this]()
		{
			TArray<int32> SourceArray = {1, 2, 3, 4, 5};
			const int32 Element = FArrayUtils::TakeAt(SourceArray, 2);
			SPEC_TEST_EQUAL(Element, 3);
			SPEC_TEST_TRUE(SourceArray.Num() == 4);
		});

		It("should retain the correct order of remaining elements", [this]()
        {
            TArray<int32> SourceArray = {1, 2, 3, 4, 5};
            FArrayUtils::TakeAt(SourceArray, 2);
			TestArraysEqual(*this, "TestArray", SourceArray, TArray<int32>{1, 2, 4, 5});
        });
	});

	Describe("TakeAtSwap", [this]()
	{
		It("should remove the correct element from the array", [this]()
        {
            TArray<int32> SourceArray = {1, 2, 3, 4, 5};
            const int32 Element = FArrayUtils::TakeAtSwap(SourceArray, 2);
            SPEC_TEST_EQUAL(Element, 3);
            SPEC_TEST_TRUE(SourceArray.Num() == 4);
        });

        It("should leave the correct elements in unspecified order", [this]()
        {
            TArray<int32> SourceArray = {1, 2, 3, 4, 5};
            FArrayUtils::TakeAtSwap(SourceArray, 2);
            TestUnorderedArraysMatch(*this, "TestArray", SourceArray, TArray<int32>{1, 2, 4, 5});
        });
	});

	Describe("GetRandomElement", [this]()
	{
		It("should never cause any access violations when called many (100) times", [this]()
		{
			const TArray<int32> SourceArray = {1, 2, 3, 4, 5};
			for (int32 i = 0; i < 100; i++)
			{
				FArrayUtils::GetRandomElement(SourceArray);
			}
		});

		It("should always return the same element when used with the same seed/random stream", [this]()
		{
			const TArray<int32> SourceArray = {1, 2, 3, 4, 5};
			for (int32 i = 0; i < 5; i++)
            {
				int32 LastIterationResult = INDEX_NONE;
				for (int32 j = 0; j < 5; j++)
				{
					FRandomStream Stream = i;
					const int32 CurrentResult = FArrayUtils::GetRandomElement(SourceArray, Stream);
					if (LastIterationResult == INDEX_NONE)
					{
						LastIterationResult = CurrentResult;
					}
					else
					{
						SPEC_TEST_EQUAL(LastIterationResult, CurrentResult);
					}
				}
            }
		});
	});

	Describe("TakeRandomElement", [this]()
	{
		It("should remove the returned element from the array", [this]()
        {
            TArray<int32> SourceArray = {1, 2, 3, 4, 5};
            const int32 Element = FArrayUtils::TakeRandomElement(SourceArray);
            SPEC_TEST_FALSE(SourceArray.Contains(Element));
            SPEC_TEST_TRUE(SourceArray.Num() == 4);
        });

		It("should completely empty the array when invoked array-length times", [this]()
		{
			TArray<int32> SourceArray = {1, 2, 3, 4, 5};
			for (int32 i = 0; i < 5; i++)
			{
				FArrayUtils::TakeRandomElement(SourceArray);
			}
			SPEC_TEST_EQUAL(SourceArray.Num(), 0);
		});

		It("should always return the same element when used with the same seed/random stream", [this]()
        {
            const TArray<int32> SourceArray = {1, 2, 3, 4, 5};
            for (int32 i = 0; i < 5; i++)
            {
                int32 LastIterationResult = INDEX_NONE;
                for (int32 j = 0; j < 5; j++)
                {
                	TArray<int32> SourceArrayCopy = SourceArray;
                    FRandomStream Stream = i;
					const int32 CurrentResult = FArrayUtils::TakeRandomElement(SourceArrayCopy, Stream);
                    if (LastIterationResult == INDEX_NONE)
                    {
                        LastIterationResult = CurrentResult;
                    }
                    else
                    {
                        SPEC_TEST_EQUAL(CurrentResult, LastIterationResult);
                    }
                }
            }
        });

		It("should retain the correct order of remaining elements", [this]()
        {
            const TArray<int32> SourceArray = {1, 2, 3, 4, 5};
            TArray<int32> SourceArrayCopy = SourceArray;
			TArray<int32> SourceArrayREferenceCopy = SourceArray;
            const int32 Element = FArrayUtils::TakeRandomElement(SourceArrayCopy);
			const int32 ElementIndex = SourceArray.Find(Element);
            SourceArrayREferenceCopy.RemoveAt(ElementIndex);
            TestArraysEqual(*this, "TestArray", SourceArrayREferenceCopy, SourceArrayCopy);
        });
	});
}

#endif
