// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Containers/Array.h"

class OUURUNTIME_API FArrayUtils
{
public:
	/** Set all existing elements in an array to the same value */
	template <typename ElementType, typename AllocatorType>
	static void SetAllTo(TArray<ElementType, AllocatorType>& Array, const ElementType& Value)
	{
		for (int32 i = 0; i < Array.Num(); i++)
		{
			Array[i] = Value;
		}
	}

	/**
	 * Change an array to the specified length (same as the SetNum() member function)
	 * and assign all elements to the same value.
	 */
	template <typename ElementType, typename AllocatorType>
	static void SetNumTo(TArray<ElementType, AllocatorType>& Array, int32 Num, const ElementType& Value)
	{
		Array.SetNum(Num);
		SetAllTo(Array, Value);
	}

	/**
	 * Removes and returns an element from the array from a specified index.
	 */
	template <typename ElementType, typename AllocatorType>
	static ElementType TakeAt(TArray<ElementType, AllocatorType>& Array, int32 Index)
	{
		check(Array.Num() > 0);
		ElementType Result = Array[Index];
		Array.RemoveAt(Index);
		return Result;
	}

	/**
	 * Removes and returns an element from the array from a specified index using RemoveAtSwap function.
	 * This version is much more efficient than TakeAt (O(Count) instead of
	 * O(ArrayNum)), but does not preserve the order.
	 */
	template <typename ElementType, typename AllocatorType>
	static ElementType TakeAtSwap(TArray<ElementType, AllocatorType>& Array, int32 Index)
	{
		check(Array.Num() > 0);
		ElementType Result = Array[Index];
		Array.RemoveAtSwap(Index);
		return Result;
	}

	/**
	 * Get a random array element based on a deterministic random stream sample.
	 * The array must contain at least one element!
	 */
	template <typename ElementType, typename AllocatorType>
	static ElementType GetRandomElement(const TArray<ElementType, AllocatorType>& Array, FRandomStream& Stream)
	{
		check(Array.Num() > 0);
		return Array[Stream.RandHelper(Array.Num())];
	}

	/**
	 * Get a random array element based on a non-deterministic FMath random sample.
	 * The array must contain at least one element!
	 */
	template <typename ElementType, typename AllocatorType>
	static ElementType GetRandomElement(const TArray<ElementType, AllocatorType>& Array)
	{
		check(Array.Num() > 0);
		return Array[FMath::RandRange(0, Array.Num() - 1)];
	}

	/**
	 * Remove and return a random element from the array.
	 * Randomness is deterministic based on random stream.
	 * The array must contain at least one element!
	 */
	template <typename ElementType, typename AllocatorType>
	static ElementType TakeRandomElement(TArray<ElementType, AllocatorType>& Array, FRandomStream& Stream)
	{
		check(Array.Num() > 0);
		const int32 Index = Stream.RandHelper(Array.Num());
		return TakeAt(Array, Index);
	}

	/**
	 * Remove and return a random element from the array.
	 * Randomness is deterministic based on random stream.
	 * The array must contain at least one element!
	 * This version is much more efficient than TakeRandomElement (O(Count) instead of
	 * O(ArrayNum)), but does not preserve the order.
	 */
	template <typename ElementType, typename AllocatorType>
	static ElementType TakeRandomElementSwap(TArray<ElementType, AllocatorType>& Array, FRandomStream& Stream)
	{
		check(Array.Num() > 0);
		const int32 Index = Stream.RandHelper(Array.Num());
		return TakeAtSwap(Array, Index);
	}

	/**
	 * Remove and return a random element from the array.
	 * Randomness is non-deterministic based on FMath random.
	 * The array must contain at least one element!
	 */
	template <typename ElementType, typename AllocatorType>
	static ElementType TakeRandomElement(TArray<ElementType, AllocatorType>& Array)
	{
		check(Array.Num() > 0);
		const int32 Index = FMath::RandRange(0, Array.Num() - 1);
		return TakeAt(Array, Index);
	}

	/**
	 * Remove and return a random element from the array.
	 * Randomness is non-deterministic based on FMath random.
	 * The array must contain at least one element!
	 * This version is much more efficient than TakeRandomElement (O(Count) instead of
	 * O(ArrayNum)), but does not preserve the order.
	 */
	template <typename ElementType, typename AllocatorType>
	static ElementType TakeRandomElementSwap(TArray<ElementType, AllocatorType>& Array)
	{
		check(Array.Num() > 0);
		const int32 Index = FMath::RandRange(0, Array.Num() - 1);
		return TakeAtSwap(Array, Index);
	}

	/**
	 * Copies the elements from a given array range to a new array.
	 */
	template <typename ElementType, typename AllocatorType>
	static TArray<ElementType, AllocatorType> CopyRange(TArray<ElementType, AllocatorType>& SourceArray, int32 StartIndex, int32 EndIndex)
	{
		check(SourceArray.IsValidIndex(StartIndex));
		check(SourceArray.IsValidIndex(EndIndex));
		return TArray<ElementType, AllocatorType>(&SourceArray[StartIndex], EndIndex - StartIndex);
	}

	/**
	 * Gives access to a single array element using slice index (negative numbers accesses elements from the end).
	 */
	template <typename ElementType, typename AllocatorType>
    static ElementType& Slice(TArray<ElementType, AllocatorType>& Array, int32 Index)
	{
		Index = SliceIndex(Array, Index);
		check(Array.IsValidIndex(Index));
		return Array[Index];
	}

	
	/**
	 * Gives access to a single array element using slice index (negative numbers accesses elements from the end).
	 */
	template <typename ElementType, typename AllocatorType>
	static const ElementType& Slice(const TArray<ElementType, AllocatorType>& Array, int32 Index)
	{
		return Array[SliceIndex(Array, Index)];
	}

	/**
	 * Copies the elements from a given array range to a new array.
	 * Negative indices can be used to convey positions from the end of the array.
	 */
	template <typename ElementType, typename AllocatorType>
	static TArray<ElementType, AllocatorType> Slice(const TArray<ElementType, AllocatorType>& SourceArray, int32 StartIndex, int32 EndIndex)
	{
		StartIndex = SliceIndex(SourceArray, StartIndex);
		EndIndex = SliceIndex(SourceArray, EndIndex);
		check(SourceArray.IsValidIndex(StartIndex));
		check(SourceArray.IsValidIndex(EndIndex));
		checkf(StartIndex < EndIndex, TEXT("Resolved StartIndex is expected to be smaller than resolved EndIndex. "
			"Dynamic reversal of ranges is not support at the moment! Please use ReverseRange() for those cases."));

		return TArray<ElementType, AllocatorType>(&SourceArray[StartIndex], EndIndex - StartIndex + 1);
	}

	/**
	 * Converts a slice index into a regular index.
	 */
	template <typename ElementType, typename AllocatorType>
	static int32 SliceIndex(const TArray<ElementType, AllocatorType>& Array, int32 Index)
	{
		return Index >= 0 ? Index : Array.Num() + Index;
	}
};
