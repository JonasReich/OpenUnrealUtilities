// Copyright (c) 2022 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "Templates/ArrayUtils.h"

BEGIN_DEFINE_SPEC(FArrayUtilsSpec, "OpenUnrealUtilities.Runtime.Templates.ArrayUtils", DEFAULT_OUU_TEST_FLAGS)
	TArray<int32> WorkingArray;
END_DEFINE_SPEC(FArrayUtilsSpec)

void FArrayUtilsSpec::Define()
{
	BeforeEach([this]() { WorkingArray = {1, 2, 3, 4, 5, 6}; });

	It("SetAllTo should set all elements in an array to the same value", [this]() {
		OUU::Runtime::ArrayUtils::SetAllTo(WorkingArray, 8);
		SPEC_TEST_ARRAYS_EQUAL(WorkingArray, (TArray<int32>{8, 8, 8, 8, 8, 8}));
	});

	Describe("SetNumTo", [this]() {
		It("should decrease the number of elements when called with a value smaller than current count", [this]() {
			OUU::Runtime::ArrayUtils::SetNumTo(WorkingArray, 3, 8);
			SPEC_TEST_ARRAYS_EQUAL(WorkingArray, (TArray<int32>{8, 8, 8}));
		});

		It("should increase the number of elements when called with a value bigger than current count", [this]() {
			OUU::Runtime::ArrayUtils::SetNumTo(WorkingArray, 10, 8);
			SPEC_TEST_ARRAYS_EQUAL(WorkingArray, (TArray<int32>{8, 8, 8, 8, 8, 8, 8, 8, 8, 8}));
		});
	});

	Describe("TakeAt", [this]() {
		It("should remove the correct element from the array", [this]() {
			TArray<int32> SourceArray = {1, 2, 3, 4, 5};
			const int32 Element = OUU::Runtime::ArrayUtils::TakeAt(SourceArray, 2);
			SPEC_TEST_EQUAL(Element, 3);
			SPEC_TEST_TRUE(SourceArray.Num() == 4);
		});

		It("should retain the correct order of remaining elements", [this]() {
			TArray<int32> SourceArray = {1, 2, 3, 4, 5};
			OUU::Runtime::ArrayUtils::TakeAt(SourceArray, 2);
			SPEC_TEST_ARRAYS_EQUAL(SourceArray, (TArray<int32>{1, 2, 4, 5}));
		});
	});

	Describe("TakeAtSwap", [this]() {
		It("should remove the correct element from the array", [this]() {
			TArray<int32> SourceArray = {1, 2, 3, 4, 5};
			const int32 Element = OUU::Runtime::ArrayUtils::TakeAtSwap(SourceArray, 2);
			SPEC_TEST_EQUAL(Element, 3);
			SPEC_TEST_TRUE(SourceArray.Num() == 4);
		});

		It("should leave the correct elements in unspecified order", [this]() {
			TArray<int32> SourceArray = {1, 2, 3, 4, 5};
			OUU::Runtime::ArrayUtils::TakeAtSwap(SourceArray, 2);
			SPEC_TEST_ARRAYS_MATCH_UNORDERED(SourceArray, (TArray<int32>{1, 2, 4, 5}));
		});
	});

	Describe("GetRandomElement", [this]() {
		It("should never cause any access violations when called many (100) times", [this]() {
			const TArray<int32> SourceArray = {1, 2, 3, 4, 5};
			for (int32 i = 0; i < 100; i++)
			{
				OUU::Runtime::ArrayUtils::GetRandomElement(SourceArray);
			}
		});

		It("should always return the same element when used with the same seed/random stream", [this]() {
			const TArray<int32> SourceArray = {1, 2, 3, 4, 5};
			for (int32 i = 0; i < 5; i++)
			{
				int32 LastIterationResult = INDEX_NONE;
				for (int32 j = 0; j < 5; j++)
				{
					FRandomStream Stream = i;
					const int32 CurrentResult = OUU::Runtime::ArrayUtils::GetRandomElement(SourceArray, Stream);
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

	Describe("TakeRandomElement", [this]() {
		It("should remove the returned element from the array", [this]() {
			TArray<int32> SourceArray = {1, 2, 3, 4, 5};
			const int32 Element = OUU::Runtime::ArrayUtils::TakeRandomElement(SourceArray);
			SPEC_TEST_FALSE(SourceArray.Contains(Element));
			SPEC_TEST_TRUE(SourceArray.Num() == 4);
		});

		It("should completely empty the array when invoked array-length times", [this]() {
			TArray<int32> SourceArray = {1, 2, 3, 4, 5};
			for (int32 i = 0; i < 5; i++)
			{
				OUU::Runtime::ArrayUtils::TakeRandomElement(SourceArray);
			}
			SPEC_TEST_EQUAL(SourceArray.Num(), 0);
		});

		It("should always return the same element when used with the same seed/random stream", [this]() {
			const TArray<int32> SourceArray = {1, 2, 3, 4, 5};
			for (int32 i = 0; i < 5; i++)
			{
				int32 LastIterationResult = INDEX_NONE;
				for (int32 j = 0; j < 5; j++)
				{
					TArray<int32> SourceArrayCopy = SourceArray;
					FRandomStream Stream = i;
					const int32 CurrentResult = OUU::Runtime::ArrayUtils::TakeRandomElement(SourceArrayCopy, Stream);
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

		It("should retain the correct order of remaining elements", [this]() {
			const TArray<int32> SourceArray = {1, 2, 3, 4, 5};
			TArray<int32> SourceArrayCopy = SourceArray;
			TArray<int32> SourceArrayReferenceCopy = SourceArray;
			const int32 Element = OUU::Runtime::ArrayUtils::TakeRandomElement(SourceArrayCopy);
			const int32 ElementIndex = SourceArray.Find(Element);
			SourceArrayReferenceCopy.RemoveAt(ElementIndex);
			SPEC_TEST_ARRAYS_EQUAL(SourceArrayReferenceCopy, SourceArrayCopy);
		});
	});

	Describe("Slice", [this]() {
		Describe("called with a single input index", [this]() {
			It("should give access to elements using a positive integer index just like regular indexing", [this]() {
				for (int32 i = 0; i < WorkingArray.Num(); i++)
				{
					int32& RegularIndexedElement = WorkingArray[i];
					int32& SliceIndexedElement = OUU::Runtime::ArrayUtils::Slice(WorkingArray, i);
					SPEC_TEST_EQUAL(&RegularIndexedElement, &SliceIndexedElement);
				}
			});

			It("should give access to elements using a negative integer index by counting elements from the end (like "
			   "in Python)",
			   [this]() {
				   // Check all variants
				   for (int32 i = 1; i <= WorkingArray.Num(); i++)
				   {
					   int32& RegularIndexedElement = WorkingArray[WorkingArray.Num() - i];
					   int32& SliceIndexedElement = OUU::Runtime::ArrayUtils::Slice(WorkingArray, -i);
					   SPEC_TEST_EQUAL(&RegularIndexedElement, &SliceIndexedElement);
				   }

				   // Check one variant explicitly
				   SPEC_TEST_EQUAL(OUU::Runtime::ArrayUtils::Slice(WorkingArray, -2), 5);
			   });
		});

		Describe("called with two input indices", [this]() {
			It("should copy a range of array elements using positive integers", [this]() {
				SPEC_TEST_ARRAYS_EQUAL(OUU::Runtime::ArrayUtils::Slice(WorkingArray, 2, 3), (TArray<int32>{3, 4}));
			});

			It("should copy a range of array elements using negative integers", [this]() {
				SPEC_TEST_ARRAYS_EQUAL(OUU::Runtime::ArrayUtils::Slice(WorkingArray, -4, -2), (TArray<int32>{3, 4, 5}));
			});

			It("should copy a range of array elements using a mix of positive and negative integers (positive then "
			   "negative)",
			   [this]() {
				   SPEC_TEST_ARRAYS_EQUAL(
					   OUU::Runtime::ArrayUtils::Slice(WorkingArray, 2, -2),
					   (TArray<int32>{3, 4, 5}));
			   });

			It("should copy a range of array elements using a mix of positive and negative integers (negative then "
			   "positive)",
			   [this]() {
				   SPEC_TEST_ARRAYS_EQUAL(
					   OUU::Runtime::ArrayUtils::Slice(WorkingArray, -4, 5),
					   (TArray<int32>{3, 4, 5, 6}));
			   });
		});
	});
}

#endif
