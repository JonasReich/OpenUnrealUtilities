// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CircularArrayAdaptor.h"
#include "Algo/MaxElement.h"
#include "Algo/MinElement.h"
#include "Containers/Array.h"
#include "Traits/IsInteger.h"

/**
 * Ring buffer that aggregates numeric data.
 * Useful e.g. for tracking data of the last X frames.
 */
template <class ChildClass, typename InElementType, typename AllocatorType>
class TRingAggregator_Base : public TCircularArrayAdaptor_Base<ChildClass, InElementType, AllocatorType>
{
public:
	using ElementType = InElementType;
	using Super = TCircularArrayAdaptor_Base<ChildClass, InElementType, AllocatorType>;
	using ArrayType = TArray<ElementType, AllocatorType>;

	TRingAggregator_Base() :
		Super(Storage, 32),
		Storage({})
	{
		Super::StorageReference = Storage;
	}

	TRingAggregator_Base(int32 MaxNum) :
		Super(Storage, MaxNum),
		Storage({})
	{
		Super::StorageReference = Storage;
		Storage.Reserve(MaxNum);
		check(&Storage == &GetStorage());
		check(Storage == GetStorage());
	}

	TRingAggregator_Base(const TRingAggregator_Base& Other) : TRingAggregator_Base()
	{
		*this = Other;
	}
	TRingAggregator_Base(TRingAggregator_Base&& Other) noexcept : TRingAggregator_Base()
	{
		*this = MoveTemp(Other);
	}
	TRingAggregator_Base& operator=(const TRingAggregator_Base& Other)
	{
		*(static_cast<Super*>(this)) = Other;
		Storage = Other.Storage;
		Super::StorageReference = Storage;
		return *this;
	}
	TRingAggregator_Base& operator=(TRingAggregator_Base&& Other) noexcept
	{
		*(static_cast<Super*>(this)) = MoveTemp(Other);
		Storage = MoveTemp(Other.Storage);
		Super::StorageReference = Storage;
		return *this;
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

protected:
	TArray<ElementType, AllocatorType> Storage = {};

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
	TRingAggregator() = default;
	TRingAggregator(int32 MaxNum) : Super(MaxNum) {}
};

/** Ring aggregator that has a compile time fixed size of elements */
template <typename ElementType, uint32 ElementNum>
class TFixedSizeRingAggregator : public TRingAggregator_Base<TFixedSizeRingAggregator<ElementType, ElementNum>, ElementType, TFixedAllocator<ElementNum>>
{
public:
	using Super = TRingAggregator_Base<TFixedSizeRingAggregator<ElementType, ElementNum>, ElementType, TFixedAllocator<ElementNum>>;
	TFixedSizeRingAggregator() : Super(ElementNum) {}
	// no constructor with MaxNum parameter, because it's a compile time constant ("ElementNum")
};
