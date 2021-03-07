// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Algo/MaxElement.h"
#include "Algo/MinElement.h"
#include "Traits/IsInteger.h"

/**
 * Ring buffer that aggregates numeric data.
 * Useful e.g. for tracking data of the last X frames.
 */
template <class ChildClass, typename InElementType, typename AllocatorType>
class TRingAggregator_Base
{
public:
	using ElementType = InElementType;
	using SizeType = typename AllocatorType::SizeType;

	void Add(ElementType Element)
	{
		if (IsPreWrap())
		{
			Storage.AddUninitialized(1);
		}
		Storage[WriteIndex] = Element;
		WriteIndex = (WriteIndex + 1) % ArrayMax();
	}

	SizeType Num() const
	{
		return Storage.Num();
	}

	bool HasData() const
	{
		return Num() > 0;
	}

	ElementType Last() const
	{
		const int32 LastIdx = WriteIndex - 1;
		const int32 LastIdxWrapped = LastIdx >= 0 ? LastIdx : LastIdx + Num();
		return Storage[LastIdxWrapped];
	}

	ElementType Oldest() const
	{
		if (IsPreWrap())
		{
			return Storage[0];
		}
		return Storage[WriteIndex];
	}

	ElementType Sum() const
	{
		ElementType ResultSum = 0;
		for (ElementType Element : Storage)
		{
			AddNumbersEnsured<ElementType>(ResultSum, Element);
		}
		return ResultSum;
	}

	ElementType Average() const
	{
		return HasData() ? (Sum() / Num()) : 0;
	}

	ElementType Max() const
	{
		return HasData() ? (*Algo::MaxElement(Storage)) : 0;
	}

	ElementType Min() const
	{
		return HasData() ? (*Algo::MinElement(Storage)) : 0;
	}

	const TArray<ElementType, AllocatorType>& GetStorage() const
	{
		return Storage;
	}

	bool IsValidIndex(SizeType Index)
	{
		return Storage.IsValidIndex(GetWrappedRingIndex(Index));
	}

	ElementType& operator[](SizeType Index)
	{
		return Storage[GetWrappedRingIndex(Index)];
	}

	// Iterators
	using TIterator = TIndexedContainerIterator<TRingAggregator_Base, ElementType, SizeType> ;
	using TConstIterator = TIndexedContainerIterator<const TRingAggregator_Base, const ElementType, SizeType>;

	FORCEINLINE TIterator begin() { return TIterator(*this, 0); }
	FORCEINLINE TConstIterator begin() const { return TConstIterator(*this, 0); }
	FORCEINLINE TIterator end() { return TIterator(*this, Num()); }
	FORCEINLINE TConstIterator end() const { return TConstIterator(*this, Num()); }

protected:
	TArray<ElementType, AllocatorType> Storage;
	int32 WriteIndex = 0;

	int32 ArrayMax() const
	{
		return static_cast<const ChildClass*>(this)->ArrayMax_Implementation();
	}

	bool IsPreWrap() const
	{
		return Num() < ArrayMax();
	}

	int32 GetWrappedRingIndex(int32 Index) const
	{
		const int32 RingIndex = (WriteIndex + Index);
		const int32 WrappedRingIndex = (RingIndex >= Num()) ? (RingIndex - ArrayMax()) : RingIndex;
		return WrappedRingIndex;
	}

	/** Overload for builds with ensure macros that checks for integer overflow */
	template <typename T, typename = typename TEnableIf<TIsInteger<T>::Value && static_cast<bool>(DO_CHECK)>::Type>
	static void AddNumbersEnsured(T& A, T B)
	{
		ElementType SignBefore = FMath::Sign(A);
		A += B;
		ElementType SignAfter = FMath::Sign(A);
		ensureMsgf(SignBefore == SignAfter || ((SignBefore + SignAfter) != 0 || SignBefore != FMath::Sign(B)), TEXT("integer overflow detected"));
	}

	template <typename T, typename = typename TEnableIf<TIsInteger<T>::Value == false || static_cast<bool>(DO_CHECK) == false>::Type>
	static void AddNumbersEnsured(T& A, T B, int32 = 0)
	{
		A += B;
	}
};

/** "Normal" ring aggregator that can be initialized with different size depending on dynamic conditions */
template <typename ElementType>
class TRingAggregator : public TRingAggregator_Base<TRingAggregator<ElementType>, ElementType, FDefaultAllocator>
{
public:
	using Super = TRingAggregator_Base<TRingAggregator<ElementType>, ElementType, FDefaultAllocator>;
	friend class Super;

	TRingAggregator(int32 InitialSize)
	{
		MaxNum = InitialSize;
	}

private:
	int32 MaxNum = INDEX_NONE;

	int32 ArrayMax_Implementation() const
	{
		return MaxNum;
	}
};

/** Ring aggregator that has a compile time fixed size of elements */
template <typename ElementType, uint32 ElementNum>
class TFixedSizeRingAggregator : public TRingAggregator_Base<TFixedSizeRingAggregator<ElementType, ElementNum>, ElementType, TFixedAllocator<ElementNum>>
{
private:
	friend class TRingAggregator_Base<TFixedSizeRingAggregator<ElementType, ElementNum>, ElementType, TFixedAllocator<ElementNum>>;

	static int32 ArrayMax_Implementation()
	{
		return ElementNum;
	}
};
